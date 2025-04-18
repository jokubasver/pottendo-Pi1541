// Pi1541 - A Commodore 1541 disk drive emulator
// Copyright(C) 2018 Stephen White
//
// This file is part of Pi1541.
// 
// Pi1541 is free software : you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Pi1541 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Pi1541. If not, see <http://www.gnu.org/licenses/>.

#ifndef IEC_BUS_H
#define IEC_BUS_H

#include "defs.h"
#include "debug.h"
#include "m6522.h"
#include "m8520.h"

#include "rpi-gpio.h"
#if !defined(__CIRCLE__) && !defined(__PICO2__)
#include "rpiHardware.h"
#endif
#if defined(__PICO2__)
#include "pico/stdlib.h"
#include "hardware/pio.h"
#endif
//ROTARY: Added for rotary encoder support - 09/05/2019 by Geo...
#include "dmRotary.h"

#define INPUT_BUTTON_DEBOUNCE_THRESHOLD 20000
#define INPUT_BUTTON_REPEAT_THRESHOLD 460000

/* moved to variables in InputMapping
#define INPUT_BUTTON_ENTER 0
#define INPUT_BUTTON_UP 1
#define INPUT_BUTTON_DOWN 2
#define INPUT_BUTTON_BACK 3
#define INPUT_BUTTON_INSERT 4
*/

// DIN ATN is inverted and then connected to pb7 and ca1
//	- also input to xor with ATNAout pb4
//		- output of xor is inverted and connected to DIN pin 5 DATAout (OC so can only pull low)
// VIA ATNin pb7 input to xor
// VIA ATNin ca1 output from inverted DIN 3 ATN
// DIN DATA is inverted and then connected to pb1
// VIA DATAin pb0 output from inverted DIN 5 DATA
// VIA DATAout pb1 inverted and then connected to DIN DATA
// DIN CLK is inverted and then connected to pb2
// VIA CLKin pb2 output from inverted DIN 4 CLK
// VIA CLKout pb3 inverted and then connected to DIN CLK

// $1800
// PB 0		data in
// PB 1		data out
// PB 2		clock in
// PB 3		clock out
// PB 4		ATNA
// PB 5,6	device address
// PB 7,CA1	ATN IN

// If ATN and ATNA are out of sync
//	- VIA's DATA IN will automatically set high and hence the DATA line will be pulled low (activated)
// If ATN and ATNA are in sync
//	- the output from the XOR gate will be low and the output of its inverter will go high
//	- when this occurs the DATA line must be still able to be pulled low via the PC or VIA's inverted PB1 (DATA OUT)
//
// Therefore in the same vein if PB7 is set to output it could cause the input of the XOR to be pulled low
//

// NOTE ABOUT SRQ
// SRQ is a little bit different.
// The 1581 does not pull it high. Only the 128 pulls it high.
// 
#if defined(HAS_40PINS)
enum PIGPIO
{
#if defined(__PICO2__)	
	// Original Non-split lines	
	PIGPIO_ATN = 10,
	PIGPIO_CLOCK = 11,
	PIGPIO_DATA = 12,
	PIGPIO_RESET = 13,
	PIGPIO_SRQ = 14,
	PIGPIO_IN_BUTTON1 = 5,
	PIGPIO_IN_BUTTON2 = 6,
	PIGPIO_IN_BUTTON3 = 7,
	PIGPIO_IN_BUTTON4 = 8,
	PIGPIO_IN_BUTTON5 = 9,
	PIGPIO_OUT_SOUND = 14,
	PIGPIO_OUT_LED   = 15,
#elif defined(ESP32)
	PIGPIO_ATN = GPIO_NUM_3,
	PIGPIO_CLOCK = GPIO_NUM_15,
	PIGPIO_DATA = GPIO_NUM_4,
	PIGPIO_RESET = GPIO_NUM_19,
	PIGPIO_SRQ = 14,
	PIGPIO_IN_BUTTON1 = 5,
	PIGPIO_IN_BUTTON2 = 6,
	PIGPIO_IN_BUTTON3 = 7,
	PIGPIO_IN_BUTTON4 = 8,
	PIGPIO_IN_BUTTON5 = 9,
	PIGPIO_OUT_SOUND = GPIO_NUM_26,
	PIGPIO_OUT_LED   = GPIO_NUM_14,

#else
	// Original Non-split lines	
	PIGPIO_ATN = 2,			// 3
	PIGPIO_CLOCK = 17,		// 11
	PIGPIO_DATA = 18,		// 12
	PIGPIO_SRQ = 19,		// 35
	PIGPIO_RESET = 3,		// 5

	// Pinout for those that want to split the lines (and the common ones like buttons, sound and LED)
	// 0 IDSC			//28
	// 1 IDSD			//27
	// 2 I2C_SDA		//3
	// 3 I2C_CLK		//5
	PIGPIO_IN_BUTTON4 = 4,	// 07 Common
	PIGPIO_IN_BUTTON5 = 9,	// 29 Common
	//PIGPIO_OUT_RESET = 6,	// 31 
	PIGPIO_OUT_SPI0_RS = 6,	// 31 
	// 7 SPI0_CS1		//26
	// 8 SPI0_CS0		//24
	// 9 SPI0_MISO		//21
	// 10 SPI0_MOSI		//19
	// 11 SPI0_CLK		//23
	PIGPIO_OUT_ATN = 12,	// 32
	PIGPIO_OUT_SOUND = 13,	// 33 Common
	// 14 TX			//8
	// 15 RX			//10
	PIGPIO_OUT_LED = 8,	// 36 Common
	PIGPIO_OUT_CLOCK = 17,	// 11
	PIGPIO_OUT_DATA = 18,	// 12
	PIGPIO_OUT_SRQ = 19,	// 35
	PIGPIO_IN_RESET = 20,	// 38
	PIGPIO_IN_SRQ = 21,		// 40
	PIGPIO_IN_BUTTON2 = 15,	// 15 Common
	PIGPIO_IN_BUTTON3 = 10,	// 16 Common
	PIGPIO_IN_ATN = 24,		// 18
	PIGPIO_IN_DATA = 25,	// 22
	PIGPIO_IN_CLOCK = 26,	// 37
	PIGPIO_IN_BUTTON1 = 14	// 13 Common
#endif
};
#else/* HAS4-PINS */
//Added GPIO bindings for Raspberry Pi 1B Rev 1/2 (only 26 I/O ports)
enum PIGPIO
{
	// Original Non-split lines
	// Raspberry Pi 1B Rev 2 has GPIO0/1 in place of GPIO2/3
#if defined(RPI1BREV1)	
	PIGPIO_ATN = 0,			// 3
	PIGPIO_RESET = 1,		// 5
#else
	PIGPIO_ATN = 2,			// 3
	PIGPIO_RESET = 3,		// 5
#endif
	PIGPIO_CLOCK = 17,		// 11
	PIGPIO_DATA = 18,		// 12
	PIGPIO_SRQ = 19,		// 35  not connected yet

	// Pinout for those that want to split the lines (and the common ones like buttons, sound and LED)
	// Funktion = 	GPIO	// Hardware PIN
	// 0 IDSC				// 28
	// 1 IDSD				// 27
	// 2 I2C_SDA			// 3
	// 3 I2C_CLK			// 5
	PIGPIO_IN_BUTTON4 = 4,	// 07 Common
	//GPIO = 5,				// 29 Common
	//PIGPIO_OUT_RESET = 6,	// 31
	PIGPIO_OUT_SPI0_RS = 6,	// 31 not connected yet
	// 7 SPI0_CS1			// 26
	PIGPIO_IN_BUTTON5 =  9,	// 21
	PIGPIO_IN_RESET = 20,	// 38
	PIGPIO_IN_CLOCK = 10,	// 19
	PIGPIO_OUT_LED = 7,	    // 26
	PIGPIO_OUT_ATN = 12,	// 32 not connected yet
	PIGPIO_OUT_SOUND = 11,  // 23
	// 14 TX				// 8
	// 15 RX  				// 10
	//GPIO = 16,			// 36 Common
	PIGPIO_OUT_CLOCK = 17,	// 11
	PIGPIO_OUT_DATA = 18,	// 12
	PIGPIO_OUT_SRQ = 19,	// 35 not connected yet
	//GPIO 20,				// 38
	PIGPIO_IN_SRQ = 21,		// 40 not connected yet
	PIGPIO_IN_BUTTON2 = 22,	// 15 Common
	PIGPIO_IN_BUTTON3 = 23,	// 16 Common
	PIGPIO_IN_ATN = 24,		// 18
	PIGPIO_IN_DATA = 25,	// 22
	//GPIO 26,				// 37
#if defined(RPI1BREV1)
	PIGPIO_IN_BUTTON1 = 21	// 13 Common
#else
	PIGPIO_IN_BUTTON1 = 27	// 13 Common
#endif
};
#endif /* HAS40PINS */

enum PIGPIOMasks
{
	// Non-split lines
	//PIGPIO_MASK_IN_ATN = 1 << PIGPIO_ATN,
	//PIGPIO_MASK_IN_DATA = 1 << PIGPIO_DATA,
	//PIGPIO_MASK_IN_CLOCK = 1 << PIGPIO_CLOCK,
	//PIGPIO_MASK_IN_SRQ = 1 << PIGPIO_SRQ,
	//PIGPIO_MASK_IN_RESET = 1 << PIGPIO_RESET,

	// Split lines
	//PIGPIO_MASK_IN_ATN = 1 << PIGPIO_IN_ATN,
	//PIGPIO_MASK_IN_DATA = 1 << PIGPIO_IN_DATA,
	//PIGPIO_MASK_IN_CLOCK = 1 << PIGPIO_IN_CLOCK,
	//PIGPIO_MASK_IN_SRQ = 1 << PIGPIO_IN_SRQ,
	//PIGPIO_MASK_IN_RESET = 1 << PIGPIO_IN_RESET,

	PIGPIO_MASK_IN_BUTTON1 = 1 << PIGPIO_IN_BUTTON1,
	PIGPIO_MASK_IN_BUTTON2 = 1 << PIGPIO_IN_BUTTON2,
	PIGPIO_MASK_IN_BUTTON3 = 1 << PIGPIO_IN_BUTTON3,
	PIGPIO_MASK_IN_BUTTON4 = 1 << PIGPIO_IN_BUTTON4,
	PIGPIO_MASK_IN_BUTTON5 = 1 << PIGPIO_IN_BUTTON5,
	PIGPIO_MASK_ANY_BUTTON = PIGPIO_MASK_IN_BUTTON1 | PIGPIO_MASK_IN_BUTTON2 | PIGPIO_MASK_IN_BUTTON3 | PIGPIO_MASK_IN_BUTTON4 | PIGPIO_MASK_IN_BUTTON5
};

static const unsigned ButtonPinFlags[5] = { PIGPIO_MASK_IN_BUTTON1, PIGPIO_MASK_IN_BUTTON2, PIGPIO_MASK_IN_BUTTON3, PIGPIO_MASK_IN_BUTTON4, PIGPIO_MASK_IN_BUTTON5 };
static const unsigned ButtonPins[5] = { PIGPIO_IN_BUTTON1, PIGPIO_IN_BUTTON2, PIGPIO_IN_BUTTON3, PIGPIO_IN_BUTTON4, PIGPIO_IN_BUTTON5 };

#if !defined(__PICO2__) && !defined(ESP32)
///////////////////////////////////////////////////////////////////////////////////////////////
// Original Non-split lines
///////////////////////////////////////////////////////////////////////////////////////////////
// I kind of stuffed up selecting what pins should be what.
// Originally I wanted the hardware interface to be as simple as possible and was focused on all the connections down one end of the Pi header.
// Also, originally I was only worried about ATN, DATA, CLOCK and RESET. Buttons were optional. With DATA and CLOCK only needing to go output.
// With hindsight (now wanting to support NIBTools ATN should have been put in the same pin group as DATA and CLOCK);
// This now requires 2 GPIO writes as PIN2 is in SEL0 and pins 17 and 18 are in SEL1.
//GPFSEL0
//	9	8	7	6	5	4	3	2	1	0
// 000 000 000 000 000 000 000 001 000 000
// To support NIBTools ATN can be made an output as well
static const u32 PI_OUTPUT_MASK_GPFSEL0 = ~((1 << (PIGPIO_ATN * 3)));
//GPFSEL1           RX  TX
//	19	18	17	16	15	14	13	12	11	10
// 000 001 001 000 000 000 000 000 000 000	(bits 21 and 24 for GPIO 17 and 18 as outputs)
//static const u32 PI_OUTPUT_MASK_GPFSEL1 = ~((1 << 21) | (1 << 24));
// 000 001 001 001 000 000 001 000 000 000	(bits 21 and 24 for GPIO 17 and 18 as outputs)
//static const u32 PI_OUTPUT_MASK_GPFSEL1 = ~((1 << ((PIGPIO_OUT_SOUND - 10) * 3)) | (1 << ((PIGPIO_OUT_LED - 10) * 3)) | (1 << ((PIGPIO_CLOCK - 10) * 3)) | (1 << ((PIGPIO_DATA - 10) * 3)));
static const u32 PI_OUTPUT_MASK_GPFSEL1 = ~((1 << ((PIGPIO_CLOCK - 10) * 3)) | (1 << ((PIGPIO_DATA - 10) * 3)));
///////////////////////////////////////////////////////////////////////////////////////////////
#endif
///////////////////////////////////////////////////////////////////////////////////////////////
// Pinout for those that want to split the lines (and the common ones like buttons, sound and LED)
///////////////////////////////////////////////////////////////////////////////////////////////
// OUT (need 7)
//  CLK
//  DATA
//  SRQ
//  ATN
//  RST
//  LED
//  SND

// IN (need 10)
//  CLK
//  DATA
//  ATN
//  RST
//  SRQ 
//  BTN1
//  BTN2
//  BTN3
//  BTN4
//  BTN5

//GPFSEL0
//  S   S   S                    
//  P   P   P               I   I
//  I   I   I               2   2
//  0   0   0               C   C
//  M   C   C               S   S   I   I
//  I   E   E               C   D   D   D
//  S   0   1   W   R   R   L   A   S   S
//  O           RST B5  B4  1   1   D   C
//	9	8	7	6	5	4	3	2	1	0
//GPFSEL1
//                  RX  TX          S   S
//                  RX  TX          P   P
//                  RX  TX          I   I
//                  RX  TX          0   0
//                  RX  TX              M
//                  RX  TX          C   O
//  W   W   W   W   RX  TX  W   W   L   S
//  SRQ DTA CLK LED RX  TX  SND ATN K   I
//	19	18	17	16	15	14	13	12	11	10
//GPFSEL2
//  X   X                           
//  X   X                           
//  X   X                           
//  X   X                           
//  X   X                           
//  X   X                           
//  X   X   R   R   R   R    R   R  R   R 
//  X   X   B1  CLK DTA ATN B3  B2  SRQ RST  
//	29	28	27	26	25	24	23	22	21	20
// 000 000 000 000 000 000 000 000 000 000
///////////////////////////////////////////////////////////////////////////////////////////////

enum VIAPortPins
{
	VIAPORTPINS_DATAIN = 0x01,	//pb0
	VIAPORTPINS_DATAOUT = 0x02,	//pb1
	VIAPORTPINS_CLOCKIN = 0x04,	//pb2
	VIAPORTPINS_CLOCKOUT = 0x08,//pb3
	VIAPORTPINS_ATNAOUT = 0x10,	//pb4
	VIAPORTPINS_ATNIN = 0x80	//bp7
};

typedef bool(*CheckStatus)();

class IEC_Bus
{
#if defined (__CIRCLE__)
	static CGPIOPin IO_led;
	static CGPIOPin IO_sound;
	static CGPIOPin IO_ATN;
	static CGPIOPin IO_CLK;
	static CGPIOPin IO_DAT;
	static CGPIOPin IO_SRQ;
	static CGPIOPin IO_RST;
	static CGPIOPin IO_buttons[5];

	static CGPIOPin IO_IN_BUTTON4;
	static CGPIOPin IO_IN_BUTTON5;
	static CGPIOPin IO_IN_RESET;
	static CGPIOPin IO_IN_SRQ;
	static CGPIOPin IO_IN_BUTTON2;
	static CGPIOPin IO_IN_BUTTON3;
	static CGPIOPin IO_IN_ATN;
	static CGPIOPin IO_IN_DATA;
	static CGPIOPin IO_IN_CLOCK;
	static CGPIOPin IO_IN_BUTTON1;

	//static CGPIOPin IO_OUT_RESET;
	static CGPIOPin IO_OUT_SPI0_RS;

	static CGPIOPin IO_OUT_ATN;
	static CGPIOPin IO_OUT_SOUND;
	static CGPIOPin IO_OUT_LED;
	static CGPIOPin IO_OUT_CLOCK;
	static CGPIOPin IO_OUT_DATA;
	static CGPIOPin IO_OUT_SRQ;
	static unsigned _mask;
#endif	
public:
	static inline void Initialise(void)
	{
		volatile int index; // Force a real delay in the loop below.
		// Clear all outputs to 0
#if !defined(__PICO2__)	&& !defined(ESP32)
		write32(ARM_GPIO_GPCLR0, 0xFFFFFFFF);	
#endif		
		//CGPIOPin::WriteAll(0xffffffff, 0xffffffff);
		if (!splitIECLines)
		{
			// This means that when any pin is turn to output it will output a 0 and pull lines low (ie an activation state on the IEC bus)
			// Note: on the IEC bus you never output a 1 you simply tri state and it will be pulled up to a 1 (ie inactive state on the IEC bus) if no one else is pulling it low.

#if !defined(__PICO2__)	&& !defined(ESP32)
			myOutsGPFSEL0 = read32(ARM_GPIO_GPFSEL0);
			myOutsGPFSEL1 = read32(ARM_GPIO_GPFSEL1);
#endif

#if defined (__CIRCLE__)
			_mask = ((1 << PIGPIO_OUT_LED) | (1 << PIGPIO_OUT_SOUND));			
#if defined (CIRCLE_GPIO)
			IEC_Bus::IO_led.AssignPin(PIGPIO_OUT_LED); IEC_Bus::IO_led.SetMode(GPIOModeOutput, false);
			IEC_Bus::IO_sound.AssignPin(PIGPIO_OUT_SOUND); IEC_Bus::IO_sound.SetMode(GPIOModeOutput, false);
			IEC_Bus::IO_CLK.AssignPin(PIGPIO_CLOCK); IEC_Bus::IO_CLK.SetMode(GPIOModeInput, false);
			IEC_Bus::IO_ATN.AssignPin(PIGPIO_ATN); IEC_Bus::IO_ATN.SetMode(GPIOModeInput, false);
			IEC_Bus::IO_DAT.AssignPin(PIGPIO_DATA); IEC_Bus::IO_DAT.SetMode(GPIOModeInput, false);
			IEC_Bus::IO_SRQ.AssignPin(PIGPIO_SRQ); IEC_Bus::IO_SRQ.SetMode(GPIOModeInput, false);
			IEC_Bus::IO_RST.AssignPin(PIGPIO_RESET); IEC_Bus::IO_RST.SetMode(GPIOModeInput, false);
#endif			
			for (int i = 0; i < 5; i++) {
				IO_buttons[i].AssignPin(ButtonPins[i]);
				IO_buttons[i].SetMode(GPIOModeInputPullUp, true);
				Kernel.log("%s: assigning button %d to pin %d", __FUNCTION__, i, ButtonPins[i]);
			}
#endif
#if defined(__PICO2__)
			gpio_init_mask(PIGPIO_MASK_IN_ATN | PIGPIO_MASK_IN_DATA | PIGPIO_MASK_IN_CLOCK | PIGPIO_MASK_IN_SRQ | PIGPIO_MASK_IN_RESET);
			gpio_init(PIGPIO_RESET);
    		gpio_set_dir(PIGPIO_RESET, GPIO_IN);
			gpio_init(PIGPIO_SRQ);
    		gpio_set_dir(PIGPIO_SRQ, GPIO_IN);
			gpio_init(PIGPIO_DATA);
    		gpio_set_dir(PIGPIO_DATA, GPIO_IN);
			gpio_init(PIGPIO_ATN);
    		gpio_set_dir(PIGPIO_ATN, GPIO_IN);
			gpio_init(PIGPIO_CLOCK);
    		gpio_set_dir(PIGPIO_CLOCK, GPIO_IN);
			for (int i = 0; i < 5; i++) 
			{
				gpio_init(ButtonPins[i]);
				gpio_set_dir(ButtonPins[i], GPIO_IN);
				gpio_pull_up(ButtonPins[i]);
				printf("%s: assigning button %d to pin %d\n", __FUNCTION__, i, ButtonPins[i]);
			}
			gpio_init(PIGPIO_OUT_LED);
			gpio_set_dir(PIGPIO_OUT_LED, GPIO_OUT);
			gpio_init(PIGPIO_OUT_SOUND);
			gpio_set_dir(PIGPIO_OUT_SOUND, GPIO_OUT);
#endif/* __PICO2__ */
#if defined(ESP32)
			gpio_reset_pin((gpio_num_t)PIGPIO_ATN);
			gpio_set_direction((gpio_num_t)PIGPIO_ATN, GPIO_MODE_INPUT);
			gpio_set_pull_mode((gpio_num_t)PIGPIO_ATN, GPIO_FLOATING);
			gpio_reset_pin((gpio_num_t)PIGPIO_RESET);
			gpio_set_direction((gpio_num_t)PIGPIO_RESET, GPIO_MODE_INPUT);
			//gpio_set_pull_mode((gpio_num_t)PIGPIO_RESET, GPIO_PULLDOWN_ONLY);

			gpio_reset_pin((gpio_num_t)PIGPIO_CLOCK);
			gpio_set_direction((gpio_num_t)PIGPIO_CLOCK, GPIO_MODE_INPUT_OUTPUT);
			//gpio_set_pull_mode((gpio_num_t)PIGPIO_CLOCK, GPIO_FLOATING);
			gpio_reset_pin((gpio_num_t)PIGPIO_DATA);
			gpio_set_direction((gpio_num_t)PIGPIO_DATA, GPIO_MODE_INPUT_OUTPUT);
			//gpio_set_pull_mode((gpio_num_t)PIGPIO_DATA, GPIO_FLOATING);

			gpio_reset_pin((gpio_num_t)PIGPIO_OUT_LED);
			gpio_set_direction((gpio_num_t)PIGPIO_OUT_LED, GPIO_MODE_OUTPUT);
			gpio_reset_pin((gpio_num_t)PIGPIO_OUT_SOUND);
			gpio_set_direction((gpio_num_t)PIGPIO_OUT_SOUND, GPIO_MODE_OUTPUT);
#endif/* ESP32 */
#if !defined(__PICO2__) && !defined(ESP32)
			myOutsGPFSEL0 |= (1 << (PIGPIO_OUT_LED * 3));
			myOutsGPFSEL1 |= (1 << ((PIGPIO_OUT_SOUND - 10) * 3));
			//RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_SOUND, FS_OUTPUT);
			//RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_LED, FS_OUTPUT);
#endif			
		}
		else
		{
#if defined(__CIRCLE__)			
			IEC_Bus::IO_IN_BUTTON4.AssignPin(PIGPIO_IN_BUTTON4); IEC_Bus::IO_IN_BUTTON4.SetMode(GPIOModeInputPullUp, true);
			IEC_Bus::IO_IN_BUTTON5.AssignPin(PIGPIO_IN_BUTTON5); IEC_Bus::IO_IN_BUTTON5.SetMode(GPIOModeInputPullUp, true);
			IEC_Bus::IO_IN_RESET.AssignPin(PIGPIO_IN_RESET); IEC_Bus::IO_IN_RESET.SetMode(GPIOModeInput);
			IEC_Bus::IO_IN_SRQ.AssignPin(PIGPIO_IN_SRQ); IEC_Bus::IO_IN_SRQ.SetMode(GPIOModeInput);
			IEC_Bus::IO_IN_BUTTON2.AssignPin(PIGPIO_IN_BUTTON2); IEC_Bus::IO_IN_BUTTON2.SetMode(GPIOModeInputPullUp, true);
			IEC_Bus::IO_IN_BUTTON3.AssignPin(PIGPIO_IN_BUTTON3); IEC_Bus::IO_IN_BUTTON3.SetMode(GPIOModeInputPullUp, true);
			IEC_Bus::IO_IN_ATN.AssignPin(PIGPIO_IN_ATN); IEC_Bus::IO_IN_ATN.SetMode(GPIOModeInput);
			IEC_Bus::IO_IN_DATA.AssignPin(PIGPIO_IN_DATA); IEC_Bus::IO_IN_DATA.SetMode(GPIOModeInput);
			IEC_Bus::IO_IN_CLOCK.AssignPin(PIGPIO_IN_CLOCK); IEC_Bus::IO_IN_CLOCK.SetMode(GPIOModeInput);
			IEC_Bus::IO_IN_BUTTON1.AssignPin(PIGPIO_IN_BUTTON1); IEC_Bus::IO_IN_BUTTON1.SetMode(GPIOModeInputPullUp, true);

			//IO_OUT_RESET.AssignPin(PIGPIO_OUT_RESET); IEC_Bus::IO_OUT_RESET.SetMode(GPIOModeOutput);
			IEC_Bus::IO_OUT_SPI0_RS.AssignPin(PIGPIO_OUT_SPI0_RS); IEC_Bus::IO_OUT_SPI0_RS.SetMode(GPIOModeOutput);

			IEC_Bus::IO_OUT_ATN.AssignPin(PIGPIO_OUT_ATN); IEC_Bus::IO_OUT_ATN.SetMode(GPIOModeOutput);
			IEC_Bus::IO_OUT_SOUND.AssignPin(PIGPIO_OUT_SOUND); IEC_Bus::IO_OUT_SOUND.SetMode(GPIOModeOutput);
			IEC_Bus::IO_OUT_LED.AssignPin(PIGPIO_OUT_LED); IEC_Bus::IO_OUT_LED.SetMode(GPIOModeOutput);
			IEC_Bus::IO_OUT_CLOCK.AssignPin(PIGPIO_OUT_CLOCK); IEC_Bus::IO_OUT_CLOCK.SetMode(GPIOModeOutput);
			IEC_Bus::IO_OUT_DATA.AssignPin(PIGPIO_OUT_DATA); IEC_Bus::IO_OUT_DATA.SetMode(GPIOModeOutput);
			IEC_Bus::IO_OUT_SRQ.AssignPin(PIGPIO_OUT_SRQ); IEC_Bus::IO_OUT_SRQ.SetMode(GPIOModeOutput);						
#elif defined(__PICO2__)
	//#warning "PICO2 TODO SPLIT IEC lines init XXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
			not_implemented(__FUNCTION__);
#elif defined(ESP32)
			not_implemented(__FUNCTION__);
#else
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_BUTTON4, FS_INPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_BUTTON5, FS_INPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_RESET, FS_INPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_SRQ, FS_INPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_BUTTON2, FS_INPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_BUTTON3, FS_INPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_ATN, FS_INPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_DATA, FS_INPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_CLOCK, FS_INPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_IN_BUTTON1, FS_INPUT);


			//RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_RESET, FS_OUTPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_SPI0_RS, FS_OUTPUT);
			
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_ATN, FS_OUTPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_SOUND, FS_OUTPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_LED, FS_OUTPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_CLOCK, FS_OUTPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_DATA, FS_OUTPUT);
			RPI_SetGpioPinFunction((rpi_gpio_pin_t)PIGPIO_OUT_SRQ, FS_OUTPUT);
#endif			
		}
	
#if not defined(EXPERIMENTALZERO)
#if !defined (__CIRCLE__)
		// Set up audio.
		write32(CM_PWMDIV, CM_PASSWORD + 0x2000);
		write32(CM_PWMCTL, CM_PASSWORD + CM_ENAB + CM_SRC_OSCILLATOR);	// Use Default 100MHz Clock
		// Set PWM
		write32(PWM_RNG1, 0x1B4);	// 8bit 44100Hz Mono
		write32(PWM_RNG2, 0x1B4);
		write32(PWM_CTL, PWM_USEF2 + PWM_PWEN2 + PWM_USEF1 + PWM_PWEN1 + PWM_CLRF1);
#endif		
#endif
		for (index = 0; index < buttonCount; ++index)
		{
			InputButton[index] = false;
			InputButtonPrev[index] = false;
			validInputCount[index] = 0;
			inputRepeatThreshold[index] = INPUT_BUTTON_REPEAT_THRESHOLD;
			inputRepeat[index] = 0;
			inputRepeatPrev[index] = 0;
		}
#if !defined(__CIRCLE__) && !defined(__PICO2__) && !defined(ESP32)
		// Enable the internal pullups for the input button pins using the method described in BCM2835-ARM-Peripherals manual.
		RPI_GpioBase->GPPUD = 2;
		for (index = 0; index < 150; ++index)
		{
		}
		RPI_GpioBase->GPPUDCLK0 = PIGPIO_MASK_IN_BUTTON1 | PIGPIO_MASK_IN_BUTTON2 | PIGPIO_MASK_IN_BUTTON3 | PIGPIO_MASK_IN_BUTTON4 | PIGPIO_MASK_IN_BUTTON5;
		for (index = 0; index < 150; ++index)
		{
		}
		RPI_GpioBase->GPPUD = 0;
		RPI_GpioBase->GPPUDCLK0 = 0;
#endif
#if !defined(__PICO2__) && !defined(ESP32)
		//ROTARY: Added for rotary encoder support - 09/05/2019 by Geo...
		if (IEC_Bus::rotaryEncoderEnable == true)
		{
			//ROTARY: Added for rotary encoder inversion (Issue#185) - 08/13/2020 by Geo...
			if (IEC_Bus::rotaryEncoderInvert == true)
			{
				IEC_Bus::rotaryEncoder.Initialize(RPI_GPIO23, RPI_GPIO22, RPI_GPIO27);
			}
			else
			{
				IEC_Bus::rotaryEncoder.Initialize(RPI_GPIO22, RPI_GPIO23, RPI_GPIO27);
			}
		}
#endif		
	}

	static inline void LetSRQBePulledHigh()
	{
		SRQSetToOut = IEC_Bus::invertIECInputs;
#if defined(PI1581SUPPORT)
		RefreshOuts1581();
#endif	
	}

#if defined(EXPERIMENTALZERO)
	static inline bool AnyButtonPressed()
	{
		return ((gplev0 & PIGPIO_MASK_ANY_BUTTON) != PIGPIO_MASK_ANY_BUTTON);
	}
#endif

	static void UpdateButton(int index, unsigned gplev0)
	{
		bool inputcurrent = (gplev0 & ButtonPinFlags[index]) == 0;

		InputButtonPrev[index] = InputButton[index];
		inputRepeatPrev[index] = inputRepeat[index];

		if (inputcurrent)
		{
			//Kernel.log("%s: button %d fired", __FUNCTION__, index);
			validInputCount[index]++;
			if (validInputCount[index] == INPUT_BUTTON_DEBOUNCE_THRESHOLD)
			{
				InputButton[index] = true;
				inputRepeatThreshold[index] = INPUT_BUTTON_DEBOUNCE_THRESHOLD + INPUT_BUTTON_REPEAT_THRESHOLD;
				inputRepeat[index]++;
			}

			if (validInputCount[index] == inputRepeatThreshold[index])
			{
				inputRepeat[index]++;
				inputRepeatThreshold[index] += INPUT_BUTTON_REPEAT_THRESHOLD / inputRepeat[index];
			}
		}
		else
		{
			InputButton[index] = false;
			validInputCount[index] = 0;
			inputRepeatThreshold[index] = INPUT_BUTTON_REPEAT_THRESHOLD;
			inputRepeat[index] = 0;
			inputRepeatPrev[index] = 0;
		}
	}

	
	//ROTARY: Added for rotary encoder support - 09/05/2019 by Geo...
	//
	// Note: This method serves as a shim to allow the rotary encoder
	//       logic to set a specific input button state (fooling the
	//       original logic into thinking a button was pressed or
	//       released).
	//
	static inline void SetButtonState(int index, bool state)
	{

		InputButtonPrev[index] = InputButton[index];
		inputRepeatPrev[index] = inputRepeat[index];

		if (state == true)
		{

			InputButton[index] = true;
			validInputCount[index] = INPUT_BUTTON_DEBOUNCE_THRESHOLD;
			inputRepeatThreshold[index] = INPUT_BUTTON_DEBOUNCE_THRESHOLD + INPUT_BUTTON_REPEAT_THRESHOLD;
			inputRepeat[index]++;

			validInputCount[index] = inputRepeatThreshold[index];
			inputRepeat[index]++;
			inputRepeatThreshold[index] += INPUT_BUTTON_REPEAT_THRESHOLD / inputRepeat[index];

		}	
		else
		{

			InputButton[index] = false;
			validInputCount[index] = 0;
			inputRepeatThreshold[index] = INPUT_BUTTON_REPEAT_THRESHOLD;
			inputRepeat[index] = 0;
			inputRepeatPrev[index] = 0;
			
		}

	}


	static void ReadBrowseMode(void);
	static void ReadGPIOUserInput(void);
	static void ReadEmulationMode1541(void);
	static void ReadEmulationMode1581(void);

	static void WaitUntilReset(void)
	{
		unsigned gplev0;
		do
		{
#if defined(__CIRCLE__)			
#if !defined (CIRCLE_GPIO)
			gplev0 = read32(ARM_GPIO_GPLEV0);
#else
			gplev0 = CGPIOPin::ReadAll();
#endif			
#elif defined(__PICO2__)
			gplev0 = gpio_get_all();
#elif defined(ESP32)
			gplev0 = (gpio_get_level((gpio_num_t)PIGPIO_RESET) << PIGPIO_RESET);
#else
			gplev0 = read32(ARM_GPIO_GPLEV0);
#endif	
			Resetting = !ignoreReset && 
						((gplev0 & PIGPIO_MASK_IN_RESET) == 
				 		 (invertIECInputs ? PIGPIO_MASK_IN_RESET : 0));
			if (Resetting)
			{
				IEC_Bus::WaitMicroSeconds(100);
			}
		}
		while (Resetting);
	}

	// Out going
	static void PortB_OnPortOut(void* pUserData, unsigned char status);

	static void RefreshOuts1541(void);
#if defined(PI1581SUPPORT)
	static inline void RefreshOuts1581(void)
	{
		unsigned set = 0;
		unsigned clear = 0;
		unsigned tmp;

		if (!splitIECLines)
		{
			unsigned outputs = 0;
#if !defined (CIRCLE_GPIO)
			if (AtnaDataSetToOut || DataSetToOut) outputs |= (FS_OUTPUT << ((PIGPIO_DATA - 10) * 3));
			if (ClockSetToOut) outputs |= (FS_OUTPUT << ((PIGPIO_CLOCK - 10) * 3));
			//if (SRQSetToOut) outputs |= (FS_OUTPUT << ((PIGPIO_SRQ - 10) * 3));			// For Option A hardware we should not support pulling more than 2 lines low at any one time!

			unsigned nValue = (myOutsGPFSEL1 & PI_OUTPUT_MASK_GPFSEL1) | outputs;
			write32(ARM_GPIO_GPFSEL1, nValue);
			write32(ARM_GPIO_GPFSEL0, myOutsGPFSEL0);
#else
			u32 im = 0, om = 0;
			if (AtnaDataSetToOut || DataSetToOut)
				om |= (1 << PIGPIO_DATA);
			else
				im |= (1 << PIGPIO_DATA);
			if (ClockSetToOut)
				om |= (1 << PIGPIO_CLOCK);
			else
				im |= (1 << PIGPIO_CLOCK);
#if RASPPI == 5		
			if (im)
				write32 (RIO0_OE (0, RIO_CLR_OFFSET), im);
			if (om)
				write32 (RIO0_OE (0, RIO_SET_OFFSET), om);
#else			
			CGPIOPin::SetModeAll(im, om);
#endif	/* RASPPI */
#endif	/* CIRCLE_GPIO */
		}
		else
		{
#if defined (CIRCLE_GPIO)		
			_mask |= ((1 << PIGPIO_OUT_DATA) | (1 << PIGPIO_OUT_CLOCK) | (1 << PIGPIO_OUT_SRQ));
#endif	
			if (AtnaDataSetToOut || DataSetToOut) set |= 1 << PIGPIO_OUT_DATA;
			else clear |= 1 << PIGPIO_OUT_DATA;

			if (ClockSetToOut) set |= 1 << PIGPIO_OUT_CLOCK;
			else clear |= 1 << PIGPIO_OUT_CLOCK;

			if (SRQSetToOut) set |= 1 << PIGPIO_OUT_SRQ;	// fast clock is pulled high but we have an inverter in our hardware so to compensate we invert in software now
			else clear |= 1 << PIGPIO_OUT_SRQ;

			if (!invertIECOutputs) {
				tmp = set;
				set = clear;
				clear = tmp;
			}
		}

		if (OutputLED) set |= 1 << PIGPIO_OUT_LED;
		else clear |= 1 << PIGPIO_OUT_LED;

		if (OutputSound) set |= 1 << PIGPIO_OUT_SOUND;
		else clear |= 1 << PIGPIO_OUT_SOUND;

#if !defined (CIRCLE_GPIO)
		write32(ARM_GPIO_GPSET0, set);
		write32(ARM_GPIO_GPCLR0, clear);
#else
#if RASPPI == 5
		write32 (RIO0_OUT (0, RIO_CLR_OFFSET), clear);
		write32 (RIO0_OUT (0, RIO_SET_OFFSET), set);
#else
		CGPIOPin::WriteAll(set, _mask);
#endif
#endif
	}			
#endif /* PI1581SUPPORT */	
	static void WaitMicroSeconds(u32 amount)
	{
		u32 count;
#if defined (__CIRCLE__) || defined(__PICO2__) || defined(ESP32)	
		usDelay(amount); 
		return;
#else
		for (count = 0; count < amount; ++count)
		{
			unsigned before;
			unsigned after;
			// We try to update every micro second and use as a rough timer to count micro seconds
			before = read32(ARM_SYSTIMER_CLO);
			do
			{
				after = read32(ARM_SYSTIMER_CLO);
			} while (after == before);
		}
#endif		
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// 1581 Fast Serial
	static inline void SetFastSerialData(bool value)
	{
		DataSetToOut = value;
	}
	static inline void SetFastSerialSRQ(bool value)
	{
		SRQSetToOut = value;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Manual methods used by IEC_Commands
	static inline void AssertData()
	{
		if (!DataSetToOut)
		{
			DataSetToOut = true;
			RefreshOuts1541();
		}
	}
	static inline void ReleaseData()
	{
		if (DataSetToOut)
		{
			DataSetToOut = false;
			RefreshOuts1541();
		}
	}

	static inline void AssertClock()
	{
		if (!ClockSetToOut)
		{
			ClockSetToOut = true;
			RefreshOuts1541();
		}
	}
	static inline void ReleaseClock()
	{
		if (ClockSetToOut)
		{
			ClockSetToOut = false;
			RefreshOuts1541();
		}
	}

	static inline bool GetPI_SRQ() { return PI_SRQ; }
	static inline bool GetPI_Atn() { return PI_Atn; }
	static inline bool IsAtnAsserted() { return PI_Atn; }
	static inline bool IsAtnReleased() { return !PI_Atn; }
	static inline bool GetPI_Data() { return PI_Data; }
	static inline bool IsDataAsserted() { return PI_Data; }
	static inline bool IsDataReleased() { return !PI_Data; }
	static inline bool GetPI_Clock() { return PI_Clock; }
	static inline bool IsClockAsserted() { return PI_Clock; }
	static inline bool IsClockReleased() { return !PI_Clock; }
	static inline bool GetPI_Reset() { return PI_Reset; }
	static inline bool IsDataSetToOut() { return DataSetToOut; }
	//static inline bool IsAtnaDataSetToOut() { return AtnaDataSetToOut; }
	static inline bool IsClockSetToOut() { return ClockSetToOut; }
	static inline bool IsReset() { return Resetting; }

	static inline void WaitWhileAtnAsserted()
	{
		while (IsAtnAsserted())
		{
			ReadBrowseMode();
		}
	}
	///////////////////////////////////////////////////////////////////////////////////////////////

	static inline void SetSplitIECLines(bool value)
	{
		splitIECLines = value;
		if (splitIECLines)
		{
#if !defined(__PICO2__)	&& !defined(ESP32)
			PIGPIO_MASK_IN_ATN = 1 << PIGPIO_IN_ATN;
			PIGPIO_MASK_IN_DATA = 1 << PIGPIO_IN_DATA;
			PIGPIO_MASK_IN_CLOCK = 1 << PIGPIO_IN_CLOCK;
			PIGPIO_MASK_IN_SRQ = 1 << PIGPIO_IN_SRQ;
			PIGPIO_MASK_IN_RESET = 1 << PIGPIO_IN_RESET;
#endif			
		}
	}

	static inline void SetInvertIECInputs(bool value) 
	{
		invertIECInputs = value;
		if (value)
		{
			PI_Atn = !PI_Atn;
			PI_Data = !PI_Data;
			PI_Clock = !PI_Clock;
			PI_SRQ = !PI_SRQ;
			PI_Reset = !PI_Reset;
		}
	}

	static inline void SetInvertIECOutputs(bool value)
	{
		invertIECOutputs = value;
	}

	static inline void SetIgnoreReset(bool value)
	{
		ignoreReset = value;
	}

	//ROTARY: Added for rotary encoder support - 09/05/2019 by Geo...
	static inline void SetRotaryEncoderEnable(bool value)
	{
		rotaryEncoderEnable = value;
	}

	//ROTARY: Added for rotary encoder inversion (Issue#185) - 08/13/2020 by Geo...
	static inline void SetRotaryEncoderInvert(bool value)
	{
		rotaryEncoderInvert = value;
	}

	// CA1 input ATN
	// If CA1 is ever set to output
	//	- CA1 will start to drive pb7
	// CA2, CB1 and CB2 are not connected
	//	- check if pulled high or low
	static m6522* VIA;
	static m8520* CIA;
	static IOPort* port;

	static void Reset(void);

	static bool GetInputButtonPressed(int buttonIndex) { return InputButton[buttonIndex] && !InputButtonPrev[buttonIndex]; }
	static bool GetInputButtonReleased(int buttonIndex) { return InputButton[buttonIndex] == false; }
	static bool GetInputButton(int buttonIndex) { return InputButton[buttonIndex]; }
	static bool GetInputButtonRepeating(int buttonIndex) { return inputRepeat[buttonIndex] != inputRepeatPrev[buttonIndex]; }
	static bool GetInputButtonHeld(int buttonIndex) { return inputRepeatThreshold[buttonIndex] >= INPUT_BUTTON_DEBOUNCE_THRESHOLD + (INPUT_BUTTON_REPEAT_THRESHOLD * 2); }

	static bool OutputLED;
	static bool OutputSound;

private:
	static u32 oldClears;
	static u32 oldSets;

	static bool splitIECLines;
	static bool invertIECInputs;
	static bool invertIECOutputs;
	static bool ignoreReset;

	static u32 PIGPIO_MASK_IN_ATN;
	static u32 PIGPIO_MASK_IN_DATA;
	static u32 PIGPIO_MASK_IN_CLOCK;
	static u32 PIGPIO_MASK_IN_SRQ;
	static u32 PIGPIO_MASK_IN_RESET;
	static u32 PIGPIO_MASK_OUT_LED;
	static u32 PIGPIO_MASK_OUT_SOUND;

	static u32 emulationModeCheckButtonIndex;

	static unsigned gplev0;

	static bool PI_Atn;
	static bool PI_Data;
	static bool PI_Clock;
	static bool PI_SRQ;
	static bool PI_Reset;

	static bool VIA_Atna;
	static bool VIA_Data;
	static bool VIA_Clock;

	static bool DataSetToOut;
	static bool AtnaDataSetToOut;
	static bool ClockSetToOut;
	static bool SRQSetToOut;
	static bool Resetting;

	static int buttonCount;

	static u32 myOutsGPFSEL0;
	static u32 myOutsGPFSEL1;
	static bool InputButton[5];
	static bool InputButtonPrev[5];
	static u32 validInputCount[5];
	static u32 inputRepeatThreshold[5];
	static u32 inputRepeat[5];
	static u32 inputRepeatPrev[5];

	//ROTARY: Added for rotary encoder support - 09/05/2019 by Geo...
	static RotaryEncoder rotaryEncoder;
	static bool rotaryEncoderEnable;
	//ROTARY: Added for rotary encoder inversion (Issue#185) - 08/13/2020 by Geo...
	static bool rotaryEncoderInvert;

};
#endif
