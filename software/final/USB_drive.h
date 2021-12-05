/*
 * USB_drive.h
 *
 *  Created on: Nov 12, 2021
 *      Author: kelvin3
 */

#include <stdio.h>
#include "system.h"
#include "altera_avalon_spi.h"
#include "altera_avalon_spi_regs.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "usb_kb/GenericMacros.h"
#include "usb_kb/GenericTypeDefs.h"
#include "usb_kb/HID.h"
#include "usb_kb/MAX3421E.h"
#include "usb_kb/transfer.h"
#include "usb_kb/usb_ch9.h"
#include "usb_kb/USB.h"
#include "tetris.h"





BYTE GetDriverandReport();
void setLED(int LED);
void clearLED(int LED);
void printSignedHex0(signed char value);
void printSignedHex1(signed char value);
void setKeycode(WORD keycode);

void keyInput();
void paintScreen(TetrisGameState* state);
