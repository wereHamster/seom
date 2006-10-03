
BITS 64

SECTION .text

; rdi : out
; rsi : in
; rdx : width
; rcx : height
global seomResample: function
seomResample:
    imul    rdx,4             ; rdx = width in bytes
    imul    rcx,rdx           ; rcx = size of buffer in bytes
    lea     r11,[rsi+rcx]     ; r11 = end of buffer
    pxor    mm7,mm7
    lea     rcx,[rsi+rdx]     ; rcx = end of 1st row

.L4:
    lea     rax,[rsi+rdx]     ; rax = rsi + one row

.L5:
    movd      mm0,[rsi]
    punpcklbw mm0,mm7
    movd      mm1,[rsi+4]
    punpcklbw mm1,mm7
    movd      mm2,[rax]
    punpcklbw mm2,mm7
    movd      mm3,[rax+4]
    punpcklbw mm3,mm7

    paddusw   mm0,mm1
    paddusw   mm0,mm2
    paddusw   mm0,mm3
    psrlw     mm0,2
    packuswb  mm0,mm7
    movd      [rdi],mm0
    
    add     rsi,8
    add     rax,8
    add     rdi,4
    
    cmp     rsi,rcx
    jne    .L5

.L6:
    cmp     r11,rax           ; end of buffer?
    je     .L9
    mov     rsi,rax           ; rsi = 1st row
    lea     rcx,[rsi+rdx]     ; rcx = end of 1st row
    jmp    .L4

.L9:
    emms
    ret
