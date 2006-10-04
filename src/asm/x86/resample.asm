
BITS 32

SECTION .text

; [esp+ 4] : buf
; [esp+ 8] : width
; [esp+16] : height
global seomResample:function
seomResample:
%define ps 20
    push    edi
    push    esi
    push    edx
    push    ecx
    push    ebx

    mov     edi, [esp+ps+ 4]
    mov     esi, [esp+ps+ 8]
    mov     edx, [esp+ps+16]
    
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
    cmp     ecx,eax           ; end of buffer?
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
