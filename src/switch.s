;; task switching & calling

[global switch_to]

switch_to:
	;; first argument for old
	mov eax, [esp + 4]

	mov [eax + 0], esp
	mov [eax + 4], ebp
	mov [eax + 8], ebx
	mov [eax + 12], esi
	mov [eax + 16], edi
	pushf			; now save eflags for old
	pop ecx
	mov [eax + 20], ecx

	;; second argument for new
	mov eax, [esp + 8]

	mov esp, [eax + 0]
	mov ebp, [eax + 4]
	mov ebx, [eax + 8]
	mov esi, [eax + 12]
	mov edi, [eax + 16]
	mov eax, [eax + 20]
	push eax
	popf			; now restore eflags for new

	ret

[global switch_to_init]

;; as for the initial thread, we only want to switch current PC to 
;; another function and deprecate all old registers.
switch_to_init:
	;; argument is in EAX
	mov eax, [esp + 4]

	mov esp, [eax + 0]
        mov ebp, [eax + 4]
        mov ebx, [eax + 8]
        mov esi, [eax + 12]
        mov edi, [eax + 16]
        mov eax, [eax + 20]
        push eax
        popf

        ret

