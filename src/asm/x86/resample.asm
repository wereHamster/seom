
BITS 32

SECTION .text
global seomResample

; edi : out
; esi : in
; edx : width
; ecx : height
seomResample:
%define ps 20
    push    edi
    push    esi
    push    edx
    push    ecx
    push    ebx

    mov     edi, [esp+ps+ 4]
    mov     esi, [esp+ps+ 8]
    mov     edx, [esp+ps+12]
    mov     ecx, [esp+ps+20]
    
    imul    edx,4             ; edx = width in bytes
    imul    ecx,edx           ; ecx = size of buffer in bytes
    lea     ebx,[esi+ecx]     ; ebx = end of buffer
    pxor    mm7,mm7
    lea     ecx,[esi+edx]     ; ecx = end of 1st row

.L4:
    lea     eax,[esi+edx]    ; eax = esi + one row

.L5:
    movd      mm0,[esi]
    punpcklbw mm0,mm7
    movd      mm1,[esi+4]
    punpcklbw mm1,mm7
    movd      mm2,[eax]
    punpcklbw mm2,mm7
    movd      mm3,[eax+4]
    punpcklbw mm3,mm7

    paddusw   mm0,mm1     
    paddusw   mm0,mm2     
    paddusw   mm0,mm3     
    psrlw     mm0,2
    packuswb  mm0,mm7
    movd      [edi],mm0        
    
    add     esi,8
    add     eax,8
    add     edi,4
    
    cmp     esi,ecx
    jne    .L5

.L6:
    cmp     ebx,eax           ; end of buffer?
    je     .L9
    mov     esi,eax           ; esi = 1st row
    lea     ecx,[esi+edx]     ; ecx = end of 1st row
    jmp    .L4

.L9:
    emms
    
    pop     ebx
    pop     ecx
    pop     edx
    pop     esi
    pop     edi
    
    ret
