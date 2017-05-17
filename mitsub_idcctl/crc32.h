/*-
 * Copyright (c) 2017 moecmks(ÐìÚ¼), ÐìÖ¾²¨ Changzhou Gauss Electric Co., Ltd.
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
 
#if !defined (crc32_iccs8_included_MOECMKS)
#define crc32_iccs8_included_MOECMKS

/* 
 * Simple CRC32 correction routine
 * 
 * XXX:
 * If CPU supports the CRC assembly instruction of SSE, 
 * it will use iSCSI (Castagnoli) polynomial, 
 * it's not the same as the standard CRC32 table[such as Ethernet/ZIP], please note
 *
 * Thanks stackoverflow's Mark Adler
 * Reference:http://stackoverflow.com/questions/29174349/mm-crc32-u8-gives-different-result-than-reference-code
 */
#if defined (__cplusplus)  /** __cplusplus */
extern "C" {
#endif  /** __cplusplus */

/* Portable fixed length ***/
#include "stdint.h"

uint32_t crc32_calc (uint32_t init, void *adjust_buf, int32_t bnums);      
                         
#if defined (__cplusplus)  /** __cplusplus */
}
#endif  /** __cplusplus */

#endif /* crc32_iccs8_included_MOECMKS */