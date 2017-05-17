#include "plc_drv.h"
#include "resource.h"
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <windowsx.h> // combo control included.
#include <commctrl.h> // listview/richedit control included.
                      // tree control. included .
#include <assert.h>
#include <process.h>
#include <stdio.h>
#include <tchar.h>
#include "stdint.h"
#include "host_drv.h"
#include "comm_enum.h"
#include "warnings.h"

//   MISC unwind.
// >>===========================================================
#define  en_window(hwnd_) EnableWindow ( (hwnd_), TRUE)
#define  di_window(hwnd_) EnableWindow ( (hwnd_), FALSE)
#define  set_window_text SetWindowText
#define  get_window_text GetWindowText 

#define  INT_PTR_FALSE ((INT_PTR)FALSE)
#define  INT_PTR_TRUE ((INT_PTR)TRUE)

//   column indicate.
// >>===========================================================
#define ISR_COLUMN_LINE 0
#define ISR_COLUMN_FUNC (ISR_COLUMN_LINE + 1)
#define ISR_COLUMN_REL_DISTANCE (ISR_COLUMN_FUNC + 1)
#define ISR_COLUMN_ABS_DISTANCE (ISR_COLUMN_REL_DISTANCE + 1)
#define ISR_COLUMN_CUR_DISTANCE (ISR_COLUMN_ABS_DISTANCE + 1)

//   char encode
// >>===========================================================
#if defined (_UNICODE) || defined (UNICODE)
#define au wchar_t 
#define au_cc TEXT
#define secominfo secominfo_unicode
#define secominfo_sec secominfo_section_unicode
#define alloc_secominfos alloc_secominfo_unicode
#define dealloc_secominfos dealloc_secominfo_unicode
#else
#define au char
#define au_cc TEXT
#define secominfo secominfo_ansi
#define secominfo_sec secominfo_section_ansi
#define alloc_secominfos alloc_secominfo_ansi
#define dealloc_secominfos dealloc_secominfo_ansi
#endif

//   .plcdef for save/load infos.
// >>===========================================================
struct _plcdef {
  int pmm;
  int ppul;
  int cut_off;
  int cut_cor;
  int punch_cor;
  int punch_down;
  int punch_up;
  int cut_down;
  int total_lines;
};

struct _bparam_s {
  //
  // basic params.
  // static write into PLC.
  // >>===========================================================
  int com_numbs;
  int pmm;
  int ppul;
  int cut_off;
  int cut_cor;
  int punch_cor;
  int punch_down;
  int punch_up;
  int cut_down;

  BOOL pmm_emp;
  BOOL ppul_emp;
  BOOL cut_off_emp;
  BOOL cut_cor_emp;
  BOOL punch_cor_emp;
  BOOL punch_down_emp;
  BOOL punch_up_emp;
  BOOL cut_down_emp;
  //
  // NT window handle.
  // >>===========================================================
  HWND hwco_com_numbs;
  HWND hwet_pmm;
  HWND hwet_ppul;
  HWND hwet_cut_off;
  HWND hwet_cut_cor;
  HWND hwet_punch_cor;
  HWND hwet_punch_down;
  HWND hwet_punch_up;
  HWND hwet_cut_down;
} SCP_bparams;

struct _item_edit {
  //
  // First, the editing is symmetrical, 
  // and the current mold data will be mirrored horizontally.
  // the two punch holes in the middle, if present, will be merged into a large block
  // The direction of internal to external diffusion. (!!!!3)
  // sample0,   item len : 1200 
  // item:100/200    (!!!!3)                                                                 [                                   mirror ]
  // result:(cut)200 - (punch)200 - (merge left 100) -    200   - (merge right 100) - (punch)200- (cut)200
  //                                                                     |______ [merge result ] _______|
  // >>===========================================================
  int item_len;
  int item_numb;
  //
  // NT window handle.
  // >>===========================================================
  HWND hwtr_item_edit;
  HWND hwet_item_len;
  HWND hwet_item_numb;
  HWND hwbt_append_item; 
} SCP_itedit;

struct _line_edit {
  //
  // NT window handle.
  // >>===========================================================
  HWND hwet_reset_line;
  HWND hwet_delete_line;
} SCP_ieedit;

struct _item_poll {
  //
  // NT window handle.
  // >>===========================================================
  HWND hwlt_poll_line;
  
} SCP_itpoll;

struct _run_ctl {
  //
  // NT window handle.
  // >>===========================================================
  HWND hwbt_link_port;
  HWND hwbt_bparams;
  HWND hwbt_lparams;
  HWND hwbt_run_test;
  HWND hwbt_interrupt;
  HWND hwbt_cover_alert;
  HWND hwbt_punch_down;
  HWND hwbt_punch_up;
  HWND hwbt_cut_down;
  HWND hwbt_closeIO;
} SCP_rctl;

struct _stat_out {

  au chbuf[2048]; // XXX: share with whole program.

  HWND hwet_stat_out;
} SCP_statOut;

struct _misc {

  void *cb_hooke ; // old edit's ctl callback...
  void *cb_hookt; // old tree's ctl callback...
  void *cb_hookm; // dialog 's callback...

  BOOL last_del_key; 
  BOOL last_ins_key; 

  HANDLE commport;

  HWND hwdi_main; // dialog callback....
  
  int lines; // :-1 
  
  int lines_poll;
  int lines_end;
  int run_ppul;
  int run_pmm;

  BOOL run_monitor;
  BOOL compelte_pulse;
} SCP_misc;


uint16_t vailed8 (uint16_t nums) {
  
 /* e.g. 
  * 1234 vailed.
  * 9000 invailed.
  * 1007 vailed.
  * 1811 invailed.
  * 0 ~ 65535
  */
  uint16_t d0 = nums % 10 >> 0;
  uint16_t d1 = nums % 100 / 10;
  uint16_t d2 = nums % 1000 / 100;
  uint16_t d3 = nums % 10000 / 1000;
   
  if ( d0 > 7 || d1 > 7)
    return -1;
  if ( d2 > 7)
    return -1;
  return d0 + d1 * 8 + d2 * 8 * 8 + d3 * 8 * 8 * 8;
}

au * __cdecl format_text (au *format, ...) {
 
  va_list arg_list; 
  va_start (arg_list, format); 
#if defined (UNICODE) || defined (_UNICODE)
  wvsprintf (& SCP_statOut.chbuf[0], format, arg_list); 
#else
  vsprintf (& SCP_statOut.chbuf[0], format, arg_list); 
#endif
  va_end(arg_list); 
  return & SCP_statOut.chbuf[0];
}

void append_text (au *chbuf) {

  /* append chbuf */
  SendMessage (SCP_statOut.hwet_stat_out, EM_SETSEL, -2, -1);
  SendMessage (SCP_statOut.hwet_stat_out, EM_REPLACESEL, TRUE, (LPARAM)chbuf);
  /* set scroll tail */
  SendMessage (SCP_statOut.hwet_stat_out, WM_VSCROLL, SB_BOTTOM, 0);
}

void __cdecl append_text2 (au *format, ...) {
 
  va_list arg_list; 
  va_start (arg_list, format); 
#if defined (UNICODE) || defined (_UNICODE)
  wvsprintf (& SCP_statOut.chbuf[0], format, arg_list); 
#else
  vsprintf (& SCP_statOut.chbuf[0], format, arg_list); 
#endif
  va_end(arg_list); 
  append_text (& SCP_statOut.chbuf[0]);
}

au * get_window_str (HWND window) {

  get_window_text (window, & SCP_statOut.chbuf[0], sizeof (SCP_statOut.chbuf));
  return & SCP_statOut.chbuf[0];
}

int get_window_int (HWND window, PBOOL empty) {

  BOOL empty_ = FALSE;
  
  if (get_window_str (window)[0] == 0)
    empty_ = TRUE;
  if (empty != NULL)
  * empty = empty_;

  return _ttoi (& SCP_statOut.chbuf[0]);
}

void set_window_int (HWND window, int val) {

  _stprintf ( & SCP_statOut.chbuf[0], au_cc ("%d"), val);
  set_window_text (window, & SCP_statOut.chbuf[0]);
}

void comm_close (void)
{
  __try {
    if (SCP_misc.commport != INVALID_HANDLE_VALUE) {
      PurgeComm (SCP_misc.commport, PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR | PURGE_TXABORT);
      CloseHandle (SCP_misc.commport);
    }
    SCP_misc.commport = INVALID_HANDLE_VALUE;
  }
  __except (EXCEPTION_EXECUTE_HANDLER) {
    SCP_misc.commport = INVALID_HANDLE_VALUE;
  }
}


void comm_init (int comm_index)
{
  DCB dcbs;
  COMMTIMEOUTS ct;  
  BOOL success_io_;

  _stprintf (& SCP_statOut.chbuf[0], au_cc ("\\\\.\\COM%i"), comm_index);
  comm_close ();

  SCP_misc.commport = CreateFile (& SCP_statOut.chbuf[0], GENERIC_READ | GENERIC_WRITE, 0,
		      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  success_io_ = GetCommState (SCP_misc.commport, & dcbs);
  assert (success_io_ != FALSE);

  dcbs.BaudRate = CBR_9600;
  dcbs.fParity  = TRUE;
  dcbs.Parity   = EVENPARITY;
  dcbs.StopBits = ONESTOPBIT;
  dcbs.ByteSize = 7;
  dcbs.fDtrControl = DTR_CONTROL_DISABLE;
  dcbs.fRtsControl = RTS_CONTROL_DISABLE;
	
  success_io_ = SetupComm (SCP_misc.commport, 2048, 2048);
  assert (success_io_ != FALSE);
  success_io_ = SetCommState (SCP_misc.commport, & dcbs);
  assert (success_io_ != FALSE);

  // SetTimeOut. 
  ct.ReadIntervalTimeout = 0x0000FFFF;
  ct.ReadTotalTimeoutMultiplier = 0x0000FFFF;
  ct.ReadTotalTimeoutConstant = 0xFFFFFFFE;
  ct.WriteTotalTimeoutMultiplier = 0x0000FFFF;
  ct.WriteTotalTimeoutConstant =   0xFFFFFFFE;  
  
  success_io_ = SetCommTimeouts (SCP_misc.commport, &ct);  
  assert (success_io_ != FALSE);

  success_io_ = PurgeComm (SCP_misc.commport, PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR | PURGE_TXABORT);
  assert (success_io_ != FALSE);
}

//
//
//   PLC register read/write.
//
// ============================================================================
#include "plc_raccess.inl"

//
//
//   listview deal.
//
// ============================================================================

int getl_abs_int (int line) {
  
  ListView_GetItemText (SCP_itpoll.hwlt_poll_line, line, ISR_COLUMN_ABS_DISTANCE, 
               SCP_statOut.chbuf, sizeof (SCP_statOut.chbuf));
  return _ttoi (SCP_statOut.chbuf);
}

int getl_cur_int (int line) {
  
  ListView_GetItemText (SCP_itpoll.hwlt_poll_line, line, ISR_COLUMN_CUR_DISTANCE, 
               SCP_statOut.chbuf, sizeof (SCP_statOut.chbuf));
  return _ttoi (SCP_statOut.chbuf);
}

int getl_rel_int (int line) { 

  ListView_GetItemText (SCP_itpoll.hwlt_poll_line, line, ISR_COLUMN_REL_DISTANCE, 
               SCP_statOut.chbuf, sizeof (SCP_statOut.chbuf));
  return _ttoi (SCP_statOut.chbuf);
}

int getl_func (int line) { 

  ListView_GetItemText (SCP_itpoll.hwlt_poll_line, line, ISR_COLUMN_FUNC, SCP_statOut.chbuf, 
               sizeof (SCP_statOut.chbuf));
  return (_tcscmp (SCP_statOut.chbuf, au_cc ("P")) == 0) ? FUNC_CODE_PUNCH : FUNC_CODE_CUT;
}

void setl_line_int (int line) {
  ListView_SetItemText (SCP_itpoll.hwlt_poll_line, line, ISR_COLUMN_LINE, format_text (au_cc ("%d"), line));
}

void setl_abs_int (int line, int val) {
  ListView_SetItemText (SCP_itpoll.hwlt_poll_line, line, ISR_COLUMN_ABS_DISTANCE, format_text (au_cc ("%d"), val));
}

void setl_cur_int (int line, int val) {
  ListView_SetItemText (SCP_itpoll.hwlt_poll_line, line, ISR_COLUMN_CUR_DISTANCE, format_text (au_cc ("%d"), val));
}

void setl_rel_int (int line, int val) { 
  ListView_SetItemText (SCP_itpoll.hwlt_poll_line, line, ISR_COLUMN_REL_DISTANCE, format_text (au_cc ("%d"), val));
}

void setl_func (int line, BOOL punch) { 
  ListView_SetItemText (SCP_itpoll.hwlt_poll_line, line, ISR_COLUMN_FUNC, FALSE != !! punch ? au_cc ("P") : au_cc ("C") );
}

void 
insert_lvi_intail (int rel_val, BOOL punch)
{
  LVITEM LVI; 

  LVI.mask = LVIF_TEXT;
  LVI.pszText = format_text (au_cc ("%d"), ++ SCP_misc.lines) ; 
  LVI.iItem = SCP_misc.lines;
  LVI.iSubItem = 0;

  ListView_InsertItem (SCP_itpoll.hwlt_poll_line, & LVI);
  ListView_SetItemText (SCP_itpoll.hwlt_poll_line, LVI.iItem, ISR_COLUMN_LINE, format_text (au_cc ("%d"), SCP_misc.lines));
  ListView_SetItemText (SCP_itpoll.hwlt_poll_line, LVI.iItem, ISR_COLUMN_REL_DISTANCE, format_text (au_cc ("%d"), rel_val));
  ListView_SetItemText (SCP_itpoll.hwlt_poll_line, LVI.iItem, ISR_COLUMN_FUNC, FALSE != !! punch ? au_cc ("P") : au_cc ("C"));

  if (SCP_misc.lines == 0) {
    ListView_SetItemText (SCP_itpoll.hwlt_poll_line, LVI.iItem, ISR_COLUMN_ABS_DISTANCE, format_text (au_cc ("%d"), rel_val));
  } else { 
    int c = getl_abs_int (SCP_misc.lines - 1);
    setl_abs_int (SCP_misc.lines, c + rel_val);
  }
}

HTREEITEM getcur_treeitem (HWND tree)
{
  TVHITTESTINFO tv_htinfos;
  
  GetCursorPos (& tv_htinfos.pt);
  ScreenToClient (tree, & tv_htinfos.pt);

  tv_htinfos.hItem = NULL;
  tv_htinfos.flags = TVHT_NOWHERE;

  return TreeView_HitTest (tree, & tv_htinfos);
}

LRESULT 
CALLBACK tree_hookcallback (HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
// because NO TVN_KEYUP message.
// we have to hook callback...
// >>===========================================================
  if ( message == WM_KEYUP) {
    if (wparam == VK_DELETE) SCP_misc.last_del_key = FALSE;
    if (wparam == VK_INSERT) SCP_misc.last_ins_key = FALSE;
  }
  return
  CallWindowProc (SCP_misc.cb_hookt, window, message, wparam, lparam);
}

int 
CALLBACK dialog_hookcallback (HWND window, UINT msg, WPARAM wpam, LPARAM lpam)
{
  union {
    void *comp; /* Compatible pointer **/
    LPARAM comp2;
    NMHDR *base;
    NMLISTVIEW *list;
    NMLVCUSTOMDRAW *draw;
    NMTVDISPINFO *tvdisp;
    NMTVKEYDOWN *tvkey;
  } nmset;

  HTREEITEM tree_cur;
  TV_INSERTSTRUCT TV_IN;

  nmset.comp2 = lpam;

  if (msg == WM_NOTIFY && nmset.base->idFrom   == IDTR_ITEM_EDIT
  &&                      nmset.base->hwndFrom == SCP_itedit.hwtr_item_edit)
  {
      if (nmset.base->code == NM_DBLCLK) {
        /*
          * double click
          */
tree_cur = getcur_treeitem (nmset.base->hwndFrom);
          TreeView_EditLabel (nmset.base->hwndFrom, tree_cur);
TreeView_SelectItem (nmset.base->hwndFrom, tree_cur);
        }
      else if (nmset.base->code == TVN_BEGINLABELEDIT) {
        /*
          * see.
          * https://msdn.microsoft.com/en-us/library/windows/desktop/bb773821(v=vs.85).aspx
          */
                // set window style .
              SetWindowLongPtr ( TreeView_GetEditControl (nmset.base->hwndFrom),
                              GWL_STYLE, 
                  ES_NUMBER | GetWindowLongPtr (TreeView_GetEditControl (nmset.base->hwndFrom),  GWL_STYLE));
          return FALSE; // enable edit..
        }
      else if (nmset.base->code == TVN_KEYDOWN) {
        if ( (nmset.tvkey->wVKey == VK_DELETE) && (SCP_misc.last_del_key == FALSE) ) {
          SCP_misc.last_del_key = TRUE;
            // Get Current item. 
          tree_cur = getcur_treeitem (nmset.base->hwndFrom);
          if (tree_cur != NULL)
            {
              TreeView_DeleteItem (SCP_itedit.hwtr_item_edit, tree_cur);
            }
        }
        else if ( (nmset.tvkey->wVKey == VK_INSERT) && (SCP_misc.last_ins_key == FALSE)) {
        SCP_misc.last_ins_key = TRUE;

      // Get Current item. 
          { 
            TV_IN.hParent = TVI_ROOT;
            TV_IN.hInsertAfter = TVI_LAST;
            TV_IN.item.pszText = au_cc ("200");
            TV_IN.item.mask = TVIF_TEXT;
            TV_IN.item.cchTextMax = _tcsclen ( au_cc ("200"));
              
            TreeView_InsertItem (SCP_itedit.hwtr_item_edit, & TV_IN);
          }
      }
    }
     else if (nmset.base->code == TVN_ENDLABELEDIT) {
      /*
        * end edit
        */
    if (nmset.tvdisp->item.pszText == NULL )
      return FALSE;
    if (* nmset.tvdisp->item.pszText == 0)
      return FALSE;
            return TRUE;
        return TRUE;
     } 
  }
  return
  CallWindowProc (SCP_misc.cb_hookm, window, msg, wpam, lpam);
}


int get_treeitem_int (HTREEITEM htitem, PBOOL empty) {

  static au _g_chbuf[4096];
  BOOL empty_ = FALSE;
  BOOL success_;

  TVITEM TV_IT;
  ZeroMemory (& TV_IT, sizeof (TV_IT));

  TV_IT.mask = TVIF_TEXT;
  TV_IT.pszText = _g_chbuf;
  TV_IT.cchTextMax = sizeof (_g_chbuf) ;
  TV_IT.hItem = htitem;
  
  success_ =          TreeView_GetItem (SCP_itedit.hwtr_item_edit, & TV_IT);
  assert (success_ != FALSE);

  if (_g_chbuf [0] == 0)
    empty_ = TRUE;
  if (empty != NULL)
  * empty = empty_;

  return _ttoi (_g_chbuf);
}

int get_treeitem_int_total (HTREEITEM htitem_, PBOOL empty, int *nums) {

  BOOL empty_ = TRUE;
  HTREEITEM htitem = htitem_;

  int total = 0;
  int nums_ = 0;

  if (htitem != NULL) 
  {
    total += get_treeitem_int (htitem, NULL);
    nums_ = 1;
    empty_ = FALSE;

    htitem = TreeView_GetNextSibling (SCP_itedit.hwtr_item_edit, htitem);

    while (htitem != NULL)
    {
      total += get_treeitem_int (htitem, NULL);
                htitem = TreeView_GetNextSibling (SCP_itedit.hwtr_item_edit, htitem);
      nums_++;
    }
  }
  if (empty != NULL)
   * empty = empty_;
  if (nums != NULL)
   * nums = nums_;
  return total;
}

LRESULT 
CALLBACK edit_hookcallback (HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  UCHAR key = (UCHAR)LOWORD(wparam);

  if (message == WM_CHAR) {
    /* allow ch 0~9 and BACKSPACE key **/
    if ((! (key >= '0' && key <= '9') && key != VK_BACK && key != VK_RETURN))
      return 0;
    else if (key == VK_RETURN)
      {
        // Get current item. 
        BOOL empty;
        int count = get_window_int (window, & empty);
        if (empty == FALSE)
          {
            // switch 
            if (window == SCP_ieedit.hwet_delete_line)
             {
                // del group, sort current.
                if (SCP_misc.lines  == -1) 
                {
                  append_text2 ( au_cc ("No Item current.\n"));
                } 
                else if (count <= SCP_misc.lines)
                {
                  // del current item. 
                  ListView_DeleteItem (SCP_itpoll.hwlt_poll_line, count);

                  // adjust after. 
                  if (count != SCP_misc.lines)
                  {
                     int ii;
                     for ( ii =count; ii != SCP_misc.lines; ii++)
                     {
                       int abs_old = getl_abs_int (ii - 1);
                       int rel_cur = getl_rel_int (ii);
                       setl_line_int (ii);
                       setl_abs_int ( ii, abs_old + rel_cur);
                     }
                  }

                  SCP_misc.lines--;
                  append_text2 ( au_cc ("Delete success..\n"));
                }
                else 
                {
                  append_text2 ( au_cc ("Delete overflow...\n"));
                }
             }
           else if (window == SCP_ieedit.hwet_reset_line)
             {
                // reset line.,check. 
                int total_line = 0;
                
                plc_rdw (PLC_DWSTORGE_TOTAL_SCANLINE, & total_line);
            
                if (count  >=  total_line) 
                {
                  append_text2 ( au_cc ("Reset overflow!.\n"));
                } 
                else if (1)
                {
                  plc_wdw (PLC_DWSTORGE_INTERRUPT_SCANLINE, count);
                  append_text2 ( au_cc ("Reset success!.\n"));
                }
             }
          }
      }
  }
  return
  CallWindowProc (SCP_misc.cb_hooke, window, message, wparam, lparam);
}


int infos_check (void)
{
  struct _tag_s { 
    int nums;
    BOOL emp;
  };

  struct _tag_s pmm;
  struct _tag_s ppul;
  struct _tag_s cut_off;
  struct _tag_s cut_cor;
  struct _tag_s punch_cor;
  struct _tag_s punch_down;
  struct _tag_s punch_up;
  struct _tag_s cut_down;

#  define ___KK_UNWIND(lllspps) \
   lllspps.##nums = get_window_int (SCP_bparams.hwet_##lllspps, & lllspps.##emp)
# define ___KK_CHK_RETURN(itemsss, lllspdap)\
  { if (itemsss.##emp == TRUE) { return -1; } }

  ___KK_UNWIND (pmm);
  ___KK_UNWIND (ppul);
  ___KK_UNWIND (cut_off);
  ___KK_UNWIND (cut_cor);
  ___KK_UNWIND (punch_cor);
  ___KK_UNWIND (punch_down);
  ___KK_UNWIND (punch_up);
  ___KK_UNWIND (cut_down);

  ___KK_CHK_RETURN (pmm, au_cc ("unset pmm\n"));
  ___KK_CHK_RETURN (ppul, au_cc ("unset ppul\n"));
  ___KK_CHK_RETURN (cut_off, au_cc ("unset cut_off\n"));
  ___KK_CHK_RETURN (cut_cor, au_cc ("unset cut_cor\n"));
  ___KK_CHK_RETURN (punch_cor, au_cc ("unset punch_cor\n"));
  ___KK_CHK_RETURN (punch_down, au_cc ("unset punch_down\n"));
  ___KK_CHK_RETURN (punch_up, au_cc ("unset punch_up\n"));
  ___KK_CHK_RETURN (cut_down, au_cc ("unset cut_down\n"));

#  undef  ___KK_UNWIND
#  define ___KK_UNWIND(sm_) \
   sm_.##nums = get_window_int (SCP_bparams.hwet_##sm_, & sm_.##emp)
#  undef  ___KK_CHK_RETURN 
#  define ___KK_CHK_RETURN(sv_, sc_)\
  { if (sv_.##emp == TRUE) { append_text (sc_); return ( INT_PTR) TRUE; } }

  return ( SCP_misc.lines == -1) == -1 ? -1: 0;
}


void read_plcdef (au *fname)
{
  static int prev;
  int ii;
  FILE *fp = _tfopen (fname, au_cc ("rb"));
  struct _plcdef pldef;

  assert (fp != NULL);
  fread (& pldef, sizeof (pldef), 1, fp);

  set_window_int (SCP_bparams.hwet_pmm, pldef.pmm);
  set_window_int (SCP_bparams.hwet_ppul, pldef.ppul);
  set_window_int (SCP_bparams.hwet_cut_off, pldef.cut_off);
  set_window_int (SCP_bparams.hwet_cut_cor, pldef.cut_cor);
  set_window_int (SCP_bparams.hwet_punch_cor, pldef.punch_cor);
  set_window_int (SCP_bparams.hwet_punch_down, pldef.punch_down);
  set_window_int (SCP_bparams.hwet_punch_up, pldef.punch_up);
  set_window_int (SCP_bparams.hwet_cut_down, pldef.cut_down);

  // delete current's list-item.
  SCP_misc.lines = -1;
  ListView_DeleteAllItems (SCP_itpoll.hwlt_poll_line);

  // set format chbuf 
  for (prev = 0, ii = 0; ii != pldef.total_lines + 1;) {

    int val;

    fread (& val, sizeof (val), 1, fp);
    insert_lvi_intail ( (val & 0x7FFFFFFF) - (prev & 0x7FFFFFFF), ! (val & 0x80000000));

    prev = val;
    ii ++;
  }

  SCP_misc.lines = pldef.total_lines;
  fclose (fp); 
}

void write_plcdef (au *fname)
{
  struct _plcdef pldef;
  int ii;
  FILE *fp = _tfopen (fname, au_cc ("wb+"));
  assert (fp != NULL);

  pldef.pmm = get_window_int (SCP_bparams.hwet_pmm, NULL);
  pldef.ppul = get_window_int (SCP_bparams.hwet_ppul, NULL);
  pldef.cut_off = get_window_int (SCP_bparams.hwet_cut_off, NULL);
  pldef.cut_cor = get_window_int (SCP_bparams.hwet_cut_cor, NULL);
  pldef.punch_cor = get_window_int (SCP_bparams.hwet_punch_cor, NULL);
  pldef.punch_down = get_window_int (SCP_bparams.hwet_punch_down, NULL);
  pldef.punch_up = get_window_int (SCP_bparams.hwet_punch_up, NULL);
  pldef.cut_down = get_window_int (SCP_bparams.hwet_cut_down, NULL);
  pldef.total_lines = SCP_misc.lines;

  fwrite (& pldef, sizeof (pldef), 1, fp);

  // write format chbuf
  for (ii = 0; ii != pldef.total_lines + 1; ii++) {

    int ii0 = getl_abs_int (ii);
        ii0|= (getl_func (ii) == FUNC_CODE_CUT) ? 0x80000000 : 0;

    fwrite (& ii0, sizeof (ii), 1, fp);
  }
  fclose (fp);
}

INT_PTR 
CALLBACK author_callback (HWND window, UINT msg, WPARAM wpam, LPARAM lpam)
{
  if ( msg == WM_COMMAND && LOWORD (wpam) == IDCANCEL) {
    EndDialog (window, 0);
  }

  return INT_PTR_FALSE;
}

INT_PTR 
CALLBACK dialog_callback (HWND window, UINT msg, WPARAM wpam, LPARAM lpam)
{
  LV_COLUMN LVC;

  switch (msg) {

  case WM_INITDIALOG:

    // Clear global memory.
    ZeroMemory (& SCP_bparams, sizeof (SCP_bparams));
    ZeroMemory (& SCP_ieedit, sizeof (SCP_ieedit));
    ZeroMemory (& SCP_itedit, sizeof (SCP_itedit));
    ZeroMemory (& SCP_itpoll, sizeof (SCP_itpoll));
    ZeroMemory (& SCP_misc, sizeof (SCP_misc));
    ZeroMemory (& SCP_rctl, sizeof (SCP_rctl));
    ZeroMemory (& SCP_statOut, sizeof (SCP_statOut));

    SCP_misc.commport = INVALID_HANDLE_VALUE;
    SCP_misc.lines = -1;

    // Get all ctl's hwnd.
    SCP_misc.hwdi_main = window;
    SCP_bparams.hwco_com_numbs = GetDlgItem (window, IDCB_COM_NUMB);
    SCP_bparams.hwet_cut_cor = GetDlgItem (window, IDET_CUT_COR);
    SCP_bparams.hwet_cut_down = GetDlgItem (window, IDET_CUT_DOWN);
    SCP_bparams.hwet_cut_off = GetDlgItem (window, IDET_CUT_OFF);
    SCP_bparams.hwet_pmm = GetDlgItem (window, IDET_PMM);
    SCP_bparams.hwet_ppul = GetDlgItem (window, IDET_PPULSE);
    SCP_bparams.hwet_punch_cor = GetDlgItem (window, IDET_PUNCH_COR);
    SCP_bparams.hwet_punch_down = GetDlgItem (window, IDET_PUNCH_DOWN);
    SCP_bparams.hwet_punch_up = GetDlgItem (window, IDET_PUNCH_UP);
    SCP_itedit.hwbt_append_item =  GetDlgItem (window, IDBT_APPEND_ITEM);
    SCP_itedit.hwet_item_len =  GetDlgItem (window, IDET_ITEM_LEN);
    SCP_itedit.hwet_item_numb =  GetDlgItem (window, IDET_ITEM_NUMB);
    SCP_itedit.hwtr_item_edit =  GetDlgItem (window, IDTR_ITEM_EDIT);
    SCP_itpoll.hwlt_poll_line =  GetDlgItem (window, IDLT_ITEM_POLL);
    SCP_ieedit.hwet_delete_line =  GetDlgItem (window, IDET_DEL_LINE);
    SCP_ieedit.hwet_reset_line =  GetDlgItem (window, IDET_RST_LINE);
    SCP_rctl.hwbt_link_port=  GetDlgItem (window, IDBT_LINK_PORT);
    SCP_rctl.hwbt_bparams=  GetDlgItem (window, IDBT_WRITE_BPARAM);
    SCP_rctl.hwbt_cover_alert=  GetDlgItem (window, IDBT_COVER_ALERT);
    SCP_rctl.hwbt_closeIO=  GetDlgItem (window, IDBT_CLOSE_IO);
    SCP_rctl.hwbt_cut_down=  GetDlgItem (window, IDBT_CUT_DOWN);
    SCP_rctl.hwbt_interrupt=  GetDlgItem (window, IDBT_INTERRUPT);
    SCP_rctl.hwbt_lparams=  GetDlgItem (window, IDBT_WRITE_LPARAM);
    SCP_rctl.hwbt_punch_down=  GetDlgItem (window, IDBT_PUNCH_DOWN);
    SCP_rctl.hwbt_punch_up=  GetDlgItem (window, IDBT_PUNCH_UP);
    SCP_rctl.hwbt_run_test=  GetDlgItem (window, IDBT_RUN_TEST);
    SCP_statOut.hwet_stat_out = GetDlgItem (window, IDET_STATOUT);

    // Set list ctrl style.
    ListView_SetExtendedListViewStyle (SCP_itpoll.hwlt_poll_line, 
              ListView_GetExtendedListViewStyle (SCP_itpoll.hwlt_poll_line)
                | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES); 

    LVC.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT; 
    LVC.pszText = au_cc ("line");        
    LVC.fmt = LVCFMT_CENTER;
    LVC.cx = 35;
    LVC.iSubItem = 0; 
    ListView_InsertColumn (SCP_itpoll.hwlt_poll_line, ISR_COLUMN_LINE, & LVC); 
    LVC.pszText = au_cc ("func");        
    ListView_InsertColumn (SCP_itpoll.hwlt_poll_line, ISR_COLUMN_FUNC, & LVC); 
    LVC.pszText = au_cc ("rel distance");        
    LVC.cx = 68;
    ListView_InsertColumn (SCP_itpoll.hwlt_poll_line, ISR_COLUMN_REL_DISTANCE, & LVC); 
    LVC.pszText = au_cc ("abs distance");        
    ListView_InsertColumn (SCP_itpoll.hwlt_poll_line, ISR_COLUMN_ABS_DISTANCE, & LVC); 
    LVC.pszText = au_cc ("cur distance");        
    ListView_InsertColumn (SCP_itpoll.hwlt_poll_line, ISR_COLUMN_CUR_DISTANCE, & LVC); 

    // di window. 
    di_window (SCP_ieedit.hwet_reset_line);
    di_window (SCP_rctl.hwbt_bparams);
    di_window (SCP_rctl.hwbt_cover_alert);
    di_window (SCP_rctl.hwbt_closeIO);
    di_window (SCP_rctl.hwbt_cut_down);
    di_window (SCP_rctl.hwbt_interrupt);
    di_window (SCP_rctl.hwbt_lparams);
    di_window (SCP_rctl.hwbt_punch_down);
    di_window (SCP_rctl.hwbt_punch_up);
    di_window (SCP_rctl.hwbt_run_test);

    SCP_misc.cb_hookm = SetWindowLongPtr (window, GWLP_WNDPROC, (LONG) dialog_hookcallback);
    SCP_misc.cb_hookt = SetWindowLongPtr (SCP_itedit.hwtr_item_edit, GWLP_WNDPROC, (LONG) tree_hookcallback);
    SCP_misc.cb_hooke = SetWindowLongPtr (SCP_ieedit.hwet_delete_line, GWLP_WNDPROC, (LONG) edit_hookcallback);
                        SetWindowLongPtr (SCP_ieedit.hwet_reset_line, GWLP_WNDPROC, (LONG) edit_hookcallback);
    return INT_PTR_TRUE;

  case WM_COMMAND:

    switch (LOWORD (wpam)) {

    case ID_APP_EXIT0:

      // close PLC, Prevent accidental disasters
      plc_wy (PUNCH_IO_DOWN, FALSE);
      plc_wy (PUNCH_IO_UP, FALSE);
      plc_wy (CUT_IO_DOWN, FALSE);
      plc_wy (USELESS_IO2, FALSE);
      plc_wy (USELESS_IO, FALSE);
      plc_wy (ALERT_IO_TRIGGER, FALSE);
      plc_force_close ();

      PostQuitMessage (0);
      break;

    case ID_ABOUT_AUTHOR:

      DialogBox (GetModuleHandle (NULL), MAKEINTRESOURCE (IDDI_AUTHOR), window, author_callback);
      break;

    case ID_FILE_SAVE0:
      {
        OPENFILENAME ofn; 
        static au file_name[MAX_PATH];
        ZeroMemory (& ofn, sizeof (ofn));
        ZeroMemory (& file_name[0], sizeof (file_name));

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.lpstrFilter = au_cc ("plcdef file (.plcdef)\0*.plcdef\0");
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = & file_name[0];
        ofn.nMaxFile = sizeof (file_name);
        ofn.lpstrTitle = au_cc ("save plfdef to...");
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

        if (GetSaveFileName (& ofn) != FALSE) { 
          if (infos_check () == 0) {
            _tcscat (& file_name[0], au_cc (".plcdef"));
            write_plcdef (& file_name[0]);
          }
        } else { 
          MessageBox (NULL, au_cc ("please select pos"), NULL, MB_ICONERROR); 
        } 
      }
      break;            

    case ID_FILE_OPEN0:
      {
        OPENFILENAME ofn; 
        static au file_name[MAX_PATH];
        ZeroMemory (& ofn, sizeof (ofn));
        ZeroMemory (& file_name[0], sizeof (file_name));

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.lpstrFilter = au_cc ("plcdef file (.plcdef)\0*.plcdef\0");
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = & file_name[0];
        ofn.nMaxFile = sizeof (file_name);
        ofn.lpstrTitle = au_cc ("open plfdef from...");
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetSaveFileName (& ofn) != FALSE) { 
          read_plcdef (& file_name[0]);
        } else { 
          MessageBox (NULL, au_cc ("please select file"), NULL, MB_ICONERROR); 
        } 
      }
      break;

    case IDBT_WRITE_LPARAM:
      {
        // test PLC's env. 
        // =======================================
        bool it_, anyb_;
        bool it_success = plc_rm (PLC_MASSERT_INIT_TIMING, &  it_);
        assert (it_ != FALSE);
               
        if (it_ != TRUE || SCP_misc.lines == -1) {
          append_text ( au_cc ("No init timing/item\n")); return FALSE;
        }

        // load PLC's env.
        // =======================================
        {
          int pmm = 0;
          int ppul = 0;
          int cut_off = 0;
          int cut_cor = 0;
          int punch_cor = 0;
          int off_base = 0;
          int off_icc = 0;
          int * ppc_block  = malloc ( sizeof (int) * (SCP_misc.lines + 32));
          assert (ppc_block != NULL);

          plc_rdw (PLC_DWSTORGE_PER_MM, & pmm);
          plc_rdw (PLC_DWSTORGE_PER_PULSE, & ppul);
          plc_rdw (PLC_DWSTORGE_CUT_OFFSET, & cut_off);
          plc_rdw (PLC_DWSTORGE_CUT_CORRENT, & cut_cor);
          plc_rdw (PLC_DWSTORGE_PUNCH_CORRENT, & punch_cor);

          {
            // load line infos. 
            int ii;
            for ( ii =0; ii != SCP_misc.lines + 1; ii++) {
              ppc_block[ii] = getl_abs_int (ii);
              ppc_block[ii] |= ( getl_func (ii) == FUNC_CODE_CUT ) ? 0x80000000 : 0;
            }
            for ( ii =0; ii != SCP_misc.lines + 1; ii++) {

              if (ppc_block[ii] & 0x80000000) {
                ppc_block[ii] &= ~0x80000000;
                ppc_block[ii] += cut_off;
                ppc_block[ii] |= 0x80000000;
              }
            }

            // sort .wait. ..asd
            {
              int ix;
              int iy;
             
              for (iy = 0; iy != SCP_misc.lines + 1; iy++) {

                for (ix = iy; ix != SCP_misc.lines + 1; ix++) {

                int iy_val = ppc_block[iy] & 0x7FFFFFFF;
                int ix_val = ppc_block[ix] & 0x7FFFFFFF;

                if (ix_val < iy_val) {
                    int temp = ppc_block[iy];
                    ppc_block[iy] = ppc_block[ix];
                    ppc_block[ix] = temp;
                  }
                }
              }
            }

            // corrent cut/punch
            for (ii = 0; ii != SCP_misc.lines + 1; ii++) {

              int mb_cur= ppc_block[ii]     & 0x80000000;
              int mc_cor= mb_cur ? cut_cor : punch_cor;
              ppc_block[ii] &= ~0x80000000;

              if (ii == 0) {
                off_base = ppc_block[ii];
                ppc_block[ii] -= mc_cor;
              } else {
                int off_tem = ppc_block[ii] - off_base;  
                int temp = ppc_block[ii-1] & ~0x80000000;
                off_base = ppc_block[ii];
                ppc_block[ii] = temp +  off_tem - ( mc_cor - off_icc);
              }
              off_icc = mc_cor;     
              ppc_block[ii] |= mb_cur;
           }

           // DAC. 
           for (ii = 0; ii != SCP_misc.lines + 1; ii++) {

             int mb_cur= ppc_block[ii]     & 0x80000000;
             int mc_cor= mb_cur ? cut_cor : punch_cor;
             ppc_block[ii] &= ~0x80000000;
             ppc_block[ii] = ppc_block[ii] * ppul / pmm;
             ppc_block[ii]|= mb_cur;
           }

           // write info into PLC's D-file register. 
           //
           // reset M385[PLC_MASSERT_INIT_LIST_GROUP]
           // reset M18[PLC_MASSERT_FIO_TRANSLATE_COMPLETION]
           // reset M11[PLC_MASSERT_PLC_COMPLETION]
           // set M12[PLC_MASSERT_HOST_COMPLETION]
           // set M16[PLC_MASSERT_EVENT_SEND_FIO]
           //        D10[PLC_DWCURRENT_IO_TRANSLATE]
           //        D12[PLC_DWCURRENT_IO_TRANSLATE_ADDRESS. ]

           plc_wm (PLC_MASSERT_INIT_LIST_GROUP, FALSE);
           plc_wm (PLC_MASSERT_PLC_COMPLETION, FALSE);

           for (ii = 0; ii != SCP_misc.lines + 1; ii++) {

             bool io_success_;
             bool io_comp;

             if (ii == 0) {
               // open translate. 
               plc_wm (PLC_MASSERT_EVENT_SEND_FIO, TRUE);
             }
                          
             plc_wdd (PLC_DWCURRENT_IO_TRANSLATE, ppc_block[ii]);
             plc_wdw (PLC_DWCURRENT_IO_TRANSLATE_ADDRESS, ii);
             plc_wm (PLC_MASSERT_HOST_COMPLETION, TRUE);

             // wait PLC signal..
             while (TRUE)
             {
               plc_rm (PLC_MASSERT_PLC_COMPLETION, & io_comp);
               if (io_comp == TRUE) {
                   plc_wm (PLC_MASSERT_PLC_COMPLETION, FALSE);
                   break;
               }
             }
           }

           plc_wm (PLC_MASSERT_FIO_TRANSLATE_COMPLETION, TRUE);
           plc_wm (PLC_MASSERT_EVENT_SEND_FIO, FALSE);
           plc_wdw (PLC_DWSTORGE_TOTAL_SCANLINE, SCP_misc.lines + 1);
           plc_wdw (PLC_DWSTORGE_INTERRUPT_SCANLINE, 0);
           plc_wdw (PLC_DWSTORGE_POLL_SCANLINE, 0);
           plc_wm (PLC_MASSERT_ABNORMAL_EXIT, FALSE);
           plc_wm (PLC_MASSERT_INIT_LIST_GROUP, TRUE);
           append_text ( au_cc ("Set list success\n")); 
         }
       }
     }
     break;

    case IDBT_CLOSE_IO:

      plc_wy (PUNCH_IO_DOWN, FALSE);
      plc_wy (PUNCH_IO_UP, FALSE);
      plc_wy (CUT_IO_DOWN, FALSE);
      plc_wy (USELESS_IO2, FALSE);
      plc_wy (USELESS_IO, FALSE);

    case IDBT_COVER_ALERT:
      plc_wy (ALERT_IO_TRIGGER, FALSE);
      break;

    case IDBT_CUT_DOWN:
      plc_wy (CUT_IO_DOWN, FALSE);
      plc_wy (CUT_IO_DOWN, TRUE);
      // simple use Sleep. .
      Sleep (get_window_int ( SCP_bparams.hwet_cut_down, NULL) * 100);
      plc_wy (CUT_IO_DOWN, FALSE);
      break;

    case IDBT_PUNCH_DOWN:
      plc_wy (PUNCH_IO_DOWN, FALSE);
      plc_wy (PUNCH_IO_DOWN, TRUE);
      // simple use Sleep. .
      Sleep (get_window_int ( SCP_bparams.hwet_punch_down, NULL) * 100);
      plc_wy (PUNCH_IO_DOWN, FALSE);
      break;

    case IDBT_PUNCH_UP:
      plc_wy (PUNCH_IO_UP, FALSE);
      plc_wy (PUNCH_IO_UP, TRUE);
      // simple use Sleep. .
      Sleep (get_window_int ( SCP_bparams.hwet_punch_up, NULL) * 100);
      plc_wy (PUNCH_IO_UP, FALSE);
      break;

    case IDBT_INTERRUPT:

      if ( _tcscmp ( get_window_str (SCP_rctl.hwbt_interrupt), au_cc ("interrupt")) == 0) {

        plc_force_close ();

        plc_wy (PUNCH_IO_DOWN, FALSE);
        plc_wy (PUNCH_IO_UP, FALSE);
        plc_wy (CUT_IO_DOWN, FALSE);
        plc_wy (USELESS_IO2, FALSE);
        plc_wy (USELESS_IO, FALSE);
        plc_wy (ALERT_IO_TRIGGER, FALSE);

        en_window (SCP_ieedit.hwet_reset_line);
        en_window (SCP_rctl.hwbt_closeIO);
        en_window (SCP_rctl.hwbt_punch_down);
        en_window (SCP_rctl.hwbt_punch_up);
        en_window (SCP_rctl.hwbt_cover_alert);
        en_window (SCP_rctl.hwbt_cut_down);

        // set_window_text (SCP_rctl.hwbt_run_test, au_cc ("run"));
        set_window_text (SCP_rctl.hwbt_interrupt, au_cc ("resume"));

      } else if ( _tcscmp ( get_window_str (SCP_rctl.hwbt_interrupt), au_cc ("resume")) == 0) {
        //  reset scanline into PLC.
        //  reset button = "interrupt"
        int line = 0;

        di_window (SCP_ieedit.hwet_reset_line);
        di_window (SCP_rctl.hwbt_closeIO);
        di_window (SCP_rctl.hwbt_punch_down);
        di_window (SCP_rctl.hwbt_punch_up);
        di_window (SCP_rctl.hwbt_cover_alert);
        di_window (SCP_rctl.hwbt_cut_down);
        di_window (SCP_rctl.hwbt_run_test);
        set_window_text (SCP_rctl.hwbt_interrupt, au_cc ("interrupt"));
        set_window_text (SCP_rctl.hwbt_run_test, au_cc ("run!!!"));

        plc_rdw (PLC_DWSTORGE_INTERRUPT_SCANLINE, & line);
        plc_wm (PLC_MASSERT_HOST_NO_INTERRUPT, TRUE);

        SCP_misc.lines_poll = line;
           
        // fill previous's list-item, empty next.
        {
          int ii;
            
          for (ii = 0; ii != SCP_misc.lines_poll; ii++) {
            setl_cur_int (ii, getl_abs_int ( ii));
          }
          for (ii = SCP_misc.lines_poll; ii != SCP_misc.lines_end; ii++) {
            ListView_SetItemText (SCP_itpoll.hwlt_poll_line, ii, ISR_COLUMN_CUR_DISTANCE, au_cc (""));
          }
        }
        plc_force_open ();
      }
      break;

    case IDBT_RUN_TEST:

      if ( _tcscmp (get_window_str (SCP_rctl.hwbt_run_test), au_cc ("run test")) == 0) {

        plc_force_close ();
        plc_force_open ();

        // en_window (SCP_rctl.hwbt_interrupt);
        en_window (SCP_rctl.hwbt_lparams);

        set_window_text (SCP_rctl.hwbt_run_test, au_cc ("plc check"));
      } else if ( _tcscmp (get_window_str (SCP_rctl.hwbt_run_test), au_cc ("plc check")) == 0) {
        
        // set timing/list item show.
        // check. interrupt line.
        bool timing_init;
        bool list_item_init;
        bool interrupt;

        int pmm =0;
        int ppul =0;
        int cut_off =0;
        int cut_cor =0;
        int punch_cor =0;
        int punch_down =0;
        int punch_up =0;
        int cut_down =0;
        int PLC_line =0;

        plc_force_close ();
        plc_force_open ();

        plc_rm (PLC_MASSERT_INIT_TIMING, & timing_init);
        plc_rm (PLC_MASSERT_INIT_LIST_GROUP, & list_item_init);
        plc_rm (PLC_MASSERT_ABNORMAL_EXIT, & interrupt);
        plc_rdw (PLC_DWSTORGE_TOTAL_SCANLINE, & PLC_line);

        if ( timing_init == FALSE || list_item_init == FALSE) {

          append_text ( au_cc ("Timing/item not set\n"));
          set_window_text (SCP_rctl.hwbt_run_test, au_cc ("run test"));
          return INT_PTR_FALSE;
        }

        // 
        //  set timing.
        // 
        plc_rdw (PLC_DWSTORGE_PER_MM, & pmm);
        plc_rdw (PLC_DWSTORGE_PER_PULSE, & ppul);
        plc_rdw (PLC_DWSTORGE_CUT_OFFSET, & cut_off);
        plc_rdw (PLC_DWSTORGE_CUT_CORRENT, & cut_cor);
        plc_rdw (PLC_DWSTORGE_PUNCH_CORRENT, & punch_cor);
        plc_rdw (PLC_DWSTORGE_PUNCH_DOWN_TIMING, & punch_down);
        plc_rdw (PLC_DWSTORGE_PUNCH_UP_TIMING, & punch_up);
        plc_rdw (PLC_DWSTORGE_CUT_DOWN_TIMING, & cut_down);

        set_window_int (SCP_bparams.hwet_pmm, pmm);
        set_window_int (SCP_bparams.hwet_ppul, ppul);
        set_window_int (SCP_bparams.hwet_cut_off, cut_off);
        set_window_int (SCP_bparams.hwet_cut_cor, cut_cor);
        set_window_int (SCP_bparams.hwet_punch_cor, punch_cor);
        set_window_int (SCP_bparams.hwet_punch_down, punch_down);
        set_window_int (SCP_bparams.hwet_punch_up, punch_up);
        set_window_int (SCP_bparams.hwet_cut_down, cut_down);

        di_window (SCP_bparams.hwet_pmm);
        di_window (SCP_bparams.hwet_ppul);
        di_window (SCP_bparams.hwet_cut_off);
        di_window (SCP_bparams.hwet_cut_cor);
        di_window (SCP_bparams.hwet_punch_cor);
        di_window (SCP_bparams.hwet_punch_down);
        di_window (SCP_bparams.hwet_punch_up);
        di_window (SCP_bparams.hwet_cut_down);

        // 
        //  set list item..
        // 
        {
          int ii, temp;
          int *ppc_block = NULL;
                          
          ppc_block = malloc ( sizeof (int) * PLC_line);

          // load PLC's line infos.
          // 
          for (ii = 0; ii != PLC_line; ii++) 
          {
            if (ii == 0) {

              plc_wdw (PLC_DWCURRENT_IO_TRANSLATE_ADDRESS, ii);
              plc_wm (PLC_MASSERT_HOST_COMPLETION, TRUE);
              plc_wm (PLC_MASSERT_PLC_COMPLETION, FALSE); 
              plc_wm (PLC_MASSERT_EVENT_RECV_FIO, TRUE); 
            }

            plc_wdw (PLC_DWCURRENT_IO_TRANSLATE_ADDRESS, ii);
            plc_wm (PLC_MASSERT_HOST_COMPLETION, TRUE);
                          // wait PLC signal..
            while (TRUE)
            {
              bool io_comp;
              int temp_mask;
              plc_rm (PLC_MASSERT_PLC_COMPLETION, & io_comp);
              if (io_comp == TRUE) {
                plc_wm (PLC_MASSERT_PLC_COMPLETION, FALSE);
              // read temp. 
                plc_rdd (PLC_DWCURRENT_IO_TRANSLATE, & temp);
                temp_mask = temp & 0x80000000;
                temp &= ~0x80000000;
                ppc_block[ii] = temp * pmm/ ppul;
                ppc_block[ii]|= temp_mask;
                break;
              }
            }     
          }

          // set listview 
          ListView_DeleteAllItems (SCP_itpoll.hwlt_poll_line);
          SCP_misc.lines = -1;

          {
            int ii;
            static int val00 = 0;

            for ( ii=0; ii != PLC_line; ii++) {
                
              if (ii == 0) {
                insert_lvi_intail (ppc_block[ii] & 0x7FFFFFFF, ! (ppc_block[ii] & 0x80000000));
              } else {
                insert_lvi_intail ( (ppc_block[ii] & 0x7FFFFFFF) - (val00 & 0x7FFFFFFF), ! (ppc_block[ii] & 0x80000000));
              }
              val00 = ppc_block[ii];
            }
          }

          SCP_misc.lines = PLC_line - 1;
          SCP_misc.lines_poll = 0;
          SCP_misc.lines_end = PLC_line;

          free (ppc_block);
          append_text ( au_cc ("No errors.!\n"));
        }  
 
        if ( interrupt == TRUE) {
          int line = 0;
          plc_rdw (PLC_DWSTORGE_INTERRUPT_SCANLINE, & line);

          append_text2 ( au_cc ("Tnterrupt line:%d.!\n"), line);
        } else {
          append_text2 ( au_cc ("No interrupt line\n"));
        }

        en_window (SCP_ieedit.hwet_reset_line);
        di_window (SCP_ieedit.hwet_delete_line);
        set_window_text (SCP_rctl.hwbt_run_test, au_cc ("run!!!"));

      } else if ( _tcscmp ( get_window_str (SCP_rctl.hwbt_run_test), au_cc ("pass ok")) == 0) {

        //  send complete event. reset all. 
        //  reset button = "run"
        plc_wm (PLC_MASSERT_PLC_COMPLETION, FALSE);
        plc_wm (PLC_MASSERT_CONTINUE_NEXT_PROCESS, TRUE);

        // wait PLC signal..
        while (TRUE)
        {
          bool io_comp;
          plc_rm (PLC_MASSERT_PLC_COMPLETION, & io_comp);

          if (io_comp == TRUE) 
          {
            plc_wm (PLC_MASSERT_PLC_COMPLETION, FALSE);
            break;
          }
        }

        { 
          // empty. list-item.
          int ii;
          for ( ii = 0; ii != SCP_misc.lines + 1; ii++)

          ListView_SetItemText (SCP_itpoll.hwlt_poll_line, ii, ISR_COLUMN_CUR_DISTANCE, au_cc (""));
        }

        di_window (SCP_rctl.hwbt_interrupt);
        set_window_text (SCP_rctl.hwbt_run_test, au_cc ("run!!!"));
      } else if ( _tcscmp ( get_window_str (SCP_rctl.hwbt_run_test), au_cc ("run!!!")) == 0) {
          
        plc_force_close (); // reset .
        plc_force_open ();
 
        en_window (SCP_rctl.hwbt_interrupt);
        di_window (SCP_rctl.hwbt_lparams);

        // 
        // load line/timing infos from PLC
        // save old infos. 
        plc_rdw (PLC_DWSTORGE_PER_MM, & SCP_misc.run_pmm);
        plc_rdw (PLC_DWSTORGE_PER_PULSE, & SCP_misc.run_ppul);
        plc_wm (PLC_MASSERT_HOST_NO_INTERRUPT, TRUE);
        plc_rdw (PLC_DWSTORGE_TOTAL_SCANLINE, & SCP_misc.lines_end);

        SCP_misc.compelte_pulse = FALSE;
        SCP_misc.run_monitor = TRUE;
        SCP_misc.lines_poll = 0;
        SCP_misc.lines_end &= 0xFFFF;
        SCP_misc.run_pmm &=  0xFFFF;
        SCP_misc.run_ppul &=  0xFFFF;

        di_window (SCP_rctl.hwbt_run_test);
        di_window (SCP_rctl.hwbt_lparams);
        di_window (SCP_rctl.hwbt_bparams);
        di_window (SCP_ieedit.hwet_delete_line);
        di_window (SCP_rctl.hwbt_link_port);
        di_window (SCP_ieedit.hwet_reset_line);
      }
      break;


    case IDBT_APPEND_ITEM:
      // append item. 
      {
        bool len_emp = FALSE;
        int len = get_window_int (SCP_itedit.hwet_item_len, & len_emp);
        bool num_emp = FALSE;
        int num = get_window_int (SCP_itedit.hwet_item_numb, & num_emp);

        if ( len_emp != FALSE) {
          append_text ( au_cc ("No set item's length\n")); return FALSE;
        } else if ( len == 0) {
          append_text ( au_cc ("Item's length is 0\n")); return FALSE;
        }
        if ( num_emp != FALSE) {
          append_text ( au_cc ("No set item's numb\n")); return FALSE;
        } else if ( num == 0) {
          append_text ( au_cc ("Item's numb is 0\n")); return FALSE;
        }

        if (TreeView_GetRoot (SCP_itedit.hwtr_item_edit) != NULL)
        {
        // collect item's length. 
          int num_pv;
          int total;
          total = get_treeitem_int_total ( TreeView_GetRoot (SCP_itedit.hwtr_item_edit) , NULL, & num_pv);
          if (total * 2 >= len)
          {
            //
            // ============================================
            append_text ( au_cc ("Item's length <= item edit's\n"));
          }
          else 
          {
            // calc marginal part. 
            int ta = len/2;
            int ii ;
            int tp = num_pv;
            int rm_patr  = ta - get_treeitem_int (TreeView_GetRoot (SCP_itedit.hwtr_item_edit), NULL);
            int *pivot_block = malloc ( sizeof (int) * num_pv * 2 +  sizeof (int));

            HTREEITEM htitem0 = TreeView_GetRoot (SCP_itedit.hwtr_item_edit);
            HTREEITEM htitem3 = TreeView_GetNextSibling (SCP_itedit.hwtr_item_edit, htitem0);
            HTREEITEM htitem2 = htitem3;
            assert (pivot_block != NULL);

            // calc remain. 
            while (htitem3 != NULL) {
              rm_patr -= get_treeitem_int (htitem3, NULL);
              htitem3 = TreeView_GetNextSibling (SCP_itedit.hwtr_item_edit, htitem3);
            }

            // first fill marginal/middle part.
            pivot_block[0] = rm_patr;
            pivot_block[num_pv * 2] = rm_patr;
            pivot_block[num_pv] = get_treeitem_int (htitem0, NULL) * 2;

            // fill left. 
            for (ii = num_pv - 1; ii != 0; ii --) {
              pivot_block[ii] = get_treeitem_int (htitem2, NULL);
              htitem2 = TreeView_GetNextSibling (SCP_itedit.hwtr_item_edit, htitem2);
            } 
            // fill right. 
            htitem3 = TreeView_GetNextSibling (SCP_itedit.hwtr_item_edit, htitem0);
            for (ii = num_pv + 1; ii != num_pv * 2; ii ++) { 
              pivot_block[ii] = get_treeitem_int (htitem3, NULL);
              htitem3 = TreeView_GetNextSibling (SCP_itedit.hwtr_item_edit, htitem3);
            }  

            // insert list item.
            {
              int ii;
              int ij;

              for (ij = 0; ij != num; ij++)
              {
                  for (ii = 0; ii != num_pv * 2; ii++)
                        insert_lvi_intail (pivot_block[ii], TRUE);
                    insert_lvi_intail (pivot_block[ii], FALSE);
              }
              free (pivot_block);
            }
          }
        }
        else 
        { 
            // imm app.
          int ij;

          for (ij = 0; ij != num; ij++)
          {
            insert_lvi_intail (len, FALSE);
          }
        }
      }
      break;

      case IDBT_WRITE_BPARAM:
      {
        struct _tag_s { 
          int nums;
          BOOL emp;
        };

        struct _tag_s pmm;
        struct _tag_s ppul;
        struct _tag_s cut_off;
        struct _tag_s cut_cor;
        struct _tag_s punch_cor;
        struct _tag_s punch_down;
        struct _tag_s punch_up;
        struct _tag_s cut_down;


        ___KK_UNWIND (pmm);
        ___KK_UNWIND (ppul);
        ___KK_UNWIND (cut_off);
        ___KK_UNWIND (cut_cor);
        ___KK_UNWIND (punch_cor);
        ___KK_UNWIND (punch_down);
        ___KK_UNWIND (punch_up);
        ___KK_UNWIND (cut_down);

        ___KK_CHK_RETURN (pmm, au_cc ("unset pmm\n"));
        ___KK_CHK_RETURN (ppul, au_cc ("unset ppul\n"));
        ___KK_CHK_RETURN (cut_off, au_cc ("unset cut_off\n"));
        ___KK_CHK_RETURN (cut_cor, au_cc ("unset cut_cor\n"));
        ___KK_CHK_RETURN (punch_cor, au_cc ("unset punch_cor\n"));
        ___KK_CHK_RETURN (punch_down, au_cc ("unset punch_down\n"));
        ___KK_CHK_RETURN (punch_up, au_cc ("unset punch_up\n"));
        ___KK_CHK_RETURN (cut_down, au_cc ("unset cut_down\n"));

        //
        // set PLC's save infos. 
        // 
        // M386:
        //      assert init timing and misc params.
        // 
        //  D136-word:host computer interrupt/except recovery scanline. 
        //  D138-word:punch-down timing [100ms/per]
        //  D139-word:punch-up timing [100ms/per]
        //  D140-word:cut-down timing [100ms/per]
        //  D141-word:total scan line 
        //  D142-word:poll scan line.
        //  D143-word:per mm.
        //  D144-word:per pulse.
        //  D145-word:cut off
        //  D146-word:cut corrent.
        //  D147-word:punch corrent.
        // ====================================================================================
        {
          // Set M386 FALSE.
          bool io_success_ = plc_wm (PLC_MASSERT_INIT_TIMING, FALSE);
          // set timing. 
          io_success_ = io_success_ && plc_wdw (PLC_DWSTORGE_PUNCH_DOWN_TIMING, punch_down.nums);
          io_success_ = io_success_ && plc_wdw (PLC_DWSTORGE_PUNCH_UP_TIMING, punch_up.nums);
          io_success_ = io_success_ && plc_wdw (PLC_DWSTORGE_CUT_DOWN_TIMING, cut_down.nums);
          // set per mm.  but.. in face. noused.. hehe/
          io_success_ = io_success_ && plc_wdw (PLC_DWSTORGE_PER_MM, pmm.nums);
          io_success_ = io_success_ && plc_wdw (PLC_DWSTORGE_PER_PULSE , ppul.nums);
          io_success_ = io_success_ && plc_wdw (PLC_DWSTORGE_CUT_OFFSET, cut_off.nums);
          io_success_ = io_success_ && plc_wdw (PLC_DWSTORGE_CUT_CORRENT, cut_cor.nums);
          io_success_ = io_success_ && plc_wdw (PLC_DWSTORGE_PUNCH_CORRENT, punch_cor.nums);
          io_success_ = plc_wm (PLC_MASSERT_INIT_TIMING, TRUE);
                  
          if (io_success_ != FALSE)
            append_text ( au_cc ("Timing set success\n"));
          else 
            append_text ( au_cc ("Timing set fail\n"));
        }
      }
      break;

      case IDBT_LINK_PORT: // no use chr table resource.

        get_window_text (SCP_rctl.hwbt_link_port,
                       & SCP_statOut.chbuf[0],  sizeof ( SCP_statOut.chbuf));
                       
        if (_tcscmp (& SCP_statOut.chbuf[0], au_cc ("discannect")) != 0)
        {
          int ii;
          get_window_text (SCP_bparams.hwco_com_numbs, & SCP_statOut.chbuf[0],
                                    sizeof ( SCP_statOut.chbuf));
          if (SCP_statOut.chbuf[0] == 0) {
            append_text ( au_cc ("No serial number set\n"));
          } else {
            ii = _ttoi (& SCP_statOut.chbuf[3]);
            comm_init (ii);
            append_text ( au_cc ("Open serial success\n"));
            // dis commb.
            di_window (SCP_bparams.hwco_com_numbs);
            en_window (SCP_rctl.hwbt_run_test);
            en_window (SCP_rctl.hwbt_bparams);
            set_window_text (SCP_rctl.hwbt_link_port, au_cc ("discannect"));
          }
        }
        else 
        {
          comm_close ();
          append_text ( au_cc ("Discon serial success\n"));

          en_window (SCP_bparams.hwco_com_numbs);
          di_window (SCP_rctl.hwbt_run_test);
          di_window (SCP_rctl.hwbt_bparams);
          set_window_text (SCP_rctl.hwbt_link_port, au_cc ("link port"));
        }
        break;

      case IDCB_COM_NUMB:

        if (HIWORD (wpam) == CBN_DROPDOWN) {
          struct secominfo_sec *sec;
          struct secominfo *scpoll;
          int ii;

          alloc_secominfos (& sec, NULL);
          ComboBox_ResetContent ( (HWND) lpam);
       
          if (sec == NULL)
            return INT_PTR_FALSE;
          if (sec->num == 0) {
            dealloc_secominfos (sec);
            return INT_PTR_FALSE;
          }

          for (ii = 0, scpoll = sec->level; ii != sec->num; ii++) {
            ComboBox_AddItemData ((HWND) lpam, format_text ( au_cc ("COM%d"), scpoll->comm_pos));
            scpoll = scpoll->level;
          }

          dealloc_secominfos (sec);
        }
      default:
        break;
      }
  default:
    break;
  }
  return INT_PTR_FALSE;
}

extern 
bool licence_check (void);

int WINAPI _tWinMain (HINSTANCE instance_cur, HINSTANCE instance_prev,
                          LPTSTR cmd_line, int cmd_show)

{
  if ( licence_check () != TRUE) {
    return -1;
  }

  __try {

    HWND di = CreateDialogParam (instance_cur, MAKEINTRESOURCE (IDDI_MAIN), NULL, dialog_callback, 0);
    MSG msg;
    ShowWindow (di, SW_SHOW);  // always shown, regardless of WS_VISIBLE flag
    while (TRUE) {
      if (PeekMessage (& msg, NULL, 0, 0, PM_REMOVE)) {

        if (msg.message == WM_QUIT)
          break;

        if (IsDialogMessage (di, & msg)) continue;
          TranslateMessage (& msg);
          DispatchMessage (& msg);
      } else if (SCP_misc.run_monitor != FALSE) {

        int abs_pulse;
        int abs_mm;

        plc_rc32d (PULSE_ENCODER_ADDR, & abs_pulse);
        abs_mm = abs_pulse * SCP_misc.run_pmm / SCP_misc.run_ppul;

        if ( SCP_misc.lines_poll < SCP_misc.lines_end) {
          if ( getl_abs_int ( SCP_misc.lines_poll) <= abs_mm) {                              
            setl_cur_int (SCP_misc.lines_poll, 
                getl_abs_int ( SCP_misc.lines_poll));
            SCP_misc.lines_poll++;
            ListView_EnsureVisible (SCP_itpoll.hwlt_poll_line, SCP_misc.lines_poll, FALSE);
          } else {
            setl_cur_int (SCP_misc.lines_poll, abs_mm);
          }
        } else {
            // poll run timing. 
            bool process_comp;
            plc_rm (PLC_MASSERT_PROCESSING_COMPLETION, & process_comp);

            if ( process_comp != FALSE && SCP_misc.compelte_pulse == FALSE) {

              SCP_misc.compelte_pulse = TRUE;

              set_window_text (SCP_rctl.hwbt_run_test, au_cc ("pass ok"));
              en_window (SCP_rctl.hwbt_run_test);
            }
        }
        Sleep (13);
      }
    }
    comm_close ();
    return 0;
  } 
  __except (EXCEPTION_EXECUTE_HANDLER) {
    comm_close ();
    return -1;
  }
}




#if defined (_DEBUG)

int main (void)
{ 
  _tWinMain (GetModuleHandle (NULL), NULL, NULL, SW_SHOWNORMAL);
  return 0;
}
#endif 