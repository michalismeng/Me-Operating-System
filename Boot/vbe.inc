%ifndef VBE_INC_18082017
%define VBE_INC_18082017

; define VESA VBE methods to select and prepare the screen resolution

struc vbe_info_block
	.vbe_sig				resd 1 		; VBE Signature
	.vbe_ver				resw 1		; VBE Version
	.oem_str_ptr			resd 1		; VbeFarPtr to OEM String
	.capabilities			resd 1		; Capabilities of graphics controller
	.video_mem_ptr			resd 1		; VbeFarPtr to VideoModeList
	.total_mem				resw 1		; Number of 64kb memory blocks; Added for VBE 2.0+
	.oem_soft_rev			resw 1		; VBE implementation Software revision
	.oem_vendor_name_ptr	resd 1		; VbeFarPtr to Vendor Name String
	.oem_product_name_ptr	resd 1		; VbeFarPtr to Product Name String
	.oem_product_rev_ptr	resd 1		; VbeFarPtr to Product Revision String
	.resv					resb 222	; Reserved for VBE implementation scratch area
	.oem_data				resb 256	; Data Area for OEM Strings
endstruc

struc vbe_mode_info_block
	.attributes				resw 1		; deprecated mode attributes, bit 7 is of interest as it indicates support for LFA
	.useless1				resb 14		; useless info, mostly deprecated. Check specification
	.pitch					resw 1		; number of bytes per horizontal line
	.width					resw 1		; width in pixels
	.height					resw 1		; height in pixels
	.useless2				resb 3		; more useless stuff
	.bpp					resb 1		; bytes per pixel
	.useless3				resb 5		; more useless stuff
	.red_mask				resb 1		;
	.red_position			resb 1		;
	.green_mask				resb 1		;
	.green_position			resb 1		;
	.blue_mask				resb 1		;
	.blue_position			resb 1		;
	.reserved				resb 3		; reserved stuff
	.framebuffer			resd 1		; the LFA used by this mode (if indicated by attibutes)
	.useless4				resb 212	; more useless stuff
endstruc


; Gets the main VESA information through the bios interface
; requires 
; es:di : pointer to vbe info block structure (that will be filled with data)

; returns
; the vbe info block structure filled upon succes
; ax contains vbe error result

BiosGetVESAInfo:

	; signature must be VBE2 to indicate VBE2.0+ interface

	mov byte [es:di + vbe_info_block.vbe_sig + 0], 'V'
	mov byte [es:di + vbe_info_block.vbe_sig + 1], 'B'
	mov byte [es:di + vbe_info_block.vbe_sig + 2], 'E'
	mov byte [es:di + vbe_info_block.vbe_sig + 3], '2'

	mov ax, 0x4F00
	int 0x10												; call function 0x4F00 of 0x10 bios interrupt

	; interrupt sets signature to VESA

	cmp byte [es:di + vbe_info_block.vbe_sig + 0], 'V'
	jnz .vesa_error

	cmp byte [es:di + vbe_info_block.vbe_sig + 1], 'E'
	jnz .vesa_error

	cmp byte [es:di + vbe_info_block.vbe_sig + 2], 'S'
	jnz .vesa_error

	cmp byte [es:di + vbe_info_block.vbe_sig + 3], 'A'
	jnz .vesa_error

	ret

	.vesa_error:
		mov si, vesa_init_error
		call Print
		cli
		hlt


; Returns the mode number that satisfies the given width/height/bpp parameters
; requires
; ax	: width in pixels
; bx	: height in pixels
; cl	: bits per pixel
; edx	: (high) segment, (low) offset of vbe info block

; returns
; ax	: 0 if a suitable mode is found, 1 otherwise
; cx	: mode number
; es:di : vbe_mode_info_block full with the details of the mode

; modifies
; es : 0
; di : 2000
; contents of memory es:di for 256 bytes

VESAGetSuitableMode:
	
	push si

	; temporarily save width, height, bpp
	mov [width], ax
	mov [height], bx
	mov [bpp], cl

	; acquire index into video memory ptr where the available modes are stored
	mov di, dx
	shr edx, 16
	mov es, dx

	mov ax, [es:di + vbe_info_block.video_mem_ptr]
	mov word [_offset], ax

	mov ax, [es:di + vbe_info_block.video_mem_ptr + 2]
	mov word [_segment], ax

	; load given segment:offset to es:si
	mov ax, [_segment]
	mov es, ax
	mov si, [_offset]

	.probe_mem_list:
	
		; increment si by two (we read words)
		mov cx, [es:si]
		add si, 2

		; check if mode list has ended
		cmp cx, 0xFFFF
		je .mode_not_found

		push es
		; here we have a candidate mode stored in cx. Now call interrupt to get information anout the mode
		mov ax, 0
		mov es, ax
		mov di, 0x2000
		mov ax, 0x4F01
		; cx contains already the mode to read

		int 0x10		; read the mode inforamtion at location 0x0:0x1000
		pop es

		cmp ax, 0x004f
		jne .vesa_error

		; compare the given graphics requirements with the returns mode information
		mov ax, [width]
		cmp ax, [es:di + vbe_mode_info_block.width]
		jne .probe_mem_list

		mov ax, [height]
		cmp ax, [es:di + vbe_mode_info_block.height]
		jne .probe_mem_list

		mov al, [bpp]
		cmp al, [es:di + vbe_mode_info_block.bpp]
		jne .probe_mem_list

		; here we have found a matching mode, so return it
		pop si
		mov ax, 0
		ret

	.mode_not_found:
		pop si
		mov ax, 1
		ret

	.vesa_error:
		mov si, vesa_bad_return
		call Print
		cli
		hlt

; Checks whether the selected mode supports LFA and is supported by the current hardware configuration
;requires
; 0x0:0x2000 : valid vbe mode info block

; returns
; ax : 0 if the mode is supported, 1 otherwise

VESAHardwareAndLFASupportMode:

	; setup registers to point to the vbe mode info block
	mov ax, 0
	mov es, ax
	mov di, 0x2000
	
	; test hardware support (bit 1 of the attributes field)
	mov ax, [es:di + vbe_mode_info_block.attributes]
	and ax, 0x1
	jz .not_supported

	; test LFA support (bit 7 of the attributes field)
	mov ax, [es:di + vbe_mode_info_block.attributes]
	and ax, 0x80
	jz .not_supported

	mov ax, 0
	ret

	.not_supported:
		mov ax, 1
		ret

; Sets the indicated mode
; requires
; cx : the mode to set

; returns
; ax : contains vbe error result
VESASetMode: 
	
	mov ax, 0x4F02
	mov bx, cx
	or bx, 0x4000			; enable the framebuffer bit to allow direct access from protected mode

	int 0x10				; set the mode
	
	ret						
		
vesa_mode db "next mode", 13, 10, 0
vesa_init_error db "VESA bad initialization", 13, 10, 0
vesa_bad_return db "VESA function failed", 13, 10, 0
width		dw 0
height		dw 0
bpp			db 0
_segment	dw 0
_offset		dw 0

%endif