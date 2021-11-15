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

extern HID_DEVICE hid_device;

static BYTE addr = 1; 				//hard-wired USB address
const char* const devclasses[] = { " Uninitialized", " HID Keyboard", " HID Mouse", " Mass storage" };

BYTE GetDriverandReport();

void setLED(int LED);

void clearLED(int LED);

void printSignedHex0(signed char value);

void printSignedHex1(signed char value);

void setKeycode(WORD keycode);

void keyInput(int x, int y, int index, int color);
void setEdge(int x, int y);
