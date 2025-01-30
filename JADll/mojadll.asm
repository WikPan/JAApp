.CODE
ApplyGammaCorrectionASM PROC
    ; Przyjmujemy argumenty:
    ; RCX - wskaŸnik do imageData
    ; RDX - liczba pikseli
    ; R8  - wskaŸnik do LUT

    push rbp
    mov rbp, rsp

    mov rbx, rdx             ; liczba pikseli
    mov rsi, r8              ; wskaŸnik do LUT

    ; SIMD: Za³aduj tablicê LUT do rejestrów
    mov rax, rsi             ; Kopiujemy wskaŸnik LUT
    vmovdqu ymm1, ymmword ptr [rax]        ; Za³aduj pierwsze 32 bajty LUT
    vmovdqu ymm2, ymmword ptr [rax + 32]   ; Za³aduj kolejne 32 bajty LUT
    vmovdqu ymm3, ymmword ptr [rax + 64]   ; Za³aduj kolejne 32 bajty LUT
    vmovdqu ymm4, ymmword ptr [rax + 96]   ; Za³aduj ostatnie 32 bajty LUT

ProcessPixels:
    cmp rbx, 0
    je DonePixels            ; zakoñcz, gdy wszystkie piksele zosta³y przetworzone

    ; Przetworzenie kana³ów RGB
    movzx rax, byte ptr [rcx]       ; za³aduj wartoœæ czerwonego kana³u
    mov al, byte ptr [rsi + rax]    ; zastosuj LUT do czerwonego kana³u
    mov byte ptr [rcx], al          ; zapisz now¹ wartoœæ

    movzx rax, byte ptr [rcx + 1]   ; za³aduj wartoœæ zielonego kana³u
    mov al, byte ptr [rsi + rax]    ; zastosuj LUT do zielonego kana³u
    mov byte ptr [rcx + 1], al      ; zapisz now¹ wartoœæ

    movzx rax, byte ptr [rcx + 2]   ; za³aduj wartoœæ niebieskiego kana³u
    mov al, byte ptr [rsi + rax]    ; zastosuj LUT do niebieskiego kana³u
    mov byte ptr [rcx + 2], al      ; zapisz now¹ wartoœæ

    ; PrzejdŸ do nastêpnego piksela (3 bajty na piksel w formacie RGB)
    add rcx, 3
    dec rbx
    jmp ProcessPixels

DonePixels:
    pop rbp
    ret
ApplyGammaCorrectionASM ENDP
END
