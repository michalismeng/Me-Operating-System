%ifndef MEMORY_INC_080516
%define MEMORY_INC_080516

; Define methods to retrieve system memory size

struc MemoryMapEntry
	.base_address resq 1 
	.length 	  resq 1	; length in bytes
	.type		  resd 1 	; 1: Available, 2: Reserved, 3: ACPI Reclaim, 4: ACPI NVS, all other undefined
	.acpi_null	  resd 1 	; reserved
endstruc

BiosGetMemorySize:

	int 0x12		; generate interrupt to get memory below 64KB (if system has more this is erroneus)
	jc .error		; on carry set => error
	test ax, ax		; is ax zero?
	je .error
	cmp ax, 0x86	; unsupported function
	je .error
	cmp ax, 0x80	; invalid command
	je .error
	ret
	
	.error:
		mov ax, -1
		ret
		
BioGetExtendedMemorySize:	
	
	mov ax, 0x88
	int 0x15		; return the amount of contiguous KB starting at 1MB. Because of 16 bit limit max value is 64 MB
	jc .error
	test ax, ax
	je .error
	cmp ax, 0x86
	je .error
	cmp ax, 0x80
	je .error
	ret
	
	.error:
		mov ax, -1
		ret

		
; returns
; ax: KB between !MB and 16MB
; bx: number of 64 KB blocks above 16MB
; bx:0 ax:-1 on error

BiosGetMemorySize64MB:
	
	push ecx
	push edx
	xor ecx, ecx
	xor edx, edx
	
	mov ax, 0xe801
	int 0x15			; this function stores results at etiher eax, ebx or ecx, edx pairs. We have zeroed ecx, edx and now we check
	jc .error
	cmp ax, 0x86
	je .error
	cmp ax, 0x80
	je .error
	
	jcxz .use_ax		; if ecx == 0 bios has used ax
	
	mov ax, cx			; exchange pairs
	mov bx, dx
	
	.use_ax:
		
		pop edx
		pop ecx
		ret
	
	.error:
		
		pop edx
		pop ecx
		mov bx, 0
		mov ax, -1
		ret

; requires
; ebx : 0 for the start of the map, != 0 for continuation
; ecx : size of buffer for result >= 20 bytes
; edx : smap litteral
; es:di buffer for result

; returns
; cf : clear on success
; eax : smap litteral
; ebx : next entry or 0 if finished
; ecx : actual length returned in bytes
; es:di biffer filled
; variable memory_map_len : entries entered (stored in bp)

memory_map_len dd 0

BiosGetMemoryMap:
	
	pushad
	
	xor ebx, ebx
	xor ebp, ebp 		; number of entries stored here
	mov edx, 'PAMS'		; little endian SMAP
	mov eax, 0xe820
	mov ecx, 24			; size of our MemoryMapEntry struct
	int 0x15
	
	jc .error
	cmp eax, 'PAMS'
	jne .error
	test ebx, ebx		; if ebx is zero memory map has only one entry so exit
	je .error
	
	jmp .start
	
	.next_entry:
		mov edx, 'PAMS'		; reset this register as some bios trash the register
		mov ecx, 24
		mov eax, 0xe820
		int 0x15
	
	.start:
		jcxz .skip_entry	; if cx == 0 this entry is empty so move to the next one
		
	.check_values:
		mov ecx, [es:di + MemoryMapEntry.length]		; get low dword of entry length
		test ecx, ecx									; if length is zero skip_entry
		jne .good_entry
		mov ecx, [es:di + MemoryMapEntry.length + 4]
		jcxz .skip_entry
		
	.good_entry:
		inc ebp			; one more entry
		add di, 24		; save this entry by moving the di pointer to the next memory location
		
	.skip_entry:
		cmp ebx, 0			; if bx == 0 we end
		jne .next_entry
		jmp .done
	
	.error:
		stc				; set carry
	.done:
		mov dword [memory_map_len], ebp
		popad 
		ret

%endif