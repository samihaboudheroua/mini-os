; boot.asm - bootloader 16 bits
[org 0x7C00]

    ; Effacer l'écran (optionnel)
    mov ax, 0x0003
    int 0x10

    ; Afficher un message
    mov si, message
print_char:
    lodsb
    or al, al
    jz done_print
    mov ah, 0x0E
    int 0x10
    jmp print_char
done_print:

    ; -----------------------------
    ; Charger le kernel (secteurs) en mémoire à 0x10000
    ; DL contient le numéro du lecteur (fourni par le BIOS)
    ; On lit 32 secteurs à partir du secteur 2 (juste après le boot sector)
    ; en utilisant CHS avec 2 têtes et 18 secteurs par piste
    ; -----------------------------
    cli
    mov ax, 0x1000
    mov es, ax      ; ES = 0x1000
    xor bx, bx      ; BX = 0x0000 (offset)

    mov bp, 32      ; nombre de secteurs à lire (counter)
    mov ch, 0       ; cylinder = 0
    mov cl, 2       ; sector = 2 (start)
    xor dh, dh      ; head = 0
    mov si, 0       ; retry counter

read_sector:
    mov ah, 0x02    ; BIOS read sectors
    mov al, 0x01    ; read 1 sector
    int 0x13
    jc read_error

    ; advance ES:BX by 512 (no wrap handling: total size < 64 KiB)
    add bx, 512

cont_next:
    ; increment sector/head/cylinder
    inc cl
    cmp cl, 19
    jl no_head_inc
    mov cl, 1
    inc dh
    cmp dh, 2
    jl no_cyl_inc
    xor dh, dh
    inc ch
no_cyl_inc:
no_head_inc:

    dec bp
    jnz read_sector
    jmp read_done

read_error:
    ; simple retry mechanism
    inc si
    cmp si, 3
    jle read_sector
    ; if persistent error, halt
    hlt
    jmp $

read_done:
    sti

    ; Passer en protected mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode

protected_mode:
    [bits 32]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; Saut vers le kernel (que l'on placera à 0x10000)
    jmp 0x10000

message db "Booting mini OS...", 0

; ---------- GDT ----------
gdt_start:
    dq 0                ; NULL
    dq 0x00CF9A000000FFFF ; Code segment
    dq 0x00CF92000000FFFF ; Data segment
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; Boot signature (obligatoire)
times 510 - ($ - $$) db 0
dw 0xAA55
