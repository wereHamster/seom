
BITS 32

SECTION .text

global seomCodecEncode:function
seomCodecEncode:
%define ps 20
    push    edi
    push    esi
    push    ebp
    push    ebx
    push    edx

    mov     edi, [esp+ps+ 4]
    mov     esi, [esp+ps+ 8]
    mov     ebp, [esp+ps+12]
    mov     ebx, [esp+ps+16]

    mov     dl,-32             ; bits to go
    
.loop:
    xor     ecx,ecx
    mov     cl,byte [esi]      ; load byte
    mov     ecx, [ebx+4*ecx]   ; load huffman code/length
    add        dl,cl              ; add code legth into 'dl'
    jl        .nostore           ; enough space in the register?

    sub     cl,dl              ; no, put in what fits in and adjust 'cl'
    sub     dl,32
    shld    eax,ecx,cl
    add     cl,dl
    bswap   eax
    mov     [edi],eax
    add     edi,4
    
.nostore:                      ; put the code into 'eax'
    shld    eax,ecx,cl
    
    add     esi,1              ; advance source by one byte
    cmp     esi,ebp            ; loop...
    jnz     .loop

    cmp     dl,-32             ; extra bits to flush?
    jle    .noextra
    
    mov     cl,dl
    neg     cl
    shl     eax,cl
    bswap   eax
    mov     [edi],eax
    add     edi,4

.noextra:
    mov     eax,edi            ; how many dwords did we encode?
    
    pop     edx
    pop     ebx
    pop     ebp
    pop     esi
    pop     edi
    
    ret

global seomCodecDecode:function
seomCodecDecode:
%define ps 16
    push    ebp
    push    edi
    push    esi
    push    ebx

    mov     edi,[esp+ps+4]
    mov     esi,[esp+ps+8]
    mov     ebp,[esp+ps+16]
    
    mov     eax,0

.loop:
    mov     ebx,eax
    mov     cl,al
    shr     ebx,5
    mov     edx,[esi+ebx*4]
    mov     ebx,[esi+ebx*4+4]
    bswap   edx
    bswap   ebx
    shld    edx,ebx,cl
    or      edx,1
    bsr     ebx,edx
    btr     edx,ebx
    mov     ebx,[ebp+1024+ebx*4]
    mov     cl,[ebx]
    shr     edx,cl
    movzx   ecx,byte [ebx+edx+1]
    movzx   ebx,byte [ebp+ecx]
    add     eax,ebx
    mov     [edi],cl

    add     edi,1
    cmp     edi,[esp+ps+12]
    jb     .loop

    pop     ebx
    pop     esi
    pop     edi
    pop     ebp
    
    ret
