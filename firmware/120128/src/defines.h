/* Global Definitions and Typedefs go here, but not variable declarations:
 *
 * Copyright 2014 littleBits Electronics
 *
 * This file is part of o11-servo.
 *
 * o11-servo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * o11-servo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License at <http://www.gnu.org/licenses/> for more details.
 */

#define F_CPU	16000000UL					// Main system crystal frequency.

// Make a C version of "bool"
//----------------------------
typedef unsigned char bool;
#define		false			(0)
#define		true			(!(false))


//Port pin setting macros:
//----------------------------------------------
#define		SetOP(port,bit)(port)|=(1<<(bit))
#define		ClearOP(port,bit)(port)&=~(1<<(bit))

#define		MACRO_DoTenNops	asm volatile("nop"::); asm volatile("nop"::); asm volatile("nop"::); asm volatile("nop"::); asm volatile("nop"::); asm volatile("nop"::); asm volatile("nop"::); asm volatile("nop"::); asm volatile("nop"::); asm volatile("nop"::);


