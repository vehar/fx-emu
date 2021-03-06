//ePS6800 Keypad Emulation

#include "kbd.hpp"
#include "mmio.hpp"
#include "cpu.hpp"

static uint8_t kbd_reg[0x40];
static uint8_t kbd_matrix[7];
static uint8_t onbit;

uint8_t kbd_get_matrix(uint8_t scan) {
	int i;
	uint8_t result = 0;
	for (i = 0; i < 7; i++) {
		if (!(scan & (1 << i))) result |= kbd_matrix[i];
	}
	result |= onbit;
	//printf("\nReturning result %02x Scan %02x\n", result, scan);
	return ~result;
}

uint8_t kbd_get_pb() {
	return ~((~kbd_reg[REG_PORTB]) & (~kbd_reg[REG_DCRB]));
}

uint8_t kbd_get_pa() {
	return kbd_get_matrix(kbd_get_pb()) & kbd_reg[REG_DCRA] & kbd_reg[REG_PACON];
}

void kbd_processint() {
	unsigned char pa;

	pa = ~kbd_get_pa();
	//printf("GETPB %02x, PB %02x, PBCON %02x, GETPA %02x, PAINTEN %02x\n", kbd_get_pb(), kbd_reg[REG_PORTB], kbd_reg[REG_PBCON], kbd_get_pa(), kbd_reg[REG_PAINTEN]);
	if (pa & kbd_reg[REG_PAINTEN]) {
		//printf("going to interrupt with %02x\n", pa);
		kbd_reg[REG_PAINTSTA] = pa & kbd_reg[REG_PAINTEN];
		cpu_interrupt(ADDR_PAINT);
	}
	if (pa & kbd_reg[REG_PAWAKE]) {
		cpu_wake(WAKE_PAINT);
	}
}

uint8_t kbd_read_byte(uint8_t addr) {
	uint8_t byte;
	if (addr < 0x40) {
		switch (addr) {
		case REG_PORTA:
			byte = ((kbd_get_pa()) |
					(kbd_reg[REG_PORTA] & (~kbd_reg[REG_DCRA])));
			mmio_bad_read_byte(addr);
			printf("GETPB %02x, PB %02x, DCRB %02x, GETPA %02x, PAINTEN %02x\n", kbd_get_pb(), kbd_reg[REG_PORTB], kbd_reg[REG_DCRB], kbd_get_pa(), kbd_reg[REG_PAINTEN]);
			printf("Reading PortA, returning %02x.\n", byte);
			break;
		case REG_PORTB:
			if (kbd_reg[REG_DCRB]) {
				printf("Reading PortB! Not emulated!\n");
				byte = 0xFF;
			} else {
				byte = kbd_reg[addr];
			}
			break;
		case REG_PORTC:
			printf("Port C Read, ��פ�\n");
			byte = kbd_reg[addr];
			break;
		default:
		byte = kbd_reg[addr];
		}
	}
	else {
		byte = 0xFF;
		mmio_bad_read_byte(addr);
	}
	return byte;
}

void kbd_write_byte(uint8_t addr, uint8_t byte) {
	if (addr < 0x40) {
		kbd_reg[addr] = byte;
		if (addr == REG_DCRB) {
			kbd_processint();
		}
	}
	else {
		mmio_bad_write_byte(addr);
	}
}

void kbd_keydown(uint8_t key) {
	int row = key / 7;
	int col = key % 7;
	kbd_matrix[row] |= 1 << col;
	//printf("PACON %02x, DCRA %02x, PAINTEN %02x, PAWAKE %02x", kbd_reg[REG_PACON], kbd_reg[REG_DCRA], kbd_reg[REG_PAINTEN], kbd_reg[REG_PAWAKE]);
	kbd_processint();
}

void kbd_keyup(uint8_t key) {
	int row = key / 7;
	int col = key % 7;
	kbd_matrix[row] &= ~(1 << col);
}

void kbd_ondown() {
	onbit = 0x80;
	if ((kbd_reg[REG_PACON] & (0x80)) & (kbd_reg[REG_DCRA] & (0x80))) {
		if (kbd_reg[REG_PAINTEN] & (0x80)) {
			kbd_reg[REG_PAINTSTA] |= (0x80);
			cpu_interrupt(ADDR_PAINT);
		}
		if (kbd_reg[REG_PAWAKE] & (0x80)) {
			cpu_wake(WAKE_PAINT);
		}
	}
}

void kbd_onup() {
	onbit = 0;
}

void kbd_reset() {
	int i;
	for (i = 0; i < 0x40; i++) kbd_reg[i] = 0;
	for (i = 0; i < 7; i++) kbd_matrix[i] = 0;
	kbd_reg[REG_DCRA] = 0xFF;
	kbd_reg[REG_DCRB] = 0xFF;
	kbd_reg[REG_DCRC] = 0xFF;
	onbit = 0;
}

void *kbd_save_state(size_t *size) {
	return NULL;
}

void kbd_load_state(void *state) {

}