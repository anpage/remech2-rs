; Hand-written assembly: the tick counters, a MASM object assembled with MASM 6.11 (ML). It starts
; at 0x10067ed8, flush against simmain.c's code, and its data at 0x100ad008. Its routines save only
; the registers they use and use short jumps, which the VC++ 4.1 inline assembler can't emit, and
; PauseTimer ends in mov esp, ebp; pop ebp rather than ML's leave, so they are transcribed
; instruction for instruction. Annotated by name in ticks.h; COMPAT_MODE builds take ticks.c's
; portable C, tested against this by tests/asmequiv.

	.386
	.model flat, c
	option noscoped
	option casemap:none

	.data

; The two tick counters GameTickTimerCallback advances, and the start values of each counter's
; handles (0: free).
	public g_ticksPaused
g_ticksPaused dd 0

	public g_ticks1Bases
g_ticks1Bases_t struct
m_data dd 40h dup (0)
g_ticks1Bases_t ends
g_ticks1Bases g_ticks1Bases_t <>

	public g_ticks2Bases
g_ticks2Bases_t struct
m_data dd 40h dup (0)
g_ticks2Bases_t ends
g_ticks2Bases g_ticks2Bases_t <>

	public g_ticks1
g_ticks1 dd 0

	public g_ticks2
g_ticks2 dd 0

	.code

; The Miles timer that FirstClock registers (181 Hz) calls this. Each counter only runs while
; its bit in g_ticksPaused is clear (PauseTimer).
GameTickTimerCallback proc
	push ds
	pushad
	test dword ptr [g_ticksPaused], 200h
	jne jmp_10067eec
	inc dword ptr [g_ticks1]
jmp_10067eec:
	test dword ptr [g_ticksPaused], 100h
	jne jmp_10067efe
	inc dword ptr [g_ticks2]
jmp_10067efe:
	popad
	pop ds
	ret
GameTickTimerCallback endp

; Takes a free slot (0) of the counter selected by bit 0x80 and starts it at the counter's
; current value. Returns the handle (the slot, with 0x80 for the first counter).
AllocTicks proc
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	push edx
	mov eax, dword ptr [ebp+8]
	test ax, 80h
	jne jmp_10067f38
	lea ebx, [g_ticks2Bases]
	xor ecx, ecx
jmp_10067f18:
	cmp dword ptr [ebx], 0
	je jmp_10067f28
	cmp dword ptr [ebx], -1
	je jmp_10067f64
	inc ecx
	add ebx, 4
	jmp jmp_10067f18
jmp_10067f28:
	mov eax, dword ptr [g_ticks2]
	or eax, eax
	jne jmp_10067f32
	inc eax
jmp_10067f32:
	mov dword ptr [ebx], eax
	mov eax, ecx
	jmp jmp_10067f68
jmp_10067f38:
	lea ebx, [g_ticks1Bases]
	xor ecx, ecx
jmp_10067f40:
	cmp dword ptr [ebx], 0
	je jmp_10067f50
	cmp dword ptr [ebx], -1
	je jmp_10067f64
	inc ecx
	add ebx, 4
	jmp jmp_10067f40
jmp_10067f50:
	mov eax, dword ptr [g_ticks1]
	or eax, eax
	jne jmp_10067f5a
	inc eax
jmp_10067f5a:
	mov dword ptr [ebx], eax
	mov eax, ecx
	or ax, 80h
	jmp jmp_10067f68
jmp_10067f64:
	mov ax, 0ffffh
jmp_10067f68:
	pop edx
	pop ecx
	pop ebx
	mov esp, ebp
	pop ebp
	ret
AllocTicks endp

GetTicks proc
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	xor ecx, ecx
	mov ecx, dword ptr [ebp+8]
	test cx, 80h
	jne jmp_10067f8d
	lea ebx, [g_ticks2Bases]
	mov eax, dword ptr [g_ticks2]
	jmp jmp_10067f9d
jmp_10067f8d:
	xor cx, 80h
	lea ebx, [g_ticks1Bases]
	mov eax, dword ptr [g_ticks1]
jmp_10067f9d:
	shl ecx, 2
	add ebx, ecx
	sub eax, dword ptr [ebx]
	pop ecx
	pop ebx
	mov esp, ebp
	pop ebp
	ret
GetTicks endp

ResetTicks proc
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	xor ecx, ecx
	mov ecx, dword ptr [ebp+8]
	test cx, 80h
	jne jmp_10067fc8
	lea ebx, [g_ticks2Bases]
	mov eax, dword ptr [g_ticks2]
	jmp jmp_10067fd8
jmp_10067fc8:
	xor cx, 80h
	lea ebx, [g_ticks1Bases]
	mov eax, dword ptr [g_ticks1]
jmp_10067fd8:
	shl ecx, 2
	add ebx, ecx
	mov dword ptr [ebx], eax
	pop ebx
	pop ecx
	mov esp, ebp
	pop ebp
	ret
ResetTicks endp

SetTicks proc
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	xor ecx, ecx
	mov ecx, dword ptr [ebp+8]
	test cx, 80h
	jne jmp_10068003
	lea ebx, [g_ticks2Bases]
	mov eax, dword ptr [g_ticks2]
	jmp jmp_10068013
jmp_10068003:
	xor cx, 80h
	mov eax, dword ptr [g_ticks1]
	lea ebx, [g_ticks1Bases]
jmp_10068013:
	shl ecx, 2
	add ebx, ecx
	sub eax, dword ptr [ebp+0ch]
	mov dword ptr [ebx], eax
	pop ecx
	pop ebx
	mov esp, ebp
	pop ebp
	ret
SetTicks endp

FreeTicks proc
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	xor ecx, ecx
	mov ecx, dword ptr [ebp+8]
	test cx, 80h
	jne jmp_1006803c
	lea ebx, [g_ticks2Bases]
	jmp jmp_10068047
jmp_1006803c:
	xor cx, 80h
	lea ebx, [g_ticks1Bases]
jmp_10068047:
	shl ecx, 2
	add ebx, ecx
	mov dword ptr [ebx], 0
	pop ecx
	pop ebx
	mov esp, ebp
	pop ebp
	ret
FreeTicks endp

; Stops (p_paused) or restarts the counters selected by p_flags: 0x80 the first, 0x100 the
; second.
PauseTimer proc
	push ebp
	mov ebp, esp
	push ebx
	xor eax, eax
	test word ptr [ebp+8], 80h
	je jmp_1006806b
	or eax, 200h
jmp_1006806b:
	test word ptr [ebp+8], 100h
	je jmp_10068078
	or eax, 100h
jmp_10068078:
	mov bx, word ptr [ebp+0ch]
	or bx, bx
	je jmp_10068083
	jmp jmp_1006808d
jmp_10068083:
	not eax
	and dword ptr [g_ticksPaused], eax
	jmp jmp_10068093
jmp_1006808d:
	or dword ptr [g_ticksPaused], eax
jmp_10068093:
	pop ebx
	mov esp, ebp
	pop ebp
	ret
PauseTimer endp

	end
