S_f_offset dw NUM_SLOTS dup (?)
S_f_counter dw NUM_SLOTS dup (?)
S_f_val dw NUM_SLOTS dup (?)
S_f_increment dw NUM_SLOTS dup (?)

S_v0_offset dw NUM_SLOTS dup (?)
S_v0_counter dw NUM_SLOTS dup (?)
S_v0_val dw NUM_SLOTS dup (?)
S_v0_increment dw NUM_SLOTS dup (?)

S_v1_offset dw NUM_SLOTS dup (?)
S_v1_counter dw NUM_SLOTS dup (?)
S_v1_val dw NUM_SLOTS dup (?)
S_v1_increment dw NUM_SLOTS dup (?)

S_p_offset dw NUM_SLOTS dup (?)
S_p_counter dw NUM_SLOTS dup (?)
S_p_val dw NUM_SLOTS dup (?)
S_p_increment dw NUM_SLOTS dup (?)

S_fb_offset dw NUM_SLOTS dup (?)
S_fb_counter dw NUM_SLOTS dup (?)
S_fb_val dw NUM_SLOTS dup (?)
S_fb_increment dw NUM_SLOTS dup (?)

S_m0_offset dw NUM_SLOTS dup (?)
S_m0_counter dw NUM_SLOTS dup (?)
S_m0_val dw NUM_SLOTS dup (?)
S_m0_increment dw NUM_SLOTS dup (?)

S_m1_offset dw NUM_SLOTS dup (?)
S_m1_counter dw NUM_SLOTS dup (?)
S_m1_val dw NUM_SLOTS dup (?)
S_m1_increment dw NUM_SLOTS dup (?)

S_ws_offset dw NUM_SLOTS dup (?)
S_ws_counter dw NUM_SLOTS dup (?)
S_ws_val dw NUM_SLOTS dup (?)
S_ws_increment dw NUM_SLOTS dup (?)


;This struct defines the non-variable length portion of a TVFX definition.
;It's invariably followed by chunks of data specifying time-variant changes to make to the MIDI/OPL controls.
TVFX            STRUC
T_length        dw ? ; 00,01
T_transpose     db ? ; 02
T_type          db ? ; 03 1 = TV_inst, 2 = TV_effect?
T_duration      dw ? ; 04, 05

T_init_f_val    dw ? ; 06, 07
T_init_f_offset dw ? ; 08, 09
T_f_rel_offset  dw ? ; 0A, 0B
T_init_v0_val    dw ?; 0C, 0D
T_init_v0_offset dw ?; 0E, 0F
T_v0_rel_offset  dw ?; 10, 11
T_init_v1_val    dw ?; 12, 13
T_init_v1_offset dw ?; 14, 15
T_v1_rel_offset  dw ?; 16, 17
T_init_p_val    dw ? ; 18, 19
T_init_p_offset dw ? ; 1A, 1B
T_p_rel_offset  dw ? ; 1C, 1D
T_init_fb_val    dw ?; 1E, 1F
T_init_fb_offset dw ?; 20, 21
T_fb_rel_offset  dw ?; 22, 23
T_init_m0_val    dw ?; 24, 25
T_init_m0_offset dw ?; 26, 27
T_m0_rel_offset  dw ?; 28, 29
T_init_m1_val    dw ?; 2A, 2B
T_init_m1_offset dw ?; 2C, 2D
T_m1_rel_offset  dw ?; 2E, 2F
T_init_ws_val    dw ?; 30, 31
T_init_ws_offset dw ?; 32, 33
T_ws_rel_offset  dw ?; 34, 35

;These next 8 fields are present in some TVFX structs, but in others, default values are used.
;The only way to differentiate is with the T_init_f_offset value. It is 34h if these values are missing and 3Ch if they're present.
T_play_ad_1     db ? ; 36
T_play_sr_1     db ? ; 37
T_play_ad_0     db ? ; 38
T_play_sr_0     db ? ; 39
T_rel_ad_1      db ? ; 3A
T_rel_sr_1      db ? ; 3B
T_rel_ad_0      db ? ; 3C
T_rel_sr_0      db ? ; 3D
                ENDS



TVFX_switch_voice PROC
                USES ds, si, di
                mov si, 0FFFFh
__next_slot:    inc si
                cmp si, NUM_SLOTS
                jz __exit
                cmp S_status[si], FREE
                jz __next_slot
                cmp S_voice[si], -1
                jnz __next_slot
                mov bx, 0FFFFh

__next_voice:   inc bx
                cmp bx, NUM_VOICES
                jz __exit
                cmp V_channel[bx], -1
                jnz __next_voice
                mov S_voice[si],bl
                mov di, bx
                mov bl, S_channel[si]
                inc MIDI_voices[bx]
                mov V_channel[di],bl
                mov S_update[si], U_ALL_REGS
                call update_priority

__exit:         ret
                ENDP



TVFX_increment_stage PROC slot, memory_element
                USES ds, si, di
                mov si, [slot]
                mov bx, si
                shl bx, 1
                mov ds, S_timbre_seg[bx]
                mov di, S_timbre_off[bx]
                add bx, [memory_element]
                add di, cs:[bx]
                mov cx, 0Ah

loc_367E:       mov ax, [di]
                mov dx, [di+2]
                cmp ax, 0
                jnz __ax_not_0
                add di, dx
                add cs:[bx], dx

__next_loop:    loop loc_367E
                mov WORD PTR cs:[bx + (6 * NUM_SLOTS)], 0        ;bx is large enough that this won't necessarily point to f-increment, but it will always point to *some* increment value
                mov WORD PTR cs:[bx + (2 * NUM_SLOTS)], 0FFFFh     ;Same principle applies here. Disassembled version of the code looks like "mov word ptr cs:[bx+28h], 0FFFFh"
                ret

__ax_not_0:     add di, 4
                add WORD PTR cs:[bx], 4
                cmp ax, -1
                jz __loc_36BE
                cmp ax, -2
                jz __loc_36C4
                mov cs:[bx + (2 * NUM_SLOTS)], ax
                mov cs:[bx + (6 * NUM_SLOTS)], dx
                ret

__loc_36BE:     mov cs:[bx+(4 * NUM_SLOTS)], dx
                jmp __next_loop

__loc_36C4:     mov ax, [memory_element]
                cmp ax, OFFSET S_m0_offset
                jz __update_m0
                cmp ax, OFFSET S_m1_offset
                jz __update_m1
                cmp ax, OFFSET S_v0_offset
                jz __update_v0
                cmp ax, OFFSET S_v1_offset
                jz __update_v1
                cmp ax, OFFSET S_f_offset
                jz __update_freq
                cmp ax, OFFSET S_fb_offset
                jz __update_fb
                jmp __next_loop

__update_m0:    mov S_AVEKM_0[si], dl
                jmp __next_loop

__update_m1:    mov S_AVEKM_1[si], dl
                jmp __next_loop

__update_v0:    mov S_KSLTL_0[si], dl
                jmp __next_loop

__update_v1:    mov S_KSLTL_1[si], dl
                jmp __next_loop

__update_freq:  mov S_BLOCK[si], dh
                cmp S_type[si], TV_INST
                jz __TV_inst
                jmp __next_loop

__TV_inst:      and S_BLOCK[si], 0E0h
                jmp __next_loop

__update_fb:    mov S_FBC[si], dh
                jmp __next_loop
                ENDP



serve_synth     PROC
                USES ds, si, di

                add TV_accum, 60
                cmp TV_accum, 120
                jnb __serve_tvfx

__check_pri_accum:
                add pri_accum, 5
                cmp pri_accum, 120
                jnb __serve_pri

__exit:         ret

__serve_pri:    sub pri_accum, 120
                call update_priority
                jmp __exit

__serve_tvfx:   sub TV_accum, 120
                xor vol_update, U_KSLTL
                mov si, -1

__next_slot:    inc si
                cmp si, NUM_SLOTS
                jz __check_pri_accum
                cmp S_status[si], FREE
                jz __next_slot
                cmp S_type[si], BNK_INST
                jz __next_slot
                mov di, si
                shl di, 1
                mov ax, S_f_increment[di]
                or ax, ax
                jz __zero_f_increment
                add S_f_val[di], ax
                or S_update[si], U_FREQ

__zero_f_increment:
                dec S_f_counter[di]
                jnz __f_counter_is_not_zero
                call TVFX_increment_stage C, si, OFFSET S_f_offset
                or S_update[si], U_FREQ

__f_counter_is_not_zero:
                mov ax, S_fb_increment[di]
                or ax, ax
                jz __zero_fb_increment
                add S_fb_val[di], ax
                or S_update[si], U_FBC

__zero_fb_increment:
                dec S_fb_counter[di]
                jnz __fb_counter_is_not_zero
                call TVFX_increment_stage C, si, OFFSET S_fb_offset
                or S_update[si], U_FBC

__fb_counter_is_not_zero:
                mov ax, S_m0_increment[di]
                or ax, ax
                jz __zero_m0_increment
                add S_m0_val[di], ax
                or S_update[si], U_AVEKM

__zero_m0_increment:
                dec S_m0_counter[di]
                jnz __m0_counter_is_not_zero
                call TVFX_increment_stage C, si, OFFSET S_m0_offset
                or S_update[si], U_AVEKM

__m0_counter_is_not_zero:
                mov ax, S_m1_increment[di]
                or ax, ax
                jz __zero_m1_increment
                add S_m1_val[di], ax
                or S_update[si], U_AVEKM

__zero_m1_increment:
                dec S_m1_counter[di]
                jnz __m1_counter_is_not_zero
                call TVFX_increment_stage C, si, OFFSET S_m1_offset
                or S_update[si], U_AVEKM

__m1_counter_is_not_zero:
                mov ax, S_v0_increment[di]
                or ax, ax
                jz __zero_v0_increment
                mov bx, S_v0_val[di]
                add S_v0_val[di], ax
                cmp S_status[si], KEYOFF
                jnz __update_volume_0
                mov cx, S_v0_val[di]
                mov dx, cx
                xor dx, bx
                jns __update_volume_0
                mov dx, cx
                xor dx, ax
                js __update_volume_0
                mov S_v0_val[di], 0

__update_volume_0:
                mov al, vol_update
                or S_update[si], al

__zero_v0_increment:
                dec S_v0_counter[di]
                jnz __v0_counter_is_not_zero
                call TVFX_increment_stage C, si, OFFSET S_v0_offset
                or S_update[si], U_KSLTL

__v0_counter_is_not_zero:
                mov ax, S_v1_increment[di]
                or ax, ax
                jz __zero_v1_increment
                mov bx, S_v1_val[di]
                add S_v1_val[di], ax
                cmp S_status[si], KEYOFF
                jnz __update_volume_1
                mov cx, S_v1_val[di]
                mov dx, cx
                xor dx, bx
                jns __update_volume_1
                mov dx, cx
                xor dx, ax
                js __update_volume_1
                mov S_v1_val[di], 0

__update_volume_1:
                mov al, vol_update
                or S_update[si], al

__zero_v1_increment:
                dec S_v1_counter[di]
                jnz __v1_counter_is_not_zero
                call TVFX_increment_stage C, si, OFFSET S_v1_offset
                or S_update[si], U_KSLTL

__v1_counter_is_not_zero:
                mov ax, S_ws_increment[di]
                or ax, ax
                jz __zero_ws_increment
                add S_ws_val[di], ax
                or S_update[si], U_WS

__zero_ws_increment:
                dec S_ws_counter[di]
                jnz __ws_counter_is_not_zero
                call    TVFX_increment_stage C, si, OFFSET S_ws_offset
                or S_update[si], U_WS

__ws_counter_is_not_zero:
                mov ax, S_p_increment[di]
                add S_p_val[di], ax
                dec S_p_counter[di]
                jnz __p_counter_is_not_zero
                call TVFX_increment_stage C, si, OFFSET S_p_offset

__p_counter_is_not_zero:
                test S_update[si], U_ALL_REGS
                jz __no_update_necessary
                call update_voice C, si

__no_update_necessary:
                cmp S_status[si], KEYOFF
                jz __key_off
                dec S_duration[di]
                jnz __still_running
                mov S_status[si], KEYOFF
                call TV_phase C, si
                jmp __next_slot

__key_off:      cmp S_v0_val[di], 0400h
                jnb __still_running
                cmp S_v1_val[di], 0400h
                jnb __still_running
                call release_voice C, si
                mov S_status[si], FREE
                call TVFX_switch_voice

__still_running:jmp __next_slot
                ENDP



TV_phase        PROC slot
                USES ds, si, di

                mov si, [slot]
                mov bx, si
                shl bx, 1
                mov di, S_timbre_off[bx]
                mov ds, S_timbre_seg[bx]
                mov al, 0h
                mov S_FBC[si], al
                mov S_KSLTL_0[si], al
                mov S_KSLTL_1[si], al
                mov al, 20h
                mov S_AVEKM_0[si], al
                mov S_AVEKM_1[si], al
                mov cl, 20h
                mov al, [di].T_type
                cmp al, TV_INST
                jz __TV_inst
                or cl, 8

__TV_inst:      mov S_type[si], al
                mov S_BLOCK[si], cl
                mov ax, 0FF0Fh ; Default AD_1 and SR_1 values
                mov dx, 0FF0Fh ; Default AD_0 and SR_0 values
                mov cx, [di].T_init_f_offset
                add cx, 2 ; Offsets never include the "size" data at the top of the struct
                cmp cx, 36h ; If the first chunk of update data is at offset 36h, then we use the default AD/SR values
                jz __use_default
;                mov ah, [di].T_play_ad_1 ;Values to use if KEYON is still true
;                mov al, [di].T_play_sr_1 ;Values to use if KEYON is still true
                mov ax, WORD PTR [di].T_play_ad_1
;                mov dh, [di].T_play_ad_0
;                mov dl, [di].T_play_sr_0
                mov dx, WORD PTR [di].T_play_ad_0
                cmp S_status[si], KEYOFF
                jnz __use_default
;                mov ah, [di].T_rel_ad_1 ;Values to use if KEYOFF is true
;                mov al, [di].T_rel_sr_1 ;Values to use if KEYOFF is true
                mov ax, WORD PTR [di].T_rel_ad_1
;                mov dh, [di].T_rel_ad_0
;                mov dl, [di].T_rel_sr_0
                mov dx, WORD PTR [di].T_rel_ad_0

__use_default:  mov S_AD_0[si], dh
                mov S_SR_0[si], dl
                mov S_AD_1[si], ah
                mov S_SR_1[si], al
                cmp S_status[si], KEYOFF
                jnz __key_on
                jmp __key_released

__key_on:       mov ax, 0FFFFh ;For TV_inst, set it to keep playing until KEYOFF
                cmp [di].T_type, TV_INST
                jz __set_TV_inst
                mov ax, [di].T_duration ; set TVFX length to the duration value, plus 1
                inc ax

__set_TV_inst:  mov S_duration[bx], ax

                mov ax, [di].T_init_f_val
                mov S_f_val[bx], ax

                mov     ax, [di].T_init_f_offset
                add     ax, 2           ; It adds 2 because of the size value of the timbre, I think
                mov     S_f_offset[bx], ax

                mov     ax, [di].T_init_v0_val
                mov     S_v0_val[bx], ax

                mov     ax, [di].T_init_v0_offset
                add     ax, 2
                mov     S_v0_offset[bx], ax ; Connected to S_KSLTL_0 Used as offset for TVFX_sub_365E

                mov     ax, [di].T_init_v1_val
                mov     S_v1_val[bx], ax

                mov     ax, [di].T_init_v1_offset
                add     ax, 2
                mov     S_v1_offset[bx], ax ; Connected to S_KSLTL_1 Used as offset for TVFX_sub_365E

                mov     ax, [di].T_init_m0_val
                mov     S_m0_val[bx], ax

                mov     ax, [di].T_init_m0_offset
                add     ax, 2
                mov     S_m0_offset[bx], ax ; connected to S_AVEKM_0 Used as offset for TVFX_sub_365E

                mov     ax, [di].T_init_m1_val
                mov     S_m1_val[bx], ax

                mov     ax, [di].T_init_m1_offset
                add     ax, 2
                mov     S_m1_offset[bx], ax ; Connected to S_AVEKM_1 Used as offset for TVFX_sub_365E

                mov     ax, [di].T_init_fb_val
                mov     S_fb_val[bx], ax

                mov     ax, [di].T_init_fb_offset
                add     ax, 2
                mov     S_fb_offset[bx], ax ; Used as offset for TVFX_sub_365E

                mov     ax, [di].T_init_ws_val
                mov     S_ws_val[bx], ax

                mov     ax, [di].T_init_ws_offset
                add     ax, 2
                mov     S_ws_offset[bx], ax ; Used as offset for TVFX_sub_365E

                mov     ax, [di].T_init_p_val
                mov     S_p_val[bx], ax

                mov     ax, [di].T_init_p_offset
                add     ax, 2
                mov     S_p_offset[bx], ax ; Used as offset for TVFX_sub_365E
                jmp     short __finalize

__key_released: mov     ax, [di].T_f_rel_offset
                add     ax, 2
                mov     S_f_offset[bx], ax

                mov     ax, [di].T_v0_rel_offset
                add     ax, 2
                mov     S_v0_offset[bx], ax ; Connected to S_KSLTL_0 Used as offset for TVFX_sub_365E

                mov     ax, [di].T_v1_rel_offset
                add     ax, 2
                mov     S_v1_offset[bx], ax ; Connected to S_KSLTL_1 Used as offset for TVFX_sub_365E

                mov     ax, [di].T_m0_rel_offset
                add     ax, 2
                mov     S_m0_offset[bx], ax ; connected to S_AVEKM_0 Used as offset for TVFX_sub_365E

                mov     ax, [di].T_m1_rel_offset
                add     ax, 2
                mov     S_m1_offset[bx], ax ; Connected to S_AVEKM_1 Used as offset for TVFX_sub_365E

                mov     ax, [di].T_fb_rel_offset
                add     ax, 2
                mov     S_fb_offset[bx], ax ; Used as offset for TVFX_sub_365E

                mov     ax, [di].T_ws_rel_offset
                add     ax, 2
                mov     S_ws_offset[bx], ax ; Used as offset for TVFX_sub_365E

                mov     ax, [di].T_p_rel_offset
                add     ax, 2
                mov     S_p_offset[bx], ax ; Used as offset for TVFX_sub_365E

__finalize:     mov     S_update[si], U_ALL_REGS ; 7 = AVEKM, 6 = KSLTL, 5 = ADSR, 4 = WS, 3 = FBC, 0 = FREQ
                mov     S_f_counter[bx], 1
                mov     S_v0_counter[bx], 1 ; Gets decremented in TVFX_serve_synth
                mov     S_v1_counter[bx], 1 ; Gets decremented in TVFX_serve_synth
                mov     S_p_counter[bx], 1 ; Gets decremented in TVFX_serve_synth
                mov     S_fb_counter[bx], 1 ; Gets decremented in TVFX_serve_synth
                mov     S_m0_counter[bx], 1 ; Gets decremented in TVFX_serve_synth
                mov     S_m1_counter[bx], 1 ; Gets decremented in TVFX_serve_synth
                mov     S_ws_counter[bx], 1 ; Gets decremented in TVFX_serve_synth
                mov     S_f_increment[bx], 0
                mov     S_v0_increment[bx], 0
                mov     S_v1_increment[bx], 0
                mov     S_p_increment[bx], 0
                mov     S_fb_increment[bx], 0
                mov     S_m0_increment[bx], 0
                mov     S_m1_increment[bx], 0
                mov     S_ws_increment[bx], 0

                ret
                ENDP
