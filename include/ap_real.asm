;;some definitions for GDT entries

;; make sure ADDR_AP_REAL is equal to the same definition
;; in ap_real.h, all APs will boot from this location
ADDR_AP_REAL 	equ 0x4000
    
S_32 	equ 0x4000
    
S_DPL0 	equ 0x00
S_DPL1 	equ 0x20
S_DPL2 	equ 0x40
S_DPL3 	equ 0x60

S_DR 	equ 0x90
S_DRW 	equ 0x92
S_DRWA 	equ 0x93
S_C 	equ 0x98
S_CR 	equ 0x9a
S_CCO 	equ 0x9c
S_CCOR 	equ 0x9e

S_G	equ 0x8000
S_DB	equ 0x4000
S_L	equ 0x2000
S_AVL	equ 0x1000

;; macro to set GDT entries
%macro GDT_SET 3
    dw %2 & 0xffff				; segment length 1
    dw %1 & 0xffff				; segment base 1 
    db(%1 >> 16) & 0xff				; segment base 2 
    dw((%2 >> 8) & 0x0f00) | (%3 & 0xf0ff) 	; attr1 + seg len 2 + attr 2 
    db(%1 >> 24) & 0xff				;segment base 3 
%endmacro


