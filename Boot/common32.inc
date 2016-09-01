%ifndef COMMON32_060516
%define COMMON32_060516

; includes all protected mode 32 bit utilities

%define KERNEL_RMODE_BASE 0x8200
%define KERNEL_PMODE_BASE 0xc0000000

CopyKernel:
	
	pusha 
	
	mov eax, dword [0x7c00 + 14]		; get sectors loaded
	sub eax, 2							; this is one sector for stage2
	
	mov ebx, 512						; multiply sector by bytes per sector
	mul ebx	

	mov dword [kernel_size], eax
	
	mov ebx, 4							; divide by four as we copy dword each time
	div ebx
	
	cld									; clear direction flag. We copy forwards
	mov esi, KERNEL_RMODE_BASE
	mov edi, KERNEL_PMODE_BASE
	mov ecx, eax
	
	rep movsd 					; repeat until cx is zero: copy from ds:esi to es:edi
	
	popa
	ret

%endif