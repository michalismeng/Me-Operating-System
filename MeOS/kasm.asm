
.386p
.model flat, c

.CODE

flush_gdt PROC gdt_data : DWORD
	
	mov eax, gdt_data

	lgdt FWORD PTR [eax]

	mov ax, 10h
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	push 08
	push _flush
	retf			; far jump (the masm way)

	_flush:
		ret

flush_gdt ENDP

flush_idt PROC idt_data : DWORD
	
	mov eax, [idt_data]
	lidt FWORD PTR [eax]
	ret

flush_idt ENDP

ISR_NOERRORCODE macro index
  isr&index& PROC
	cli

	push 0
	push &index&

	jmp isr_common_stub

	 isr&index& ENDP
endm

ISR_ERRORCODE macro index
  isr&index& PROC
	cli

	push &index&

	jmp isr_common_stub

	isr&index& ENDP
endm

IRQ macro irq_index, index
  irq&irq_index& PROC

	cli

	push 0
	push &index&

	jmp irq_common_stub

	irq&irq_index& ENDP
endm

ISR_ERRORCODE 8
ISR_ERRORCODE 10
ISR_ERRORCODE 11
ISR_ERRORCODE 12
ISR_ERRORCODE 13
ISR_ERRORCODE 14

ISR_NOERRORCODE 0
ISR_NOERRORCODE 1
ISR_NOERRORCODE 2
ISR_NOERRORCODE 3
ISR_NOERRORCODE 4
ISR_NOERRORCODE 5
ISR_NOERRORCODE 6
ISR_NOERRORCODE 7
ISR_NOERRORCODE 9
ISR_NOERRORCODE 15
ISR_NOERRORCODE 16
ISR_NOERRORCODE 17
ISR_NOERRORCODE 18
ISR_NOERRORCODE 19
ISR_NOERRORCODE 20
ISR_NOERRORCODE 21
ISR_NOERRORCODE 22
ISR_NOERRORCODE 23
ISR_NOERRORCODE 24
ISR_NOERRORCODE 25
ISR_NOERRORCODE 26
ISR_NOERRORCODE 27
ISR_NOERRORCODE 28
ISR_NOERRORCODE 29
ISR_NOERRORCODE 30
ISR_NOERRORCODE 31

IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

extern isr_handler : near
extern irq_handler : near

isr_common_stub:
				; already pushed error_code, interrupt no
  pushad		; push eax, ecx, edx, ebx, esp, ebp, esi, edi

  mov ax, ds
  push eax		; push "ds"

  mov ax, 10h
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  call isr_handler

  pop ebx
  mov ds, bx
  mov es, bx
  mov fs, bx
  mov gs, bx

  popad
  add esp, 8
  sti
  iret        ; pop CS, EIP, EFLAGS, SS, and ESP

irq_common_stub:
  pushad

  mov ax, ds
  push eax

  mov ax, 10h
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

 
  call irq_handler

  pop eax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  popad
  add esp, 8
  sti
  iretd

  call_interrupt:
	int 10
	ret

END