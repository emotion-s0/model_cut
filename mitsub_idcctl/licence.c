#include "stdbool.h"
#include <tchar.h>
#include <assert.h>
#include <Winsock2.h>
#include <iphlpapi.h>
#include <Windows.h>
#include <shlwapi.H>
#include "crc32.h"
#include "resource.h"
#include "config.h"

/* host 's friendly name */
static TCHAR fd_buf[2048];
/* host 's MAC address */
static BYTE ntcc_cod[8];
/* software's licence key */
static BYTE pass_cod[8];
/* default edit-ctl's callback */
static void *cb_edit = NULL;
/* edit-ctl's handle [register]*/
static HWND hwet_reg = NULL;
/* result for return */
static bool pass_check = FALSE;
/* for GET MAC **/
static IP_ADAPTER_ADDRESSES ip_adp_address[256];

/* Convert characters to specific number - noexport */
static 
char ascii_to_num (char ch) {
  /* e.g.
   *  source '9' -> target 9
   *  source 'A' -> target 10
   *  source '1' -> target 1
   *  source 'a' -> (nondone, Don't use lowercase letters in fx1s-14mr-001).
   */
  if (ch >= '0' && ch <= '9')
    return (ch - '0');
  if (ch >= 'A' && ch <= 'F')
    return (ch - ('A' - 10));
  else
    assert (0);

  return ch;
}

/* Convert number to specific characters - noexport */
static 
char num_to_ascii (char ch) {
  /* e.g.
   *  source 9 -> target '9'
   *  source A -> target '0'
   *  source 1 -> target '1'
   *  source a -> (nondone, Don't use lowercase letters in fx1s-14mr-001).
   */
  if (ch >= 0x00 && ch <= 0x09)
    return (ch + '0');
  if (ch >= 0x0A && ch <= 0x0F)
    return (ch + ('A' - 10));
  else
    assert (0);

  return ch;
}


static 
void decode_encipher_mac (BYTE *chrbuf)
{
  if ( strlen (chrbuf) != 12 ) {
    pass_check = FALSE;
    return;
  }

  {
    DWORD ii = 0;
    BYTE mac[8];
    BYTE *mac2 = & ntcc_cod[0];
    BYTE encode6[8];

    * (DWORD *) & mac[0] = (* (DWORD *) & mac2[0] & 0xFFFFFF) - 0x788921;
    * (DWORD *) & mac[3] = ( (* (DWORD *) & mac2[3] & 0xFFFFFF)- 0x144567) ^ 0xFFFFFF;

    for (; ii != 12; ii+= 2) {
      BYTE obi = ascii_to_num (chrbuf[ii+1]);
      BYTE obt = obi << 4;
           obt|= ascii_to_num (chrbuf[ii]);
      encode6[ii>>1]= obt;
    }

    if (strncmp (& encode6[0], & mac[0], 6) == 0) {

      // set register-table's infos.
      // create, oopen. SOFTWARE\\mitsub_idcctl key.
      LSTATUS status = 0;
      DWORD rw;
      DWORD crc32;
      HKEY rtkey;
      DWORD opt_mask = KEY_ALL_ACCESS;

#  if defined (_WOW64)
      opt_mask |= KEY_WOW64_64KEY;
#  endif 
      status = RegCreateKeyEx (HKEY_LOCAL_MACHINE, _T ("SOFTWARE\\mitsub_idcctl"), 
                        0, REG_NONE, REG_OPTION_NON_VOLATILE, opt_mask, NULL, & rtkey, & rw);
      assert (status == 0);

      // set KeyValue.
      // XXX: should use resource.
      RegSetValueEx (rtkey, _T ("Author"), 0, REG_SZ, _T ("程序设计:徐诩/电路设计:徐志波"), sizeof (TCHAR) * _tcslen (_T ("程序设计:徐诩/电路设计:徐志波")));
      RegSetValueEx (rtkey, _T ("Company"), 0, REG_SZ, _T ("常州高斯电气公司"), sizeof (TCHAR) * _tcslen (_T ("常州高斯电气公司")));
      RegSetValueEx (rtkey, _T ("MapDevice"), 0, REG_SZ, & fd_buf[0], sizeof (TCHAR) * _tcslen (& fd_buf[0]));
      RegSetValueEx (rtkey, _T ("MyMail"), 0, REG_SZ, _T ("sosicklove@yahoo.com"), sizeof (TCHAR) * _tcslen (_T ("sosicklove@yahoo.com")));
      RegSetValueEx (rtkey, _T ("PlcDevice"), 0, REG_SZ, _T ("三菱 fx1s-14mr"), sizeof (TCHAR) * _tcslen (_T ("三菱 fx1s-14mr")));
      RegSetValueEx (rtkey, _T ("Warnings"), 0, REG_SZ, _T ("不要尝试修改此处内容, 尤其是映射设备和Ntcc/Pass"), sizeof (TCHAR) * _tcslen (_T ("不要尝试修改此处内容, 尤其是映射设备和Ntcc/Pass")));
      RegSetValueEx (rtkey, _T ("NtccCode"), 0, REG_QWORD, & ntcc_cod[0] , 8);
      RegSetValueEx (rtkey, _T ("PassCode"), 0, REG_QWORD, & encode6[0] , 8);

      // Get crc32 [NtccCode/PassCode/MapDevice]
      crc32 = crc32_calc (0, & ntcc_cod[0], 6);
      crc32 = crc32_calc (crc32, & encode6[0], 6);
      crc32 = crc32_calc (crc32, & fd_buf[0], sizeof (TCHAR) * _tcslen (& fd_buf[0]));

      // Write crc32 .
      RegSetValueEx (rtkey, _T ("CrcCode"), 0, REG_DWORD, & crc32 , 4);
      RegCloseKey (rtkey);

      pass_check = TRUE;
    }
    else 
      pass_check = FALSE;
  }
}

static 
int CALLBACK hook_editnumb (HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  UCHAR key = (UCHAR)LOWORD(wparam);

  if (message == WM_CHAR)
  {
    if ((key >= '0' && key <= '9') )
      goto __final;
    if ((key >= 'A' && key <= 'F') || key == VK_BACK)
      goto __final;
    return 0;
  } 
  __final:
  return CallWindowProc (cb_edit, window, message, wparam, lparam);
}

INT_PTR 
CALLBACK licence_callback (HWND window, UINT msg, WPARAM wpam, LPARAM lpam)
{
  if ( msg == WM_COMMAND && LOWORD (wpam) == IDCANCEL)  {

    pass_check = FALSE;
    EndDialog (window, 0);
  } 
  if ( msg == WM_COMMAND && LOWORD (wpam) == IDOK)  {

    CHAR chbuf[256];
    GetWindowTextA (hwet_reg, chbuf, sizeof (chbuf));
    /* decode user's licence **/
    decode_encipher_mac (& chbuf[0]);

    EndDialog (window, 0);
  } 
  else if (msg == WM_INITDIALOG) {
    LONG ii;
    TCHAR cbuf[64];

    hwet_reg = GetDlgItem (window, IDET_REG_CODE);
    cb_edit = (void *)SetWindowLongPtr (hwet_reg, GWLP_WNDPROC, (LONG)hook_editnumb);

    for (ii = 0; ii != 12; ii+= 2) {
      cbuf[ii+0] = num_to_ascii (ntcc_cod[ii>>1] & 0x0F);
      cbuf[ii+1] = num_to_ascii (ntcc_cod[ii>>1]>> 4);
      cbuf[12] = 0;
    }
    SetWindowText ( GetDlgItem (window, IDET_NT_CODE), & cbuf[0]);
  }
  return (INT_PTR) FALSE;
}

bool licence_check (void) {

#if defined (_WOW64)
  DWORD opt_mask = KEY_ALL_ACCESS | KEY_WOW64_64KEY;
#else
  DWORD opt_mask = KEY_ALL_ACCESS;
#endif 
  HKEY rtkey = NULL;
  LSTATUS scss = RegOpenKeyEx (HKEY_LOCAL_MACHINE, _T ("SOFTWARE\\mitsub_idcctl"), 0, opt_mask, & rtkey);

  if (scss == ERROR_SUCCESS) {
    // open success.
    // 
    // We will proofread the CRC/ and other code in the code to \
    // prevent software corruption caused by malicious tampering 
    BYTE ntcc[8];
    BYTE pass[8];
    DWORD crc32_0 =0;
    DWORD temp_unused;
    DWORD temp_unused2;

    // search MapDevice from Ethernet network interface. 
    LONG ii;
    ULONG outBufLen = sizeof (IP_ADAPTER_ADDRESSES); 
    PIP_ADAPTER_ADDRESSES adap_poll = & ip_adp_address[0];
    DWORD dwRetVal;
    DWORD crc32;

    RegQueryValueEx (rtkey, _T ("MapDevice"), 0, & temp_unused2, & fd_buf[0], & temp_unused);
    RegQueryValueEx (rtkey, _T ("MapDevice"), 0, & temp_unused2, & fd_buf[0], & temp_unused);
    RegQueryValueEx (rtkey, _T ("NtccCode"), 0, & temp_unused2, & ntcc[0], & temp_unused);
    RegQueryValueEx (rtkey, _T ("NtccCode"), 0, & temp_unused2, & ntcc[0], & temp_unused);
    RegQueryValueEx (rtkey, _T ("PassCode"), 0, & temp_unused2, & pass[0], & temp_unused);
    RegQueryValueEx (rtkey, _T ("PassCode"), 0, & temp_unused2, & pass[0], & temp_unused);
    RegQueryValueEx (rtkey, _T ("CrcCode"), 0, & temp_unused2, & crc32_0, & temp_unused);
    RegQueryValueEx (rtkey, _T ("CrcCode"), 0, & temp_unused2, & crc32_0, & temp_unused);

    GetAdaptersAddresses (0, 0, NULL, NULL, & outBufLen);
    dwRetVal = GetAdaptersAddresses (0, 0, NULL, adap_poll, &outBufLen);
    assert (dwRetVal == NO_ERROR);
    assert (adap_poll != NULL);

    // search Ethernet network interface. 
    for (; adap_poll != NULL; adap_poll = adap_poll->Next) {
      if (adap_poll->IfType == IF_TYPE_ETHERNET_CSMACD) {
        int Zcount = 0;
        for (ii = 0; ii != adap_poll->PhysicalAddressLength; ii ++) {
          if (adap_poll->PhysicalAddress[ii] == 0) {
            Zcount ++;
          }
        }
        if (Zcount != adap_poll->PhysicalAddressLength) {
          // record current device's MAC address.
          // record current device's FriendlyName
          assert (adap_poll->PhysicalAddressLength == 6);

          if ( _tcsncmp (& adap_poll->FriendlyName[0], & fd_buf[0], 
                 _tcslen ( & adap_poll->FriendlyName[0])) == 0)
          {
            if ( memcmp (& adap_poll->PhysicalAddress[0], & ntcc[0], 
                   adap_poll->PhysicalAddressLength) == 0) 
            {
              // Get crc32 [NtccCode/PassCode/MapDevice]
              crc32 = crc32_calc (0, & ntcc[0], 6);
              crc32 = crc32_calc (crc32, & pass[0], 6);
              crc32 = crc32_calc (crc32, & fd_buf[0], sizeof (TCHAR) * _tcslen (& fd_buf[0]));

              if (crc32 == crc32_0) { 
                // CMP Ntcc and Pass Code. 
                BYTE ntcc2[12];

                * (DWORD *) & ntcc2[0] = (* (DWORD *) & ntcc[0] & 0xFFFFFF) - 0x788921;
                * (DWORD *) & ntcc2[3] = ( (* (DWORD *) & ntcc[3] & 0xFFFFFF)- 0x144567) ^ 0xFFFFFF;

                if ( memcmp (& ntcc2[0], & pass[0], 6) == 0) {
                  RegCloseKey (rtkey);
                  return TRUE;
                }
              }
            }
          }
        }
      }
    }
    MessageBoxA (NULL, "ready to crack the program ???, \nIf not, please re-register",
 "Please contact moecmks for registration code", MB_ICONERROR);

    //  no find, return FALSE. 
    //  XXX:Maybe BUG, but i not test..
    SHDeleteKey (rtkey, NULL); 
    RegDeleteKeyEx (HKEY_LOCAL_MACHINE, _T ("SOFTWARE\\mitsub_idcctl"), KEY_WOW64_64KEY ,0);
    RegCloseKey (rtkey);
    return FALSE;
  } else if (scss == 2) {

    // first init. we will find host device's MAC address as seed to 
    // make password.
    LONG ii;
    ULONG outBufLen = sizeof (IP_ADAPTER_ADDRESSES); 
    PIP_ADAPTER_ADDRESSES adap_poll = & ip_adp_address[0];
    DWORD dwRetVal;
    GetAdaptersAddresses (0, 0, NULL, NULL, & outBufLen);
    dwRetVal = GetAdaptersAddresses (0, 0, NULL, adap_poll, &outBufLen);
    assert (dwRetVal == NO_ERROR);
    assert (adap_poll != NULL);
    // search Ethernet network interface. 
    // 
    for (;; adap_poll = adap_poll->Next) {
      if (adap_poll->IfType == IF_TYPE_ETHERNET_CSMACD) {
        int Zcount = 0;
        for (ii = 0; ii != adap_poll->PhysicalAddressLength; ii ++) {
          if (adap_poll->PhysicalAddress[ii] == 0) {
            Zcount ++;
          }
        }
        if (Zcount != adap_poll->PhysicalAddressLength) {
          // record current device's MAC address.
          // record current device's FriendlyName
          assert (adap_poll->PhysicalAddressLength == 6);

          memcpy (& ntcc_cod[0], & adap_poll->PhysicalAddress[0], adap_poll->PhysicalAddressLength);
          memcpy (& fd_buf[0], & adap_poll->FriendlyName[0], 
               (_tcslen (& adap_poll->FriendlyName[0]) + 1) * sizeof (TCHAR) );
          break;
        }
      }
    }

    DialogBox (GetModuleHandle (NULL), MAKEINTRESOURCE (IDDI_LICENCE), NULL, licence_callback);

  } else {
    // other error.
    assert (0);
  }
  return pass_check; 
}