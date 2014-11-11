// o11 servo firmware
// Sean Schumer
// littleBits Electronics Inc.
// 29 MAY 2014
/*
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
//=============================
// Atmel AVR ATtiny25/V, ATtiny45/V, ATtiny85/V
// 8MHz XTAL Oscillator
// GCC compiler.
//==============================

/*
Description:
========================================================================================================

This code is meant to be flashed onto the o11 servo's on board MCU. It will send a control signal to the
attached servo meant for 180 degree control. A low input turns the servo counter-clockwise fully and a
high signal turns the servo clockwise fully.
*/

#define F_CPU	8000000UL					// Main system crystal frequency.

//#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#define UPDATE_PERIOD_US 10000 //How often a pulse is sent to the servo to dictate the servo's position
#define INTERRUPT_CTC_COUNT 80 //The count of the internal timer that triggers an interrupt to regulat the pulse length
#define PULSE_COUNT_MINIMUM 75 //Pulse time in microseconds * 10
#define PULSE_COUNT_MAXIMUM 205 //Pulse time in microseconds * 10

unsigned int UpdatePeriodCycles;
unsigned int PulseCountHigh; //How long to stay high for
unsigned int PulseCurrentCount; //When this exceeds pulse count high, change the signal to low 
unsigned char ADCin;
unsigned char SwingDirection = 1;
unsigned int count = 160;
unsigned int ADCval = 0;
int lastSwingRate = 0;
int lastVal = 0;

unsigned long servoBuffer = 0;

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// Program main loop:
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
// void Delay10us(unsigned int DelayMultiplier)
// 
// Delay for 10*DelayMultiplier microseconds
//
// Since our delay is variable, we must loop through delay calls as the
// _delay_us function requires a constant
//
/////////////////////////////////////////////////////////////////////////
void Delay10us(unsigned int DelayMultiplier)
{
	while (DelayMultiplier--) _delay_us(10);
}

ISR(TIMER0_COMPA_vect)
// The pin change interrupt vector.  Used to wake from sleep.
{
	//PulseCurrentCount = (PulseCurrentCount+1)%UpdatePeriodCycles;
	//if (PulseCurrentCount > PulseCountHigh)
	PORTB ^= 0b00000010; //Go high
	Delay10us(count);
	PORTB &= 0b11111101; //Go low
}

int main (void)
// Application main loop.
{
	UpdatePeriodCycles = 1000;
	PulseCountHigh = 74;
	PulseCurrentCount = 0;
	cli();
	
	// Set up timer 
	DDRB = 0b00000010;
	PORTB |= 0b00000001;
	ADMUX = 0b00100001; //Adjust left, Select PBS as ADC input pin
	ADCSRA = 0b10000111; //Enable ADC, div 16
	TCCR0A = 0b00000010; // CTC Mode
	TCCR0B = 0b00000101; // CLKio/1024 = 8kHz
	OCR0A = INTERRUPT_CTC_COUNT; //count to interrupt
	TIMSK = 0b00010000; //Interrupt at CTC match
	
	// Set up timer for 'swing' speed
	TCCR1 = 0b10001101;
	OCR1A = 9; // Set this equal to the reset timing always.
	OCR1C = 9; //Set this to the value you want the clock to reset at.
	
	sei();
	ADCSRA |= 0b01000000;
	while (!(ADCSRA & (1 << ADIF)));
	ADCSRA |= (1 << ADIF);
	ADCval = ADCH;
	servoBuffer = (unsigned long)ADCval << 8; 
	lastVal = ADCval;
	count = (ADCval>>1) + 75;
	
	while (1)
	{
		ADCSRA |= 0b01000000;
		while (!(ADCSRA & (1 << ADIF)));	
		ADCSRA |= (1 << ADIF);
		ADCval = ADCH;
		servoBuffer = ( servoBuffer - (servoBuffer >> 8) ) + ADCval;
		ADCval = servoBuffer >> 8;
		if (PINB & 1) // Check if the servo switch is set to turn mode
		{
			if ( (lastVal - (int)ADCval > 3) || (lastVal - (int)ADCval < -3) )
			{
				lastVal = ADCval;
				count = (ADCval>>1) + 75;
			}
			
			//count = ADCH;
		}
		else
		{
			if (ADCval > 10)
			{
				ADCval = 70 - (ADCval>>2);
				if ( ((lastSwingRate - ADCval) > 5) || ((ADCval - lastSwingRate) > 5) ) //Prevent motion jitter
				{
					while (TCNT1 > 5);
					OCR1A = ADCval;
					OCR1C = ADCval;
					lastSwingRate = ADCval;
				}
				
				if ((TIFR & (1 << OCF1A))) {     //Wait for the CTC to increment the position
					
					TIFR |= (1 << OCF1A); //Clear the interrupt
			
					if (SwingDirection == 1)
						count++;
					else
						count--;
				
					if (count >= PULSE_COUNT_MAXIMUM) {
						count--;
						SwingDirection = 0;
					}
					else if (count <= PULSE_COUNT_MINIMUM) {
						count++;
						SwingDirection = 1;
					}
				}
			}
		}
	}

}

