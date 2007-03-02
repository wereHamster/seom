
BITS 64

SECTION .rodata
ALIGN 16
yMul: dw    16,      157,        47,      0
uMul: dw   112,      -86,       -26,      0
vMul: dw   -10,     -102,       112,      0

SECTION .text

; rdi : buf
; rsi : width
; rdx : height
global __seomFrameResample: function
__seomFrameResample:
    imul    rsi,4             ; rsi = width in bytes
    imul    rdx,rsi           ; rdx = size of buffer in bytes
    lea     rcx,[rdi+rdx]     ; rcx = end of buffer
    pxor    mm7,mm7
    mov     r8,rdi            ; r8 = 1st row
    lea     rdx,[r8+rsi]      ; rdx = end of 1st row

.L4:
    lea     r9,[r8+rsi]       ; r9 = 2nd row

.L5:
    movd      mm0,[r8]
    punpcklbw mm0,mm7
    movd      mm1,[r8+4]
    punpcklbw mm1,mm7
    movd      mm2,[r9]
    punpcklbw mm2,mm7
    movd      mm3,[r9+4]
    punpcklbw mm3,mm7

    paddusw   mm0,mm1
    paddusw   mm0,mm2
    paddusw   mm0,mm3
    psrlw     mm0,2
    packuswb  mm0,mm7
    movd      [rdi],mm0
    
    add     r8,8
    add     r9,8
    add     rdi,4
    
    cmp     r8,rdx
    jne    .L5

.L6:
    cmp     rcx,r9            ; end of buffer?
    je     .L9
    mov     r8,r9             ; r8 = 1st row
    lea     rdx,[r8+rsi]      ; rdx = end of 1st row
    jmp    .L4

.L9:
    emms
    ret

; rdi : out[3]
; rsi : in
; rdx : width
; rcx : height
global __seomFrameConvert: function
__seomFrameConvert:
    push    r12
    push    r13
    push    r14
    push    r15
    
    mov     r8,rsi            ; r8 = 1st src row
    lea     r14,[rdx*4]       ; r14 = width of one src line
    imul    rcx,r14           ; rcx = size of buffer in bytes
    lea     r15,[r8+rcx]      ; r15 = end of buffer
    lea     rcx,[r8+r14]      ; rcx = end of 1st row
    
    mov     r13,[rdi+16]       ; Y/U/V pointers
    mov     r12,[rdi+8]
    mov     rdi,[rdi]

.L4:
    lea     r9,[r8+r14]       ; r9 = 2nd src row
    lea     rsi,[rdi+rdx]     ; rsi = 2nd dst row

.L5:

; mm1-mm4, the separate pixels
; mm5, sum(mm1-mm4)
; eax, the output register
; mm0, working copy
 
    pxor      mm7,mm7
    movd      mm1,[r8]
    punpcklbw mm1,mm7
    movd      mm2,[r8+4]
    punpcklbw mm2,mm7
    movd      mm3,[r9]
    punpcklbw mm3,mm7  
    movd      mm4,[r9+4]
    punpcklbw mm4,mm7  
    movq      mm5,mm1
    paddw     mm5,mm2
    paddw     mm5,mm3
    paddw     mm5,mm4
    
    movq      mm7,[yMul wrt rip]
    pmaddwd   mm1,mm7
    movq      mm0,mm1
    psrlq     mm0,32
    paddd     mm1,mm0
    movd      eax,mm1
    shr       eax,8
    add       eax,16
    mov       [rdi],al
    
    pmaddwd   mm2,mm7
    movq      mm0,mm2
    psrlq     mm0,32
    paddd     mm2,mm0
    movd      eax,mm2
    shr       eax,8
    add       eax,16
    mov       [rdi+1],al
    
    pmaddwd   mm3,mm7
    movq      mm0,mm3
    psrlq     mm0,32
    paddd     mm3,mm0 
    movd      eax,mm3
    shr       eax,8
    add       eax,16
    mov       [rsi],al
    
    pmaddwd   mm4,mm7
    movq      mm0,mm4
    psrlq     mm0,32
    paddd     mm4,mm0
    movd      eax,mm4
    shr       eax,8 
    add       eax,16
    mov       [rsi+1],al
    
    movq      mm7,[uMul wrt rip]
    movq      mm6,mm5
    pmaddwd   mm5,mm7
    movq      mm0,mm5
    psrlq     mm0,32
    paddd     mm5,mm0
    movd      eax,mm5 
    shr       eax,10
    add       eax,128
    mov       [r12],al
    
    movq      mm7,[vMul wrt rip]
    pmaddwd   mm6,mm7
    movq      mm0,mm6
    psrlq     mm0,32
    paddd     mm6,mm0
    movd      eax,mm6
    shr       eax,10
    add       eax,128
    mov       [r13],al
    
    add     r8,8
    add     r9,8
    
    add     rdi,2
    add     rsi,2
    
    inc     r12
    inc     r13
    
    cmp     r8,rcx
    jne    .L5

.L6:
    cmp     r15,r9            ; end of buffer?
    je     .L9
    mov     r8,r9             ; r8 = 1st src row
    mov     rdi,rsi           ; rdi = 1st dst row
    lea     rcx,[r8+r14]      ; rcx = end of 1st row
    jmp    .L4

.L9:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    
    ret

SECTION ".note.GNU-stack" noalloc noexec nowrite progbits
