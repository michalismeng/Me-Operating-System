%ifndef BOOT_COMMON_060516
%define BOOT_COMMON_060516

Print:
	pusha
	Printloop:
			
			lodsb				; load next byte from string from SI to AL
			or	al, al			; Does AL=0?
			jz	PrintDone		; Yep, null terminator found-bail out
			mov	ah, 0eh			; Nope-Print the character
			int	10h
			jmp	Printloop		; Repeat until null terminator found
PrintDone:
			popa
			ret					; we are done, so return
		
; compares two null-terminated strings at si and di cx times
Strcmp:
	pusha
	
	.compare:
		mov al, byte [ds:si]
		mov bl, byte [ds:di]
		cmp al, bl
		
		jne .nequal
		
		inc si
		inc di
		dec cx
		jnz .compare
		
		popa
		mov ax, 1
		ret
	
	.nequal:
		popa
		mov ax, 0
		ret
	
			
Reset:
	mov ah, 0
	mov dl, byte [drive]
	int 13h
	ret
	
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
		
%endif