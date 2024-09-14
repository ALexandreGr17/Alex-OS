#include "arch/i686/isr.h"
#include "io.h"
#include "irq.h"
#include "pic.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <vfs/vfs.h>

#define KEYBOARD_CONTROLLER_PORT	0x64
#define KEYBOARD_PORT				0x60

char keycode_translated[256];
bool caps_lock = false;
bool num_lock = false;

char is_printable(char c){
	return (c >= 'a' && c <= 'e') || 
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		(c == ',' || c == ';' || 
		 c == ':' || c == '!');
}

void keyboard_handler_main(Register* regs){
	uint8_t status;
	char keycode;
	
	i686_PIC_SendEOI(0x1);

	status = i686_inb(KEYBOARD_CONTROLLER_PORT);

	if(status & 0x1){
		keycode = i686_inb(KEYBOARD_PORT);
		if(keycode < 0){
			return;
		}
		if(keycode_translated[keycode] > 0){
			char c = keycode_translated[keycode];
			if(caps_lock && c >= 'a' && c <= 'z'){
				c -= 32;
			}

			switch (c) {
				case ',':
					c = '?';
					break;
				case ';':
					c = '.';
					break;
				case ':':
					c = '/';
					break;
			}

			if(c >= '0' && c <= '9' && !num_lock){
				return;
			}
			write(0, 1, &c);
			printf("%c", c);
		}
		else{
			if(keycode == 0x3a){
				caps_lock ^= 1;
				i686_outb(KEYBOARD_PORT, 0xED);
				uint8_t data = i686_inb(KEYBOARD_PORT);
				i686_outb(KEYBOARD_PORT, 2);
			}
			else if(keycode == 0x45){
				i686_outb(KEYBOARD_PORT, 0xED);
				uint8_t data = i686_inb(KEYBOARD_PORT);
				i686_outb(KEYBOARD_PORT, 1);
				num_lock ^= 1;
			}
			else {
				printf("0x%x", keycode);
			}
		}
	}
}

/*"azertyuiop"
"qsdfghjklm"
"wxcvbn"*/

void i686_Keyboard_init(){
	char* first_row = "azertyuiop";
	char* second_row = "qsdfghjklm";
	char* third_row = "wxcvbn";


	i686_IRQ_RegisterHandler(0x1, keyboard_handler_main);
	for(int i = 0; i < 256; i++){
		if(i >= 0x10 && i <= 0x19){
			keycode_translated[i] = first_row[i - 0x10];
		}
		else if (i >= 0x1e && i <= 0x27){
			keycode_translated[i] = second_row[i - 0x1e];
		}
		else if (i >= 0x2c && i <= 0x31){
			keycode_translated[i] = third_row[i - 0x2c];
		}
		else if (i >= 0x47 && i <= 0x49){
			keycode_translated[i] = '7' + (i - 0x47);
		}
		else if (i >= 0x4b && i <= 0x4d){
			keycode_translated[i] = '4' + (i - 0x4b);
		}
		else if (i >= 0x4f && i <= 0x51){
			keycode_translated[i] = '1' + (i - 0x4f);
		}
		else if (i >= 0x32 && i <= 0x35){
			switch (i){
				case 0x32:
					keycode_translated[i] = ',';
					break;
				case 0x33:
					keycode_translated[i] = ';';
					break;
				case 0x34:
					keycode_translated[i] = ':';
					break;;
				case 0x35:
					keycode_translated[i] = '!';
					break;
			}
		}
		else if (i == 0x56){
			keycode_translated[i] = '<';
		}
		else {
			keycode_translated[i] = -1;
		}
	}

	keycode_translated[0x39] = ' ';
	keycode_translated[0x1c] = '\n';
	keycode_translated[0x0f] = '\t';
	keycode_translated[0x52] = '0';
}

