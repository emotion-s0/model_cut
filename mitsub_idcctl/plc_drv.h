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
 
#if !defined (plc_drv_included_MOECMKS)
#define plc_drv_included_MOECMKS

/* 
 * ---------------------------------------------------------------------------------------------------------
 * IO-dispatch 
 * ---------------------------------------------------------------------------------------------------------
 * Y0 <- punch down
 * Y1 <- punch up 
 * Y2 <- no use.
 * Y3 <- cut down
 * Y4 <- no use
 * Y5 <- alert alarm
 * Ynnn ... <- no use.
 * ---------------------------------------------------------------------------------------------------------
 * X0 <- OMRON high speed pulse encoder [One-way increment count, only link phase A, discard phase B and Z]
 * Xnnn ... <- no use.
 * ---------------------------------------------------------------------------------------------------------
 * M0:assert initialization PLC internal data
 * M1:assert host no interrupt 
 * M2:assert host link.
 * M3:assert punch-down timing 
 * M4:assert punch-up timing 
 * M5:assert cut-down timing 
 * M7:assert current processing completion 
 * M8:assert user can continue next batch processing 
 * M9:assert waiting for delayed IO scan to complete        
 * M11:assert plc action completion indication 
 * M12:assert host action completion indication 
 * M13:assert cut or punch (FALSE:punch TRUE:cut)
 * M14:assert start new pass next batch processing 
 * M16:assert host send file IO tranlate event.
 * M18:assert file IO tranlate complete.
 *
 * M384:
 *     assert abnormal exit ,
 *        such as equipment manufacturers sudden power trip, 
 *           the need to maintain power and resume/host interrupt reset scanline.
 * M385:
 *     assert init list group data.
 * M386:
 *     assert init timing and misc params.
 *
 * D0-dword:pulse count for current file register settings
 * D2-word:punch-down timing [100ms/per]
 * D3-word:punch-up timing [100ms/per]
 * D4-word:cut-down timing [100ms/per]
 * D5-word:total scan line
 * D6-word:poll scan line.
 * D10-dword:for file IO tranlate value.
 *
 * D136-word:host computer interrupt/except recovery scanline. 
 * D138-word:punch-down timing [100ms/per]
 * D139-word:punch-up timing [100ms/per]
 * D140-word:cut-down timing [100ms/per]
 * D141-word:total scan line 
 * D142-word:poll scan line.
 * D143-word:per mm.
 * D144-word:per pulse.
 * D145-word:cut off
 * D146-word:cut corrent.
 * D147-word:punch corrent.
 */
 
#if defined (__cplusplus)  /** __cplusplus */
extern "C" {
#endif  /** __cplusplus */
  
/* PLC output distribution */

#define PUNCH_IO_DOWN 0 
#define PUNCH_IO_UP 1
#define CUT_IO_DOWN 3
#define USELESS_IO 2
#define ALERT_IO_TRIGGER 4 
#define USELESS_IO2 5 

/* PLC output mask */

#define PUNCH_IO_DOWN_MASK 1 
#define USELESS_IO_MASK 2
#define CUT_IO_DOWN_MASK 8
#define CUT_IO_UP_MASK 4
#define ALERT_IO_TRIGGER_MASK 16
#define USELESS_IO2_MASK 32

/* Function code */

#define FUNC_CODE_PUNCH 0 
#define FUNC_CODE_CUT 1 

/* Function code mask */

#define FUNC_CODE_PUNCH_MASK 0 
#define FUNC_CODE_CUT_MASK 0x80000000

/* PLC pulse encoder address */

#define PULSE_ENCODER_ADDR 235 

/* PLC and host condition mask */

#define PLC_MASSERT_INITIALIZATION_INTERNAL 0 
#define PLC_MASSERT_HOST_NO_INTERRUPT 1
#define PLC_MASSERT_HOST_LINK 2
#define PLC_MASSERT_PUNCH_DOWN_TIMING 3
#define PLC_MASSERT_PUNCH_UP_TIMING 4
#define PLC_MASSERT_CUT_DOWN_TIMING 5
#define PLC_MASSERT_PROCESSING_COMPLETION  7
#define PLC_MASSERT_CONTINUE_NEXT_PROCESS  8
#define PLC_MASSERT_DIO_SCAN_COMPLETE 9
#define PLC_MASSERT_PLC_COMPLETION  11
#define PLC_MASSERT_HOST_COMPLETION 12
#define PLC_MASSERT_CUT_OR_PUNCH_CURRENT 13
#define PLC_MASSERT_CUT_OWN_TIMING 14
#define PLC_MASSERT_EVENT_SEND_FIO 16
#define PLC_MASSERT_EVENT_RECV_FIO 17
#define PLC_MASSERT_FIO_TRANSLATE_COMPLETION 18
#define PLC_MASSERT_ABNORMAL_EXIT 384
#define PLC_MASSERT_INIT_LIST_GROUP 385
#define PLC_MASSERT_INIT_TIMING 386

/* PLC and host data */

#define PLC_DDCURRENT_PULSE_COUNT 0 
#define PLC_DWCURRENT_PUNCH_DOWN_TIMING 2
#define PLC_DWCURRENT_PUNCH_UP_TIMING 3
#define PLC_DWCURRENT_CUT_DOWN_TIMING 4
#define PLC_DWCURRENT_TOTAL_SCANLINE 5
#define PLC_DWCURRENT_POLL_SCANLINE 6
#define PLC_DWCURRENT_IO_TRANSLATE 10
#define PLC_DWCURRENT_IO_TRANSLATE_ADDRESS 12
#define PLC_DWSTORGE_INTERRUPT_SCANLINE 136
#define PLC_DWSTORGE_PUNCH_DOWN_TIMING 138
#define PLC_DWSTORGE_PUNCH_UP_TIMING 139
#define PLC_DWSTORGE_CUT_DOWN_TIMING 140
#define PLC_DWSTORGE_TOTAL_SCANLINE 141
#define PLC_DWSTORGE_POLL_SCANLINE 142
#define PLC_DWSTORGE_PER_MM 143
#define PLC_DWSTORGE_PER_PULSE 144
#define PLC_DWSTORGE_CUT_OFFSET 145
#define PLC_DWSTORGE_CUT_CORRENT 146
#define PLC_DWSTORGE_PUNCH_CORRENT 147

#if defined (__cplusplus)  /** __cplusplus */
}
#endif  /** __cplusplus */

#endif /* plc_drv_included_MOECMKS */