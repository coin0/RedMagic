;;
;;      Red Magic 1996 - 2015
;;
;;      x86.s - platform relevant code for X86
;;
;;      2015 Lin Coin - initial version
;;

;;
;; tools for X86-32bit
;;

;; save eflags
[global local_get_flags]

local_get_flags:
	pushf
	pop	eax
	ret

;; restore eflags
[global local_set_flags]

local_set_flags:
	mov 	eax, [esp + 4]
        push 	eax
        popf
	ret

