.CODE
ApplyGammaCorrectionASM PROC
    ; Przyjmujemy argumenty:
    ; RCX - wska�nik do imageData
    ; RDX - liczba pikseli
    ; R8  - wska�nik do LUT

    push rbp
    mov rbp, rsp

    mov rbx, rdx             ; liczba pikseli
    mov rsi, r8              ; wska�nik do LUT

    ; SIMD: Za�aduj tablic� LUT do rejestr�w
    mov rax, rsi             ; Kopiujemy wska�nik LUT
    vmovdqu ymm1, ymmword ptr [rax]        ; Za�aduj pierwsze 32 bajty LUT
    vmovdqu ymm2, ymmword ptr [rax + 32]   ; Za�aduj kolejne 32 bajty LUT
    vmovdqu ymm3, ymmword ptr [rax + 64]   ; Za�aduj kolejne 32 bajty LUT
    vmovdqu ymm4, ymmword ptr [rax + 96]   ; Za�aduj ostatnie 32 bajty LUT

ProcessPixels:
    cmp rbx, 0
    je DonePixels            ; zako�cz, gdy wszystkie piksele zosta�y przetworzone

    ; Przetworzenie kana��w RGB
    movzx rax, byte ptr [rcx]       ; za�aduj warto�� czerwonego kana�u
    mov al, byte ptr [rsi + rax]    ; zastosuj LUT do czerwonego kana�u
    mov byte ptr [rcx], al          ; zapisz now� warto��

    movzx rax, byte ptr [rcx + 1]   ; za�aduj warto�� zielonego kana�u
    mov al, byte ptr [rsi + rax]    ; zastosuj LUT do zielonego kana�u
    mov byte ptr [rcx + 1], al      ; zapisz now� warto��

    movzx rax, byte ptr [rcx + 2]   ; za�aduj warto�� niebieskiego kana�u
    mov al, byte ptr [rsi + rax]    ; zastosuj LUT do niebieskiego kana�u
    mov byte ptr [rcx + 2], al      ; zapisz now� warto��

    ; Przejd� do nast�pnego piksela (3 bajty na piksel w formacie RGB)
    add rcx, 3
    dec rbx
    jmp ProcessPixels

DonePixels:
    pop rbp
    ret
ApplyGammaCorrectionASM ENDP
END
