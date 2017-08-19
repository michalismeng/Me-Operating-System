%ifndef COMMON16_060516
%define COMMON16_060516

; includes all real mode 16 bit utilities

sector_to_read db 0
track_to_read db 0
head_to_read db 0

drive db 0

number_of_heads db 0
sectors_per_track db 0

Print:
	pusha
	Printloop:
			
			lodsb
			or	al, al
			jz	PrintDone
			mov	ah, 0eh			
			int	10h
			jmp	Printloop		
PrintDone:
			popa
			ret					
			
Mode12:
	mov		ax, 0xa000
	mov		es, ax
	
	mov ah, 00		; switch to color mode
	mov al, 0x12
	int 0x10

	ret
			
Reset:
	mov ah, 0
	mov dl, 80h
	int 13h
	ret
	
error_params_msg db "Errors params", 13, 10, 0
error_msg db "Cannot read sectors", 13, 10, 0

ReadDriveParams:
	pusha
	
	mov ah, 0x8
	mov dl, byte [drive]
	mov di, 00
	
	int 0x13
	jc .error
		
	inc dh
	mov byte [number_of_heads], dh

	and cl, 0x3F
	mov byte [sectors_per_track], cl
	
	jmp .success
	
	.error:
		mov si, error_params_msg
		call Print	
	
	.success:
	popa
	ret
	
;Temp = LBA / (Sectors per Track)
;Sector = (LBA % (Sectors per Track)) + 1
;Head = Temp % (Number of Heads)
;Cylinder = Temp / (Number of Heads) 

GetTrack:
	pusha
	
	xor ax, ax	
	mov al, cl
	div byte [sectors_per_track]		; byte division saves remainder at ah. !!!!! word division saves remainder at dx
	
	inc ah
	mov byte [sector_to_read], ah
	
	xor ax, ax
	mov al, cl
	div byte [sectors_per_track]
	
	xor ah, ah
	div byte [number_of_heads]
	
	mov byte [head_to_read], ah
	mov byte [track_to_read], al
	
	popa	
	ret
			
ReadSector:
	pusha
	
	mov di, 10
	
	_loop:
		mov ah, 2
		mov al, 1
		mov cl, byte [sector_to_read]
		mov ch, byte [track_to_read]
		mov dh, byte [head_to_read]
		mov dl, byte [drive]
		
		int 13h 
		
		jnc success
		
		dec di
		cmp di, 0
		je .error
		
		call Reset
		jmp _loop
	
	.error:
		mov si, error_msg
		call Print
		
		cli
		hlt
	
	success:
		popa
		ret

EnableA20Bios:
	
	cli
	mov ax, 0x2401
	int 0x15
	
	jc .not_supported
	
	mov ax, 0		; no error, line enabled
	jmp .return
	
	.not_supported:
		mov ax, 1	; function not supported
		
	.return:
		sti 
		ret
	
EnableA20Fast:

	cli
	push ax
	mov al, 0xdd
	out 0x64, al
	pop ax
	sti
	ret
	
%endif