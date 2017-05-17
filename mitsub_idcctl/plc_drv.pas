 (* Copyright (c) 2017 moecmks(徐诩), 徐志波 Changzhou Gauss Electric Co., Ltd.
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
  * --------------------------------------------------------------------------------------------------------
  * PLC Dynamic setting cutting program, IDE:GxWorks2 program:ST lanuage[same as pascal], 
  * need to cooperate with the host computer  
  *
  * Global symbol management table
  * --------------------------------------------------------------------------------------------------------
  * VAR_GLOBAL	D0_sd	Double Word[Signed]		D0	%MD0.0	
  * VAR_GLOBAL	D0_ud	Double Word[Unsigned]/Bit STRING[32-bit]		D0	%MD0.0	
  * VAR_GLOBAL	D1000_uda	Double Word[Unsigned]/Bit STRING[32-bit](0..399)		D1000	%MD0.1000	
  * VAR_GLOBAL	D1000_sda	Double Word[Signed](0..399)		D1000	%MD0.1000	
  * VAR_GLOBAL	D10_sd	Double Word[Signed]		D10	%MD0.10	
  * VAR_GLOBAL	D10_ud	Double Word[Unsigned]/Bit STRING[32-bit]		D10	%MD0.10	
  * VAR_GLOBAL	D12_sd	Double Word[Signed]		D12	%MD0.12	
  * VAR_GLOBAL	D12_ud	Double Word[Unsigned]/Bit STRING[32-bit]		D12	%MD0.12	
  * VAR_GLOBAL	D2_sd	Double Word[Signed]		D2	%MD0.2	
  * VAR_GLOBAL	D2_ud	Double Word[Unsigned]/Bit STRING[32-bit]		D2	%MD0.2	
  * VAR_GLOBAL	M0_swa	Bit(0..15)		M0	%MX0.0	
  * VAR_GLOBAL	M392_swa	Bit(0..15)		M392	%MX0.392	
  * VAR_GLOBAL	D0_swa	Word[Signed](0..7)		D0	%MW0.0	
  * VAR_GLOBAL	D144_swa	Word[Signed](0..7)		D144	%MW0.144	
  * VAR_GLOBAL	D136_sd	Double Word[Signed]		D136	%MD0.136	
  * VAR_GLOBAL	D136_ud	Double Word[Unsigned]/Bit STRING[32-bit]		D136	%MD0.136	
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
  * M8:assert host accept reset for M7
  * M9:assert waiting for delayed IO scan to complete        
  * M11:assert plc action completion indication 
  * M12:assert host action completion indication 
  * M13:assert cut or punch (FALSE:punch TRUE:cut)
  * M16:assert host send file IO tranlate event.
  * M17:assert host recv file IO tranlate event.
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
  *
  * T0-bit/word:punch-down timing[100ms/per]
  * T1-bit/word:punch-up timing[100ms/per]
  * T2-bit/word:cut-down timing[100ms/per] (An output can be, because the cylinder is not connected to the pump.. )
  * 
  * When the host interrupt occurs, the timer data will be cleared
  *
  * V0:indexed use
  *)
  
IF M8000 AND M386 THEN (* BTL0-ROOT - Boot and init timing and misc param ???*)

   IF M1 THEN (* BTT-L0 no interrupt happen ??? *)
   
     IF M0 AND M385 THEN (* BTT-L0-L0 sweep loop  *)
     
        (* REF (TRUE, X0, 1); *)  (* Immediate refresh IO in current cycle *)
        OUT_C (TRUE, CC235, K99999999); (* Flush timer *)

        IF M3 THEN       (* Poll timing punch-down*)
           OUT_T (TRUE, TC0, D2);
        ELSIF M4 THEN    (* Poll timing punch-up*)
           OUT_T (TRUE, TC1, D3);
        END_IF;

        IF M5 THEN       (* Poll timing cut-down*)
           OUT_T (TRUE, TC2, D4);
        END_IF;
        
        IF M3 AND (T0 = D2) THEN (* Close pucnh-down, open punch-up  *)
           
           M3 := FALSE;
           M4 := TRUE;    
           T0 := 0;
           
           OUT (FALSE, Y0);
           OUT (TRUE, Y1);
           
           OUT_T (FALSE, TC0, D2);
           OUT_T (TRUE, TC1, D3);
        ELSIF M4 AND (T1 = D3) THEN (* Close pucnh-up  *)
           M4 := FALSE;     
           T1 := 0;
           D136 := D136 + 1;
           
           OUT_T (FALSE, TC1, D3);
           OUT (FALSE, Y1);
        END_IF;
        
        IF M5 AND (T2 = D4) THEN (* Close cut-down  *)
           M5 := FALSE;
           D136 := D136 + 1;
           
           OUT_T (FALSE, TC2, D4);
           OUT (FALSE, Y3);
        END_IF;

        IF M9 THEN (*  Scan completion >???  *)
          IF      (M3 = FALSE) 
              AND (M4 = FALSE) 
              AND (M5 = FALSE)
          THEN  
              M2 := FALSE;
              M1 := FALSE; (* interrupt !! *)
              M9 := FALSE;
              M16 := FALSE;
              M17 := FALSE;               
              D136 := D5 - 1;
              M7 := TRUE;
          END_IF;
        ELSIF C235 > D0_sd THEN (* Poll pulse main *)
          IF M13 THEN
             OUT (TRUE, Y3); (* drive cut-timing. *)
             OUT_T (TRUE, TC2, D4);
			 REF (TRUE, Y3, 1); (* Immediate refresh IO in current cycle *)
             SET (TRUE, M5);
          ELSE
             OUT (TRUE, Y0);(* drive punch-down. *)
             OUT_T (TRUE, TC0, D2);
			 REF (TRUE, Y0, 1); (* Immediate refresh IO in current cycle*)
             SET (TRUE, M3);
          END_IF;
          
          IF D6 >= (D5 - 1) THEN
            M9 := TRUE;
            C235 := D0_sd;
          ELSE
            D6 := D6 + 1;
            V0 := D6;
            (* load next value. **)
            BMOV (TRUE, D1000_uda[V0], 2, D0_sd);
            DAND (TRUE, D0_sd, H80000000, D10_sd);
            DAND (TRUE, D0_sd, H7FFFFFFF, D0_sd);
            M13:= DINT_TO_BOOL (D10_sd);
          END_IF;  
        END_IF;(* !! Scan completion >???  *)
     ELSIF (NOT M0) AND M385 THEN (* BTT-L0-L1 initialization PLC internal data !!! *)
       (* 
        * Boot or first sweep.
        * to cancel the drive M8235 again to increase the pulse mode
        * close all output points. [FX1S-14mr]
        * file chunk number: 2[500 * point], EEPROM not battery.
        *)
        
        RST (TRUE, C235); 
        OUT_C (FALSE, CC235, K99999999);
        OUT (FALSE, M8235); (* Pulse increment mode *)
        C235 := 0; (* Reset pulse counter *)

        (* Copy to normal variable area *)
        BMOV (TRUE, D136, 8, D0);
        
        (* Reset all output *)
        OUT (FALSE, Y0);
        OUT (FALSE, Y1);
        OUT (FALSE, Y2);
        OUT (FALSE, Y3);
        OUT (FALSE, Y4);
        OUT (FALSE, Y5);  
        
        (* close t timing and it's coils *)
        OUT_T (FALSE, TC0, D2);
        OUT_T (FALSE, TC1, D3);
        OUT_T (FALSE, TC2, D4); 
        
        IF NOT M384 THEN
          (* First load register from FILE IO.*)
          BMOV (TRUE, D1000_uda[0], 2, D0_sd);
          DAND (TRUE, D0_sd, H80000000, D10_sd);
          DAND (TRUE, D0_sd, H7FFFFFFF, D0_sd);
          M13:= DINT_TO_BOOL (D10_sd);
        ELSE 
         (* 
          * Recovery from abnormal interrupt, reset scanline,
          * adjust poll scanline.
          *)
          IF D136 = HFFFF THEN
             D6 := 0;
          ELSE 
             D6 := D136;
          END_IF;
          
          V0 := D6;
          M0 := TRUE;
          
          (* First load register from FILE IO.*)
          BMOV (TRUE, D1000_uda[V0], 2, D0_sd);
          DAND (TRUE, D0_sd, H80000000, D10_sd);
          DAND (TRUE, D0_sd, H7FFFFFFF, D0_sd);
          M13:= DINT_TO_BOOL (D10_sd);
          
          (* Reset pulse counter *)
          IF NOT (D6 = 0) THEN 
             V0 := D6 - 1;
             BMOV (TRUE, D1000_uda[V0], 2, D10_sd);
             DAND (TRUE, D10_sd, H7FFFFFFF, D10_sd);
             C235 := D10_sd;
          END_IF;
        END_IF;
        
        M0 := TRUE; (* Initialize reset, block the next entry into this BTTanch until PLC restart *)
        M2 := TRUE;
        M10 := TRUE;
       
        M384 := TRUE; (* set exception flag*)
        M11 := TRUE;
     END_IF;
   ELSIF M16 THEN (* BTT-L1 start send file IO tranlate event ??? *)
    (* 
     * host database transfer, it is regrettable that the MITSUBISHI PLC. 
     * can not be used to load the static way to write the file register area by RS422/RS232, 
     * or limited to my level and knowledge. I can't do this. Only the dynamic loading. 
     * 
     * before into this step, host must do:
     *
     * reset M18/M11/M385
     * set M12/M16
     *
     * D10-dword - data temp.
     * D12-word - index.
     *)
     IF M18 THEN 
      (* 
       * IO tranlate complete. set M385.
       *)
       M385 := TRUE;
       M16 := FALSE;
       M18 := FALSE;
       M11:= TRUE;
     ELSIF M12 THEN 
      (* 
       * pick IO tranlate value. write into buffer.
       *)
       V0 := D12;
       BMOV (TRUE, D10_sd, 2, D1000_uda[V0]);
    
       M12:= FALSE;
       M11:= TRUE;
     END_IF;
   ELSIF M17 THEN (* BTT-L2 start recv file IO tranlate event ??? *)
    (* 
     * plc database transfer, it is regrettable that the MITSUBISHI PLC. 
     * can not be used to load the static way to write the file register area by RS422/RS232, 
     * or limited to my level and knowledge. I can't do this. Only the dynamic loading. 
     * 
     * before into this step, host must do:
     *
     * reset M18/M11
     * set M12/M17
     *
     * D10-dword - data temp.
     * D12-word - index.
     *)
     IF M18 THEN 
      (* 
       * IO tranlate complete. 
       *)
       M17 := FALSE;
       M18 := FALSE;
       M11:= TRUE;
     ELSIF M12 THEN 
      (* 
       * pick IO tranlate value. write into temp.
       *)
       V0 := D12;
       BMOV (TRUE, D1000_uda[V0], 2, D10_sd);
    
       M12:= FALSE;
       M11:= TRUE;
     END_IF;
   ELSIF M8 THEN (* BTT-L3 Current processing complete *)
     M384 := FALSE;
     
     D136 := 0;
     D142 := 0;
     
     M8 := FALSE;
     M12:= FALSE;
     M11:= TRUE;
   END_IF;
END_IF;
