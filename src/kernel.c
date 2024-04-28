#define COLM 80
#define ROW 25
#define IDTSIZE 256
#define KERNELCODESEGOFFSET 0x8
#define IDTINTGATE32BIT 0x8e
#define PIC1_COMMAND_PORT 0x20
#define PIC1_DATA_PORT 0x21
#define PIC2_COMMAND_PORT 0xA0
#define PIC2_DATA_PORT 0xA1
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#include "map.h"

extern void print_char_with_asm(char c, int row, int col);
extern void load_gdt();
extern void keyboard_handler();
extern char ioport_in(unsigned short port);
extern void ioport_out(unsigned short port, unsigned char data);
extern void load_idt(unsigned int* idt_address);
extern void enable_interrupts();

struct IDT_pointer {
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));
struct IDT_entry {
	unsigned short offset_lowerbits; // 16 bits
	unsigned short selector; // 16 bits
	unsigned char zero; // 8 bits
	unsigned char type_attr; // 8 bits
	unsigned short offset_upperbits; // 16 bits
} __attribute__((packed));

struct IDT_entry IDT[IDTSIZE];

int cursor_pos = 0;

void init_idt() {
	// Get the address of the keyboard_handler code in kernel.asm as a number
	unsigned int offset = (unsigned int)keyboard_handler;
	// Populate the first entry of the IDT
	// TODO why 0x21 and not 0x0?
	// My guess: 0x0 to 0x2 are reserved for CPU, so we use the first avail
	IDT[0x21].offset_lowerbits = offset & 0x0000FFFF; // lower 16 bits
	IDT[0x21].selector = KERNELCODESEGOFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = IDTINTGATE32BIT;
	IDT[0x21].offset_upperbits = (offset & 0xFFFF0000) >> 16;
    ioport_out(PIC1_COMMAND_PORT, 0x11);
	ioport_out(PIC2_COMMAND_PORT, 0x11);

    ioport_out(PIC1_DATA_PORT, 0x20);
	ioport_out(PIC2_DATA_PORT, 0x28);

    ioport_out(PIC1_DATA_PORT, 0x0);
	ioport_out(PIC2_DATA_PORT, 0x0);

    ioport_out(PIC1_DATA_PORT, 0x1);
	ioport_out(PIC2_DATA_PORT, 0x1);

    ioport_out(PIC1_DATA_PORT, 0xff);
	ioport_out(PIC2_DATA_PORT, 0xff);

    struct IDT_pointer idt_ptr;
	idt_ptr.limit = (sizeof(struct IDT_entry) * IDTSIZE) - 1;
	idt_ptr.base = (unsigned int) &IDT;
	// Now load this IDT
	load_idt(&idt_ptr);
}

void kb_init() {
	// 0xFD = 1111 1101 in binary. enables only IRQ1
	// Why IRQ1? Remember, IRQ0 exists, it's 0-based
	ioport_out(PIC1_DATA_PORT, 0xFD);
}


void handle_keyboard_interrupt() {
	// Write end of interrupt (EOI)
	ioport_out(PIC1_COMMAND_PORT, 0x20);

	unsigned char status = ioport_in(KEYBOARD_STATUS_PORT);
	// Lowest bit of status will be set if buffer not empty
	// (thanks mkeykernel)
	if (status & 0x1) {
		char keycode = ioport_in(KEYBOARD_DATA_PORT);
		if (keycode < 0 || keycode >= 128) return; // how did they know keycode is signed?
		print_char_with_asm(keyboard_map[keycode],0,cursor_pos);
		cursor_pos++;
	}
}


void print_message() {
	// Fill the screen with 'x'
	int i, j;
	for (i = 0; i < COLM; i++) {
		for (j = 0; j < ROW; j++) {
			print_char_with_asm('*',j,i);
		}
	}
	print_char_with_asm('-',0,0);
	print_char_with_asm('P',0,1);
	print_char_with_asm('K',0,2);
	print_char_with_asm('O',0,3);
	print_char_with_asm('S',0,4);
	print_char_with_asm('-',0,5);
}

void main() {
	print_message();
	// load_gdt();
	init_idt();
	kb_init();
	enable_interrupts();
	// Finish main execution, but don't halt the CPU. Same as `jmp $` in assembly
	while(1);
}








