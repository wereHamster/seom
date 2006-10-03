
BITS 64

SECTION .rodata
ALIGN 16
yMul: dw    25,      129,        66,      0
uMul: dw   112,      -74,       -38,      0
vMul: dw   -18,      -94,       112,      0

SECTION .text
global seomConvert

; rdi : out[3]
; rsi : in
; rdx : width
; rcx : height
seomConvert:
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
    
    pxor    mm7,mm7

.L4:
    lea     r9,[r8+r14]       ; r9 = 2nd src row
    lea     rsi,[rdi+rdx]     ; rsi = 2nd dst row

.L5:

; mm1-mm4, the separate pixels
; mm5, sum(mm1-mm4)
; eax, the output register
; mm0, working copy
 
    pxor      mm7,mm7
    movq      mm1,[r8]
    punpcklbw mm1,mm7
    movq      mm2,[r8+4]
    punpcklbw mm2,mm7
    movq      mm3,[r9]
    punpcklbw mm3,mm7  
    movq      mm4,[r9+4]
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
