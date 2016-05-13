%ifndef GDT_UTILITY_060516
%define GDT_UTILITY_060516

gdt_data:

	; null gdt entry (8 bytes)
	dd 0
	dd 0
	
	; Kernel space code at (0x8)
	dw 0xFFFF				; limit low
	dw 0					; base low
	db 0					; base middle
	db 10011010b 			; access - Notice that bits 5 and 6 (privilege level) are 0 for Ring 0
	db 11001111b 			; granularity
	db 0 					; base high
	
	; Kernel space data at (0x10)
	dw 0FFFFh
	dw 0
	db 0			
	db 10010010b
	db 11001111b
	db 0
gdt_end:
	
gdt_info:
	
	dw gdt_end - gdt_data - 1 ; define size -1
	dd gdt_data
	
%endif