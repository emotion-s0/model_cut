/*-
 * Copyright (c) 2017 moecmks(徐诩), 徐志波 Changzhou Gauss Electric Co., Ltd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRCMD, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
 
#include <assert.h>
#include <string.h>
#include "host_drv.h"

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


/* XXX:L-endian.
*/

static 
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

static 
uint8_t fxcrc_adjust (unsigned char *crcbuf, uint32_t num) {
  
  uint32_t s= 0;
  uint32_t st = 0;
  
  for (; s != num; s++)
    st += crcbuf[s];
  /* we only save lowest bit's byte **/
  return st & 0xFF; 
}

static /* we not check numb cross register **.**/
int fxcalc_addru (enum FX1S_REGISTER_FIELD reg, uint16_t addr, 
                           uint16_t  *opbsize,
                                uint16_t *raddr, unsigned char *dboff) {
  
  uint16_t addr0 = 0x00A0;
  uint16_t eig = vailed8 (addr);
  uint8_t off = -1;
  uint32_t opbsize0 = 2;

  switch (reg) {
    
  case FX1S_REGISTER_FIELD_D: 
  
    if (addr <= 127 && addr == addr) /* numbers: 128, normal use */
      addr0 = 0x1000 + addr * 2;
    else if (addr <= 255) /* numbers: 128, save use */
      addr0 = 0x1000 + addr * 2;
    else if (addr >= 1000 && addr <= 2499)  /* numbers: 1500, file register. in fact, can't use */
      addr0 = 0x1000 + addr * 2;
    else if (addr >= 8000 && addr <= 8255) /* numbers: 256, special IO port */
      addr0 = 0x0E00 + (addr - 8000) * 2;
    else  /* Illegal access */
      return FX1S_RANGE;
    break;

  case FX1S_REGISTER_FIELD_X:

    /*
     * Check the number of available X-coils according to the PLC version 
     */
    eig = vailed8 (addr);
    addr0 = 0x0080 + eig / 8;
    off = eig & 7;
    
    opbsize0 = 1;
    break;
    
  case FX1S_REGISTER_FIELD_Y_PLS:
    addr0 += 0x0200;
  case FX1S_REGISTER_FIELD_Y_OUT:
  
    /*
     * Check the number of available Y-coils according to the PLC version 
     */
    if ((eig = vailed8 (addr)) == -1)
      return FX1S_PARA;
    
    addr0 += eig / 8;
    off = eig & 7;
    
    opbsize0 = 1;
    break;

  case FX1S_REGISTER_FIELD_S:
  
    if ((addr >=  128)) /* numbers:128, status register **/
      return FX1S_RANGE;

    addr0 = addr / 8;
    off = addr & 7;
    
    opbsize0 = 1;
    break;
  
  case FX1S_REGISTER_FIELD_T:
  
    if ( (addr <=  63)) /* numbers:64, 100ms or 10ms M8028/D8030/D8031 **/
      addr0 = 0x0800 + addr * 2;
    else     
      return FX1S_RANGE;
    break;
    
  case FX1S_REGISTER_FIELD_M:
  
    if (addr < 384) /* numbers: 384, normal use */
      addr0 = 0x0100 + addr / 8;
    else if (addr < 512) /* numbers: 512, save use */
      addr0 = 0x0100 + addr / 8;
    else if (addr >= 8000 && addr < 8256) /* numbers: 256, special IO port */
      addr0 = 0x01E0 + (addr - 8000) / 8;
    else /* Illegal access */
      return FX1S_RANGE;
    
    off = addr & 7;
    opbsize0 = 1;
    break;
  
  case FX1S_REGISTER_FIELD_C16:
  
    if (addr < 16) /* numbers: 16, normal use */
      addr0 = 0x0A00 + addr * 2;
    else if (addr < 32) /* numbers: 16, save use */
      addr0 = 0x0A00 + addr * 2;
    else /* Illegal access */
      return FX1S_RANGE;
    break;
    
  case FX1S_REGISTER_FIELD_C32:
  
   /* for C32 high speed registers, 
    * we only perform some basic checks, please note
    **/
    if (addr > 200 && addr <= 255)
      addr0 = 0x0C00 + (addr - 200) * 4;
    else /* Illegal access */
      return FX1S_RANGE;
      
    opbsize0 = 4;
    break;

  case FX1S_REGISTER_FIELD_C_03C0:
  
    if (addr <= 255)
      addr0 = 0x03C0 + addr / 8;
    else /* Illegal access */
      return FX1S_RANGE;
      
    opbsize0 = 1;
    break;
    
  default:
      return FX1S_PARA;
  }
  
  *raddr = addr0;
  *dboff = off;
  *opbsize = opbsize0;
  return FX1S_OK;
}

int fx1s_makersec (struct read_section2 *rsec, /* write to the serial port, use the size of the read_section */
                         enum FX1S_REGISTER_FIELD rf, 
                                    uint8_t read_nums, /* unit/per. e.g. word, and dword, for bit-reg, treat it as byte **/
                     uint16_t  *rvap_size, uint16_t address)
{
  struct read_section2 sec;
  int e;
  
  /** phase 1:fill stdhead/stdend flags and cmd, rread count,s */
  sec.stx = SECTION_LINK_STX;
  sec.etx = SECTION_LINK_ETX;
  sec.cmd = SECTION_CMD_READ;

  /** phase 2:calc address for register and current PLC version */
  e = fxcalc_addru (rf, address, & sec.opbsize, & sec.opbaddr, & sec.opboff);
  if (e != FX1S_OK) 
    return e;
  else 
   *rvap_size = sizeof (sec.stx) +
                sizeof (sec.crc)+ sizeof (sec.etx) + sec.opbsize * 2 * read_nums;

  /** phase 3:fill numb ascii, * */
  sec.numb[0] = num_to_ascii ( (sec.opbsize * read_nums  & 0xF0) >>4);
  sec.numb[1] = num_to_ascii ( (sec.opbsize * read_nums  & 0x0F) >>0);
  
  /** phase 4:fill address ascii, * */
  sec.unit_address[0] = num_to_ascii ( (sec.opbaddr  & 0xF000) >>12);
  sec.unit_address[1] = num_to_ascii ( (sec.opbaddr  & 0x0F00) >> 8);
  sec.unit_address[2] = num_to_ascii ( (sec.opbaddr  & 0x00F0) >> 4);
  sec.unit_address[3] = num_to_ascii ( (sec.opbaddr  & 0x000F) >> 0);
  
  /** phase 5:crc adjust, fill ascii buf * */
  sec.crce = fxcrc_adjust (& sec.cmd, sizeof (sec.cmd) + sizeof (sec.unit_address) 
                                        + sizeof (sec.numb) 
                                        + sizeof (sec.etx));
  sec.crc[0] = num_to_ascii ( (sec.crce  & 0xF0) >> 4);
  sec.crc[1] = num_to_ascii ( (sec.crce  & 0x0F) >> 0);
  
  memcpy (rsec, & sec, sizeof (sec));
  return FX1S_OK;  
}

int fx1s_makewsec (void *wsec, /* Variable size structure, so use void *, please understand **/
                   void *wbbuf, enum FX1S_REGISTER_FIELD rf, 
                         uint8_t write_nums, /* unit/per. e.g. word, and dword, for bit-reg, treat it as byte **/
                         uint16_t  *wsec_size,  uint16_t address)
{
  uint16_t opbsize, opbaddr;
  char obpoff;
  char varsbuf[256];
  char *as = wbbuf, cs;
  uint32_t e;
  uint32_t s = 0;
  struct write_section *secp = wsec;
  struct write_section *secdp = (void *)varsbuf;

  /** phase 1:fill stdhead flags and cmd */
  secdp->stx = SECTION_LINK_STX;
  secdp->cmd = SECTION_CMD_WRITE;

  /** phase 2:calc address for register and current PLC version */
  e = fxcalc_addru (rf, address,  & opbsize, & opbaddr, & obpoff);
  if (e != FX1S_OK) 
    return e;
  else 
    *wsec_size = sizeof (struct write_section) + opbsize * 2 * write_nums;

  /** phase 3:fill numb ascii, * */
  secdp->numb[0] = num_to_ascii ( (opbsize * write_nums  & 0xF0) >>4);
  secdp->numb[1] = num_to_ascii ( (opbsize * write_nums  & 0x0F) >>0);
  
  /** phase 4:fill address ascii, * */
  secdp->unit_address[0] = num_to_ascii ( (opbaddr  & 0xF000) >>12);
  secdp->unit_address[1] = num_to_ascii ( (opbaddr  & 0x0F00) >> 8);
  secdp->unit_address[2] = num_to_ascii ( (opbaddr  & 0x00F0) >> 4);
  secdp->unit_address[3] = num_to_ascii ( (opbaddr  & 0x000F) >> 0);
  
  /** phase 5:fill variable buffer, * */
  for ( ; s != opbsize *  write_nums; s++) {
    unsigned char  temp = as[s];
    char  tmphi = num_to_ascii (temp >> 4);
    char  tmplo = num_to_ascii (temp & 15);
    
    secdp->numb[2+s*2+0] = tmphi;
    secdp->numb[2+s*2+1] = tmplo;
  }

  /** phase 6:crc adjust, fill ascii buf * */
  secdp->numb[2+opbsize*  write_nums *2] = SECTION_LINK_ETX;
  
  cs = fxcrc_adjust (& secdp->cmd, opbsize * 2 * write_nums + sizeof (secp->cmd) + sizeof (secp->unit_address) 
                                        + sizeof (secp->numb) 
                                        + sizeof (secp->etx));
  secdp->numb[2+opbsize*2*  write_nums+1] = num_to_ascii ( (cs  & 0xF0) >> 4);
  secdp->numb[2+opbsize*2*  write_nums+2] = num_to_ascii ( (cs  & 0x0F) >> 0);
  
  memcpy (wsec, & varsbuf, *wsec_size);
  return FX1S_OK; 
}


int fx1s_makebuf (void *encbuf, void *decbuf, uint32_t enc_max, uint32_t dec_max,
             uint8_t enc_nums) /* byte/per **/  
{
  char *encbuf0 = encbuf;
  char *decbuf0 = decbuf;
  uint32_t ii;

  assert (decbuf != NULL);
  assert (encbuf != NULL);
  assert (enc_max != 0);
  assert (dec_max != 0);
  assert (enc_nums != 0);
  assert (enc_nums >= dec_max);

  /* phase 1 :scan buf find SECTION_LINK_STX ***/ 
  for (ii = 0; ii != enc_max; ii++) 
  {
     if (encbuf0[ii] == SECTION_LINK_STX)
      {
        /* phase 2:start current pos **/
         uint32_t crc_nums = enc_nums * 2 + 1 + 1 + 2; /* 1:STX, 1:ETX, 2:CRC **/
         uint32_t s;
         uint8_t crc, crc0; /* current encode mode, \
                           STX [byte0-hi byte0-lo ... byten-hi byten-lo] ETX CRC-HI CRC-LO 
                    */
         assert ((ii + crc_nums ) < enc_max);

         /* phase 3:judge CRC [STX + enc_nums * 2] **/
         crc = fxcrc_adjust (& encbuf0[ii+1], 1 + enc_nums * 2);
         crc0 =  ascii_to_num ( encbuf0[ii+1+1+ enc_nums * 2+0] ) << 4;
         crc0 |=  ascii_to_num ( encbuf0[ii+1+1+ enc_nums * 2+1]) ;
         if (crc0 != crc)
           return FX1S_INCOP;
         else 
          {
             /** phase 4:fill variable buffer, * */
             for ( s = 0; s != enc_nums * 2; s+=2) {
           
               unsigned char  obt;
               obt =  ascii_to_num ( encbuf0[ii+1+s]) << 4;
               obt |=  ascii_to_num ( encbuf0[ii+1+s+1] );
     
               decbuf0[s >> 1] = obt;
             }
            return FX1S_OK;
          }
      }
  }
 
  /* phase 5 :scan buf find SECTION_LINK_NAK ***/ 
  for (ii = 0; ii != enc_max; ii++) {
    if (encbuf0[ii] == SECTION_LINK_NAK)
    {
      return FX1S_NAK;
    }
  }

  return FX1S_NFIND;
}









/* Stupid attempt! **/
uint32_t fx1s_cmprvpack (void *raccbuf, /* Variable size structure, so use void *, please understand **/
                         uint16_t rc, void **ascii_buf, uint16_t *opbsize
                         , uint16_t *stdpos)
{
  char *varsbuf = raccbuf;
  uint16_t c = 0;
  char stx_find = 0;
  uint16_t stdpos0 = -1;
  
  /* we find SECTION_LINK_NAK or SECTION_LINK_STX at first **/
  for (; c != rc; c++)
   {
     if (varsbuf[c] == SECTION_LINK_NAK)
       return FX1S_NAK;
     if (varsbuf[c] == SECTION_LINK_STX)
      {
        /* second, we check SECTION_LINK_ETX in buffer **/
        stx_find = 1;
        stdpos0 = c + 1;
      }  
     if (varsbuf[c] == SECTION_LINK_ETX && stx_find == 1)
      {
        /* exist CRC byte ??**/
        if ((c + 2) >= rc)
          return FX1S_INCOP;
        /* calculate, compare the CRC code **/
        {
      # if 0
      # else 
          *ascii_buf = & varsbuf[stdpos0];
          *opbsize = c - stdpos0;
          *stdpos = stdpos0;
          return FX1S_OK;
      # endif    
        }
      }
   }
   
   return FX1S_INCOP;
}                     

uint32_t fx1s_decrvsec (void *raccbuf, void *sbuf, uint16_t opbasize) {
  
  char *varsbuf = raccbuf;
  char *ssbuf = sbuf;
  uint16_t c = 0;
  
  if (opbasize % 2 == 1)
    return FX1S_INCOP;
  if (opbasize == 0)
    return FX1S_PARA;
  
  for ( ; c != opbasize; c += 2)
    {
      char tmphi = ascii_to_num (varsbuf[c]) << 4;
      char tmplo = ascii_to_num (varsbuf[c+1]);   
      char temp  =   (tmphi & 0xF0) |    (tmplo & 0x0F);
      
      ssbuf[c>>1] = temp;
    }
    
    return FX1S_OK;
}