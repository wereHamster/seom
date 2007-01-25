
BITS 32

SECTION .rodata
ALIGN 16
yMul: dw    25,      129,        66,      0
uMul: dw   112,      -74,       -38,      0
vMul: dw   -18,      -94,       112,      0

SECTION .text

; [esp+ 4] : buf
; [esp+ 8] : width
; [esp+12] : height
global __seomFrameResample: function
__seomFrameResample:
%define ps 20
    push    edi
    push    esi
    push    edx
    push    ecx
    push    ebx

    mov     edi, [esp+ps+ 4]
    mov     esi, [esp+ps+ 8]
    mov     edx, [esp+ps+12]
    
    imul    esi,4             ; esi = width in bytes
    imul    edx,esi           ; edx = size of buffer in bytes
    lea     ecx,[edi+edx]     ; ecx = end of buffer
    pxor    mm7,mm7
    mov     eax,edi           ; eax = 1st row
    lea     edx,[eax+esi]     ; edx = end of 1st row

.L4:
    lea     ebx,[eax+esi]     ; ebx = 2nd row

.L5:
    movd      mm0,[eax]
    punpcklbw mm0,mm7
    movd      mm1,[eax+4]
    punpcklbw mm1,mm7
    movd      mm2,[ebx]
    punpcklbw mm2,mm7
    movd      mm3,[ebx+4]
    punpcklbw mm3,mm7

    paddusw   mm0,mm1
    paddusw   mm0,mm2
    paddusw   mm0,mm3
    psrlw     mm0,2
    packuswb  mm0,mm7
    movd      [edi],mm0
    
    add     eax,8
    add     ebx,8
    add     edi,4
    
    cmp     eax,edx
    jne    .L5

.L6:
    cmp     ecx,ebx           ; end of buffer?
    je     .L9
    mov     eax,ebx           ; eax = 1st row
    lea     edx,[eax+esi]     ; edx = end of 1st row
    jmp    .L4

.L9:
    emms
    
    pop     ebx
    pop     ecx
    pop     edx
    pop     esi
    pop     edi
    
    ret


SECTION .text

; edi : out[3]
; esi : in
; edx : width
; ecx : height
global __seomFrameConvert: function
__seomFrameConvert:
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
    mov     ecx, [esp+ps+16]
    
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
    lea     ecx,[esi+edx]     ; ecx = end of 1st row

.L4:
    lea     ebp,[esi+edx]     ; ebp = esi + one row

.L5:

; mm1-mm4, the separate pixels
; mm5, sum(mm1-mm4)
; eax, the output register
; mm0, working copy
 
    pxor      mm7,mm7
    movd      mm1,[esi]
    punpcklbw mm1,mm7
    movd      mm2,[esi+4]
    punpcklbw mm2,mm7
    movd      mm3,[ebp]
    punpcklbw mm3,mm7  
    movd      mm4,[ebp+4]
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

SECTION ".note.GNU-stack" noalloc noexec nowrite progbits
