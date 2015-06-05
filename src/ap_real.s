;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; this is the first step for AP ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%include "ap_real.asm"

[SECTION .apinit.text]
[BITS 16]
ap_start:
	mov ax, cs
	mov ds, ax

	cli

	lgdt [gdtdesc]

	mov eax, cr0
	or  eax, 1
	mov cr0, eax

	jmp dword SEL_KCODE:ADDR_AP_REAL + pm32

[BITS 32]
pm32:
	mov ax, SEL_KDATA
	mov ds, ax
	mov es, ax
	mov ss, ax

	mov ax, 0
	mov fs, ax
	mov gs, ax

	mov esp, [ADDR_AP_REAL - 8]			; the stack
	push dword [ADDR_AP_REAL - 12]			; push per-cpu struct
	call dword [ADDR_AP_REAL - 4]			; the main

;; initial GDT for APs
ALIGN 32

gdt:
gdt_null: GDT_SET 0, 0, 0
gdt_code: GDT_SET 0, 0xffffffff, S_CR + S_G + S_DB
gdt_data: GDT_SET 0, 0xffffffff, S_DRW + S_G + S_DB

gdtdesc:
        dw $ - gdt - 1
        dd ADDR_AP_REAL + gdt

SEL_KCODE	equ	gdt_code - gdt
SEL_KDATA	equ	gdt_data - gdt
