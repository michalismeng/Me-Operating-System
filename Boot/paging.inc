%ifndef PAGING_INC_27082016
%define PAGING_INC_27082016

bits 32

%define PAGE_DIR		0x9C000
%define PAGE_TABLE_0	0x9D000
%define PAGE_TABLE_768	0x9E000

%define PAGE_TABLE_ENTRIES	1024
%define ATTR_PRIV 3					; page is present, writable and kernel

EnablePaging:
	pusha

	; create page table 0

	mov eax, PAGE_TABLE_0
	mov ebx, 0x0 | ATTR_PRIV		; physical frame ored with attrivutes
	mov ecx, PAGE_TABLE_ENTRIES		; for each pt entry (4MB space)

	.loop:
	mov dword [eax], ebx
	add eax, 4
	add ebx, 4096
	loop .loop

	; setup page directory

	mov eax, PAGE_TABLE_0 | ATTR_PRIV
	mov dword [PAGE_DIR], eax

	mov eax, PAGE_TABLE_768 | ATTR_PRIV
	mov dword [PAGE_DIR + 4 * 768], eax

	mov eax, PAGE_DIR
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	mov eax, PAGE_TABLE_768
	mov ebx, 0x100000 | ATTR_PRIV
	mov ecx, PAGE_TABLE_ENTRIES

	.loop2:
	mov dword [eax], ebx
	add eax, 4
	add ebx, 4096
	loop .loop2

	popa
	ret

%endif