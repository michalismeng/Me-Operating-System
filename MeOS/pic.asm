; Listing generated by Microsoft (R) Optimizing Compiler Version 19.00.23918.0 

	TITLE	C:\Users\michalis\Documents\Visual Studio 2015\Projects\MeOS\MeOS\pic.c
	.686P
	.XMM
	include listing.inc
	.model	flat

INCLUDELIB MSVCRTD
INCLUDELIB OLDNAMES

PUBLIC	_init_pic
EXTRN	_outportb:PROC
; Function compile flags: /Odtpy
_TEXT	SEGMENT
_cw$ = -1						; size = 1
_init_pic PROC
; File c:\users\michalis\documents\visual studio 2015\projects\meos\meos\pic.c
; Line 4
	push	ebp
	mov	ebp, esp
	push	ecx
; Line 7
	mov	BYTE PTR _cw$[ebp], 17			; 00000011H
; Line 9
	movzx	eax, BYTE PTR _cw$[ebp]
	push	eax
	push	32					; 00000020H
	call	_outportb
	add	esp, 8
; Line 10
	movzx	ecx, BYTE PTR _cw$[ebp]
	push	ecx
	push	160					; 000000a0H
	call	_outportb
	add	esp, 8
; Line 14
	push	32					; 00000020H
	push	33					; 00000021H
	call	_outportb
	add	esp, 8
; Line 15
	push	40					; 00000028H
	push	161					; 000000a1H
	call	_outportb
	add	esp, 8
; Line 19
	push	4
	push	33					; 00000021H
	call	_outportb
	add	esp, 8
; Line 20
	push	2
	push	161					; 000000a1H
	call	_outportb
	add	esp, 8
; Line 24
	mov	BYTE PTR _cw$[ebp], 1
; Line 26
	movzx	edx, BYTE PTR _cw$[ebp]
	push	edx
	push	33					; 00000021H
	call	_outportb
	add	esp, 8
; Line 27
	movzx	eax, BYTE PTR _cw$[ebp]
	push	eax
	push	161					; 000000a1H
	call	_outportb
	add	esp, 8
; Line 31
	push	0
	push	33					; 00000021H
	call	_outportb
	add	esp, 8
; Line 32
	push	0
	push	161					; 000000a1H
	call	_outportb
	add	esp, 8
; Line 33
	mov	esp, ebp
	pop	ebp
	ret	0
_init_pic ENDP
_TEXT	ENDS
END
