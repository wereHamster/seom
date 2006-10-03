
BITS 32

SECTION .rodata
ALIGN 16
yMul: dw    25,      129,        66,      0
uMul: dw   112,      -74,       -38,      0
vMul: dw   -18,      -94,       112,      0

SECTION .text
global seomConvert

; edi : out[3]
; esi : in
; edx : width
; ecx : height
seomConvert:
%define ps 24
    push    edi
    push    esi
    push    edx
    push    ecx
    push    ebx
    push    ebp

    mov     edi, [esp+ps+ 4]
    mov     esi, [esp+ps+ 8]
    mov     edx, [esp+ps+12]
    mov     ecx, [esp+ps+20]
    
    mov     edi,[esp+ps+ 4]
    mov     edi,[edi]
    mov     [esp-4],edi
    
    mov     edi,[esp+ps+ 4]
    mov     edi,[edi]
    add     edi,edx
    mov     [esp-8],edi
    
    mov     edi,[esp+ps+ 4]
    mov     edi,[edi+4]
    mov     [esp-12],edi
    
    mov     edi,[esp+ps+ 4]
    mov     edi,[edi+8]
    mov     [esp-16],edi
    
    imul    edx,4             ; edx = width in bytes
    imul    ecx,edx           ; ecx = size of buffer in bytes
    lea     ebx,[esi+ecx]     ; ebx = end of buffer
    pxor    mm7,mm7
    lea     ecx,[esi+edx]     ; ecx = end of 1st row

.L4:
    lea     ebp,[esi+edx]     ; ebp = esi + one row

.L5:

; mm1-mm4, the separate pixels
; mm5, sum(mm1-mm4)
; eax, the output register
; mm0, working copy
 
    pxor      mm7,mm7
    movq      mm1,[esi]
    punpcklbw mm1,mm7
    movq      mm2,[esi+4]
    punpcklbw mm2,mm7
    movq      mm3,[ebp]
    punpcklbw mm3,mm7  
    movq      mm4,[ebp+4]
    punpcklbw mm4,mm7  
    movq      mm5,mm1
    paddw     mm5,mm2
    paddw     mm5,mm3
    paddw     mm5,mm4
    
    mov       edi,[esp-4]
    
    movq      mm7,[yMul]                
    pmaddwd   mm1,mm7
    movq      mm0,mm1
    psrlq     mm0,32
    paddd     mm1,mm0
    movd      eax,mm1
    shr       eax,8
    add       eax,16
    mov       [edi],al
    
    pmaddwd   mm2,mm7
    movq      mm0,mm2
    psrlq     mm0,32
    paddd     mm2,mm0
    movd      eax,mm2
    shr       eax,8
    add       eax,16
    mov       [edi+1],al
    
    add       edi,2
    mov       [esp-4],edi
    
    mov       edi,[esp-8]
    
    pmaddwd   mm3,mm7
    movq      mm0,mm3
    psrlq     mm0,32
    paddd     mm3,mm0 
    movd      eax,mm3
    shr       eax,8
    add       eax,16
    mov       [edi],al
    
    pmaddwd   mm4,mm7
    movq      mm0,mm4
    psrlq     mm0,32
    paddd     mm4,mm0
    movd      eax,mm4
    shr       eax,8 
    add       eax,16
    mov       [edi+1],al
    
    add       edi,2
    mov       [esp-8],edi
    
    movq      mm7,[uMul]            
    movq      mm6,mm5
    pmaddwd   mm5,mm7
    movq      mm0,mm5
    psrlq     mm0,32
    paddd     mm5,mm0
    movd      eax,mm5 
    shr       eax,10     
    add       eax,128
    mov       edi,[esp-12]
    mov       [edi],al
    add       edi,1
    mov       [esp-12],edi
    
    movq      mm7,[vMul]         
    pmaddwd   mm6,mm7
    movq      mm0,mm6        
    psrlq     mm0,32
    paddd     mm6,mm0
    movd      eax,mm6   
    shr       eax,10       
    add       eax,128
    mov       edi,[esp-16]
    mov       [edi],al
    add       edi,1
    mov       [esp-16],edi
    
    add     esi,8
    add     ebp,8
    
    cmp     esi,ecx
    jne    .L5

.L6:
    cmp     ebx,ebp           ; end of buffer?
    je     .L9
    mov     esi,ebp           ; esi = 1st row
    lea     ecx,[esi+edx]     ; ecx = end of 1st row
    mov     edi,[esp-8]
    mov     [esp-4],edi
    add     edi,[esp+ps+12]
    mov     [esp-8],edi
    jmp    .L4

.L9:
    emms
    
    pop     ebp
    pop     ebx
    pop     ecx
    pop     edx
    pop     esi
    pop     edi
    
    ret
