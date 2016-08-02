bits 16

org 0x7e00

jmp main

%include "Boot/common16.inc"
%include "Boot/GDT.inc"
%include "Boot/memory.inc"
%include "Boot/BootInfo.inc"

boot_info:
istruc multiboot_info
	at multiboot_info.flags,			dd 0
	at multiboot_info.memoryLo,			dd 0
	at multiboot_info.memoryHi,			dd 0
	at multiboot_info.bootDevice,		dd 0
	at multiboot_info.cmdLine,			dd 0
	at multiboot_info.mods_count,		dd 0
	at multiboot_info.mods_addr,		dd 0
	at multiboot_info.syms0,			dd 0
	at multiboot_info.syms1,			dd 0
	at multiboot_info.syms2,			dd 0
	at multiboot_info.mmap_length,		dd 0
	at multiboot_info.mmap_addr,		dd 0
	at multiboot_info.drives_length,	dd 0
	at multiboot_info.drives_addr,		dd 0
	at multiboot_info.config_table,		dd 0
	at multiboot_info.bootloader_name,	dd 0
	at multiboot_info.apm_table,		dd 0
	at multiboot_info.vbe_control_info,	dd 0
	at multiboot_info.vbe_mode_info,	dw 0
	at multiboot_info.vbe_interface_seg,dw 0
	at multiboot_info.vbe_interface_off,dw 0
	at multiboot_info.vbe_interface_len,dw 0
iend

GDTLoaded db "GDT loaded", 13, 10, 0
A20Enable db "A20 Enabled", 13, 10, 0

main:
	
	cli
	
	mov ax, 0		; we are loaded at 0x7e0:0
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	mov ax, 0x9000
	mov ss, ax
	mov sp, 0xFFFF		   ; no no no stack at 0x7c0:0
	
	sti
	mov byte [drive], dl				; boot loader passes this info to us through dl just before it jumps
	
	call EnableA20Bios
	cmp ax, 0			; check if enabled
	
	je .A20Enabled
	
	call EnableA20Fast
	
.A20Enabled:

	mov si, A20Enable
	call Print

	xor edx, edx
	mov dl, byte [drive]
	mov [boot_info + multiboot_info.bootDevice], edx
	
	xor eax, eax
	xor ebx, ebx
	call BiosGetMemorySize64MB
	
	mov word [boot_info + multiboot_info.memoryHi], bx
	mov word [boot_info + multiboot_info.memoryLo], ax

	mov di, 0x500		; es:di = 0x0:0x500
	call BiosGetMemoryMap
	
	; Load global descriptor tables
	cli
	
	lgdt [gdt_info]

	mov	eax, cr0			; set bit 0 in cr0--enter pmode
	or	eax, 1
	mov	cr0, eax
	
	jmp 0x8:Stage3
	
	
bits 32

%include "Boot/common32.inc"

kernel_size dd 0

Stage3:							; PMode here

	mov ax, 0x10				; change all other segments to point to data descriptor
	mov ds, ax
	mov es, ax
	
	mov ss, ax
	mov esp, 0xFFFF
	
	call CopyKernel						; copy kernel to 1MB
	
	mov edx, dword [kernel_size]		; size of kernel in bytes
	
	; Do black magic stuff of PE format... (See brokenthorn Kernel Setup)
	mov ebx, [KERNEL_PMODE_BASE + 60]
	add ebx, KERNEL_PMODE_BASE
	
	add ebx, 24
	mov eax, [ebx]
	add ebx, 16
	
	mov ebp, dword [ebx]
	add ebx, 12
	mov eax, dword [ebx]
	add ebp, eax	

	mov eax, 0x2BADB002
	mov ebx, 0
	
	push dword [memory_map_len]
	push dword boot_info

	call ebp		; execute kernel
	
	cli
	hlt

