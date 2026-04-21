#include <stdint.h>

/* =====================================================
   I/O PORTS
===================================================== */

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* =====================================================
   IDT STRUCTURES
===================================================== */

struct IDTEntry
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IDTPointer
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

IDTEntry idt[256];
IDTPointer idt_ptr;
int cursor_pos = 4; // Commence après "Hi " (index 2 caractères = index 4 octets)
extern "C" void isr_stub();
extern "C" void keyboard_stub();

/* =====================================================
   IDT FUNCTIONS
===================================================== */

void set_idt_gate(int n, uint32_t handler)
{
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].selector = 0x08; // kernel code segment
    idt[n].zero = 0;
    idt[n].type_attr = 0x8E; // interrupt gate
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void load_idt()
{
    idt_ptr.limit = sizeof(IDTEntry) * 256 - 1;
    idt_ptr.base = (uint32_t)&idt;
    asm volatile("lidt %0" : : "m"(idt_ptr));
}

/* =====================================================
   PIC 8259
===================================================== */

void pic_remap()
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    outb(0x21, 0x20); // IRQ0 → 0x20
    outb(0xA1, 0x28); // IRQ8 → 0x28

    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void pic_unmask_irq(uint8_t irq)
{
    uint16_t port = (irq < 8) ? 0x21 : 0xA1;
    uint8_t mask = inb(port);
    mask &= ~(1 << (irq % 8));
    outb(port, mask);
}

/* =====================================================
   SERIAL (COM1)
===================================================== */

void serial_init()
{
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

int serial_empty()
{
    return inb(0x3F8 + 5) & 0x20;
}

void serial_putc(char c)
{
    while (!serial_empty())
    {
    }
    outb(0x3F8, c);
}

void serial_write(const char *s)
{
    while (*s)
        serial_putc(*s++);
}

/* =====================================================
   KEYBOARD
===================================================== */

char scancode_to_ascii(uint8_t sc)
{
    switch (sc)
    {
    case 0x1E:
        return 'a';
    case 0x30:
        return 'b';
    case 0x2E:
        return 'c';
    case 0x20:
        return 'd';
    case 0x12:
        return 'e';
    case 0x21:
        return 'f';
    case 0x22:
        return 'g';
    case 0x23:
        return 'h';
    case 0x17:
        return 'i';
    case 0x24:
        return 'j';
    case 0x25:
        return 'k';
    case 0x26:
        return 'l';
    case 0x32:
        return 'm';
    case 0x31:
        return 'n';
    case 0x18:
        return 'o';
    case 0x19:
        return 'p';
    case 0x10:
        return 'q';
    case 0x13:
        return 'r';
    case 0x1F:
        return 's';
    case 0x14:
        return 't';
    case 0x16:
        return 'u';
    case 0x2F:
        return 'v';
    case 0x11:
        return 'w';
    case 0x2D:
        return 'x';
    case 0x15:
        return 'y';
    case 0x2C:
        return 'z';
    case 0x39:
        return ' ';
    default:
        return 0;
    }
}

/* =====================================================
   KERNEL ENTRY
===================================================== */

extern "C" void kernel_main()
{
    char *vga = (char *)0xB8000;
    vga[0] = 'H';
    vga[1] = 0x0F;
    vga[2] = 'i';
    vga[3] = 0x0F;

    for (int i = 0; i < 256; i++)
        set_idt_gate(i, (uint32_t)isr_stub);

    set_idt_gate(0x21, (uint32_t)keyboard_stub);

    pic_remap();
    pic_unmask_irq(1);

    load_idt();
    asm volatile("sti");

    serial_init();
    serial_write("[kernel] ready\r\n");

    while (1)
        asm volatile("hlt");
}

/* =====================================================
   KEYBOARD INTERRUPT HANDLER
===================================================== */

extern "C" void keyboard_handler()
{
    uint8_t sc = inb(0x60);
    // 2. Traitement du Retour Arrière (Backspace)
    if (sc == 0x0E) // Scancode pour Retour arrière
    {
        // 🚨 Ne pas effacer le "Hi" initial (cursor_pos initialisé à 4)
        if (cursor_pos > 4)
        {
            // Décrémenter la position
            cursor_pos--;

            // Écrire un espace à la position précédente dans la mémoire VGA
            char *vga = (char *)0xB8000;
            vga[cursor_pos * 2] = ' ';      // Écrire un espace
            vga[cursor_pos * 2 + 1] = 0x0F; // Conserver la couleur

            // Également effacer sur la console série (optionnel)
            serial_putc('\b'); // Caractère de retour arrière (non toujours supporté par stdio)
            serial_putc(' ');
            serial_putc('\b');
        }
    }
    // 3. Traitement des Caractères Normaux (si ce n'est pas Retour Arrière)
    else
    {
        // Convertir le scancode en ASCII
        char c = scancode_to_ascii(sc);

        if (c)
        {
            serial_putc(c);
            // 🚨 Utilisation de la variable globale
            char *vga = (char *)0xB8000;
            vga[cursor_pos * 2] = c;
            vga[cursor_pos * 2 + 1] = 0x0F;
            cursor_pos++; // Incrémentation de la variable globale
        }
    }

    outb(0x20, 0x20); // EOI PIC
}
