
[BITS 32]                            ;indique que le code est en mode protégé 32 bits
global multiboot_header              ;rend le symbole visible pour l’éditeur de liens
align 4                              ;aligne l’en-tête sur 4 octets (exigence Multiboot)
multiboot_header:
    dd 0x1BADB002                    ;magic number Multiboot (signature officielle)
    dd 0x00                          ;flags (aucune option demandée au bootloader)
    dd -(0x1BADB002 + 0x00)          ;checksum


[BITS 32]
global _start

extern kernel_main

_start:

    ;----------------------------------------
    ; 2. Appeler le kernel C++
    ;----------------------------------------
    call kernel_main

hang:
    hlt  ;garantit que le CPU reste dans un état stable
    jmp hang



BITS 32
global keyboard_stub
extern keyboard_handler

keyboard_stub:
    pusha                      ;sauvegarde tous les registres
    call keyboard_handler      ;appelle le handler C++
    popa                       ;restaure les registres
    mov al, 0x20
    out 0x20, al               ;informe le PIC que l’interruption est traitée (EOI)
    iret                       ;retour d’interruption

[BITS 32]
global isr_stub


isr_stub:
    pusha
    popa
    iret