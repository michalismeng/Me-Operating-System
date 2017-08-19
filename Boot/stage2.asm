bits 16

%define STAGE2_BASE		 0xFA00				; remember to keep these values synced with the number of sectors used by the stage2 boot.
%define STAGE2_BASE_HIGH 0xFA0				; we change the base where every reserved sector is loaded to align the krnldr at 0x10000 (vs requirement).
org STAGE2_BASE

jmp main

%include "Boot/common16.inc"
%include "Boot/GDT.inc"
%include "Boot/memory.inc"
%include "Boot/BootInfo.inc"
%include "Boot/vbe.inc"

boot_info:
istruc multiboot_info
	at multiboot_info.flags,				dd 0
	at multiboot_info.memoryLo,				dd 0
	at multiboot_info.memoryHi,				dd 0
	at multiboot_info.bootDevice,			dd 0
	at multiboot_info.cmdLine,				dd 0
	at multiboot_info.mods_count,			dd 0
	at multiboot_info.mods_addr,			dd 0
	at multiboot_info.syms0,				dd 0
	at multiboot_info.syms1,				dd 0
	at multiboot_info.syms2,				dd 0
	at multiboot_info.mmap_length,			dd 0
	at multiboot_info.mmap_addr,			dd 0
	at multiboot_info.drives_length,		dd 0
	at multiboot_info.drives_addr,			dd 0
	at multiboot_info.config_table,			dd 0
	at multiboot_info.bootloader_name,		dd 0
	at multiboot_info.apm_table,			dd 0
	at multiboot_info.vbe_control_info,		dd 0
	at multiboot_info.vbe_mode_info,		dw 0
	at multiboot_info.vbe_interface_seg,	dw 0
	at multiboot_info.vbe_interface_off,	dw 0
	at multiboot_info.vbe_interface_len,	dw 0
iend

GDTLoaded db "GDT loaded", 13, 10, 0
A20Enable db "A20 Enabled", 13, 10, 0
VESA_no_support db "VESA not supported", 13, 10, 0
VESAMode_fail db "VESA mode selection failed", 13, 10, 0
LFAFail db "No LFA supported", 13, 10, 0
VESAMode_success db "Mode found", 13, 10, 0
VESAMode_no_support db "Mode is not supported", 13, 13, 0

VESA_mode dw 0

main:
	
	cli
	
	mov ax, 0				; we are loaded at 0xFC0:0
	mov ds, ax
	mov fs, ax
	mov gs, ax

	mov ax, 0
	mov es, ax
	
	mov ax, 0x9000
	mov ss, ax
	mov sp, 0xFFFF		   ; no no no stack at 0xFC0:0
	
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

	mov dword [boot_info + multiboot_info.mmap_addr], 0x500 
	mov eax, dword [memory_map_len]
	mov dword [boot_info + multiboot_info.mmap_length], eax

	;call Mode12			; bios switch to color mode 12h (640x400x4 - 16 colors)

	; call vbe controller info
	mov di, 0x1000
	call BiosGetVESAInfo

	cmp ax, 0x004F
	jne .vesa_no_support

	mov ax, 800
	mov bx, 600
	mov cl, 32
	mov edx, 0x1000

	call VESAGetSuitableMode

	cmp ax, 1
	je .mode_fail

	.mode_success:
		
		mov [VESA_mode], cx
		
		mov si, VESAMode_success
		call Print

		; check whether this mode is supported
		call VESAHardwareAndLFASupportMode
		cmp ax, 0
		jne .mode_not_supported

		; set the selected mode

		mov cx, [VESA_mode]
		mov [boot_info + multiboot_info.vbe_mode_info], cx
		call VESASetMode

		cmp ax, 0x004F
		jne .vesa_no_support

		jmp .vesa_complete

	.vesa_no_support:
		mov si, VESA_no_support
		call Print
		cli
		hlt

	.mode_not_supported:
		mov si, VESAMode_no_support
		call Print
		cli
		hlt

	.mode_fail:
		mov si, VESAMode_fail
		call Print
		cli
		hlt
	
	.vesa_complete:
		cli

		lgdt [gdt_info]
		
		mov	eax, cr0			; set bit 0 in cr0--enter pmode
		or	eax, 1
		mov	cr0, eax

		jmp 0x8:Stage3
	
bits 32

%include "Boot/common32.inc"
%include "Boot/paging.inc"

krn_ldr dd 0

Stage3:							; PMode here
	
	mov ax, 0x10				; change all other segments to point to data descriptor
	mov ds, ax
	mov es, ax
	
	mov ss, ax
	mov esp, 0x90000

	;call EnablePaging
	
	;call CopyKernel						; copy kernel to 3GB (virtual)

	call GetKrnLdrSize	
		
	; Do black magic stuff of PE format... (See brokenthorn Kernel Setup)
	mov ebx, [KERNEL_RMODE_BASE + 60]
	add ebx, KERNEL_RMODE_BASE
	
	add ebx, 24
	mov eax, [ebx]
	add ebx, 16
	
	mov ebp, dword [ebx]
	add ebx, 12
	mov eax, dword [ebx]
	add ebp, eax	

	mov eax, 0x2BADB002
	mov ebx, 0
	
	push dword [krn_ldr]
	push dword boot_info

	call ebp		; execute kernel

