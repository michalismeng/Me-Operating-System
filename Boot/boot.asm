bits 16

jmp short main				; short jump uses relative address (jmp +12 bytes) and not absolute. We want this as segments are not yet set and	
							; we have used not org directive
							
sectors: dd 0
index_start: dd 0
index_count: dd 0
reserved_sectors: dd 0

main:

	cli
	mov     ax, 0x07C0
	mov     ds, ax
	mov     fs, ax
	mov     gs, ax
	 
	mov     ax, 0x0000
	mov     ss, ax
	mov     sp, 0x7C00 
	
	mov ax, 0x7e0
	mov es, ax
	sti
	
	mov byte [drive], dl
	
	call ReadDriveParams
	
	mov si, msg
	call Print
	
	call Reset
	
	mov si, reset_done
	call Print
	
	mov bx, 0x0								; buffer to read to (es:bx)
	mov cl, 1								; LBA sector to start reading from used in GetTrack
	mov dx, word [reserved_sectors]			; count of sectors to be read
	
	read_loop:
		push dx
		call GetTrack
		call ReadSector
		pop dx
		
		add bx, 512
		inc cl
		dec dx
		jz .end
		
		jmp read_loop
	
	.end:
	
	mov si, end_msg
	call Print	
	
	mov dl, byte [drive]

	jmp 0x07e0:0x0			; far jump to where we load
	
	ne_it:
	cli
	hlt
	
%include "Boot/BootCommon.inc"

number_of_heads db 0
sectors_per_track db 0

sector_to_read db 0
track_to_read db 0
head_to_read db 0

drive db 0

error_params_msg db "Errors params", 13, 10, 0
msg db "Welcome",13, 10, 0
reset_done db "reset done", 13, 10, 0
error_msg db "Cannot read sectors", 13, 10, 0
end_msg db "end of read", 13, 10, 0

TIMES 510-($-$$) DB 0
DW 0xAA55