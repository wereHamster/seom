
BITS 64

SECTION .text

; rdi : buf
; rsi : width
; rdx : height
global seomResample: function
seomResample:
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
