// littleBits Servo Module
// Sat Jan 28 13:01:00 EST 2012

//=============================
// Atmel AVR Attiny45 (or Attiny85) MCU
// (n)MHz XTAL Oscillator
// GCC compiler.
//==============================
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
/*
Description:
========================================================================================================
Controls a servo motor by generating a 50Hz pulse waveform.  The duration of the pulse sets the position
of the servo.  1.5mSec (high, I presume) is the center.  0.9mS is CCW extreme, 2.1mSec is CW extreme
(I think -- the DS is not clear about either of these guesses).

There are two modes of operation, selected by a a switch.
In the first mode, the ADC reads an input voltage from 0-5v, and sets the servo to a corresponding point in its arc.
In the second, the servo continuously oscillates between full CCW and CW.  The voltage in on the ADC determines the speed of oscillation.

OC1A is used to generate the waveforms.
Ideally, we would use a 3.2768 or 6.5535 MHz xtal, since this gives us exact division into 50Hz.
However, ceramic resonators don't come at those frequencies so we will use an 8MHz system clock.


WITH A 16 BIT TIMER/COUNTER MODULE:
--------------------------------------
This means we will use a 16 bit timer (timer 1) at a prescaler of 4, and set the TOP of the counter at 40,000.  This gives us exactly 50Hz.
50Hz is 20mSec.  Our operating range (0.9mSec to 2.1mSec) is 1.2mSec.  At these settings, this gives us an effective resolution of 2400 ticks -- better than 11 bit resolution.
Since we will be hard pressed to get that kind of resolution from the ADC, this is more than adequate.

0.9mSec = 1800 cycles
2.1mSec = 4200 cycles

So, every time our timer starts over, we will set the output pin high for at least 1800 cycles, but no more than 4200.

WITH AN 8 BIT TIMER/COUNTER MODULE:
--------------------------------------
...or rather with two like in the Attiny45/85
Set one to interrupt every 50Hz.
During the idle period before the servo pulse, calculate the pulse duration in timer ticks (16800 max at /1, or 4200 at /4)
Calculate the number of 256 step timer cycles to reach this.
Calculate the remainder.
If the remainder is less than 100, add 100 to the remainder and remember this

On the 50Hz interrupt, set the servo pin high.


NOTE:
------
Oversampling to 14 bit resolution, then dividing by 7 gives pretty close to 2400 steps (2.5% error)

(1024 * 9) gives really close to 9600 steps
Likewise, (4096 * 9) / 4 (both of these give 4% error)


*/

#include	"includes.h"


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// Program main loop:
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

ISR(PCINT0_vect)
// The pin change interrupt vector.  Used to wake from sleep.
{

}

int main (void)
// Application main loop.
{				

	// Generic Fuse Programming Caveats:
	// ---------------------------------
	// This device boots by default at 1MHz (Internal RC Osc dividec by 8).  Once into the program we can change this, or we can change the fuses for default behavior.
	// ONCE YOU KNOW YOU CAN GET CODE ONTO THE CHIP, ratchet the clock up to crystal speeds.  Otherwise you can get hosed.
	// NOTE -- on these little AVRs make sure you DON'T disable the RESET pin or you won't be able to ISP.
	//
	// So for this I use:
	//
	// EByte:  Don't change.
	// HByte:  Defaults to 0xDF.  Change to 0xDD to make the BOD less tolerant.
	// LByte:  Defaults to 0x62.  CKSEL=1111  SUT=11  CKDIV=1  CKOUT=1  so  1111 1111 or 0xFF 
	//
	// The device will then start at the frequency of the full-swing crystal in circuit (if there's no XTAL, this will brick your AVR)
	//
	// Command line:
	// avrdude -p attiny45 -c stk500v2 -u -t		// Or whatever processor we're using.
	// r lfuse (should be 62)
	// w lfuse 0 0xFF
	// r hfuse (DF is default)
	// w hfuse 0 0xDD

	// cli =	avrdude -p attiny45 -c stk500v2 -u -U lfuse:w:0xFF:m

	// Fri Feb 11 22:11:23 EST 2011
	// BOD should be set to dropout for the 5v check with a fast oscillator, so the BOD should be 100 and hfuse should be 0xdd like we said, and lfuse should be (SUT 01, 10, or 11 -- 11 is safe and slow, and CKSEL 1111) so 0xFF like we said.


	
	static unsigned char
		adcCount;		// Oversampling / Averaging counter
	unsigned char
		temp;
	static bool
		servoDirection,		// Keeps track of servo CW/CCW
		pulseJustDone,		// Goes true when the servo pulse has finished, false when the ADC stuff is done.
		servoPulseHigh;		// True when the pulse is being counted out
	static unsigned int
		adcAccumulator,		// Used for oversampling and averaging
		lastServoPosition;	// Use to increment servo change over time
	static volatile unsigned int
		servoTime,		// Counter for varialble part of servo pulse
		adcResult;		// Massaged output, used by servo code.  Volatile to keep compiler from messing with it

	cli();				// No interrupts.
	PRR=0xFF;			// Power off everything, let the initialization routines turn on modules you need.
	PLLCSR=0;			// Turn off all PLL mess so the timers get the system clock.

	CLKPR=0x80;			// This two byte combination undoes the CLKDIV setting and sets the clock prescaler to 1,
	CLKPR=0x00;			// giving us a clock frequency of 8MHz (We boot at 1MHz).

	DDRB=0xFF;			// All pins to outputs.
	PORTB=0;			// All pins low (keep drive / impedance low and similar from unit to unit).

// Set up input switch (PB0, pin 5)

	DDRB&=~(1<<PB0);		// Switch pin to input
	PORTB|=(1<<PB0);		// Turn on pullup
	// Leave last switch state arbitrary.

// Set up ADC to read position / speed value

	PRR&=~(1<<PRADC);		// Turn the ADC power on (clear the bit to power on).
	DDRB&=~(1<<2);			// PB2 to input
	PORTB&=~(1<<2);			// No pullup
	ADMUX=0x21; 			// 0010 0001.  VCC is the reference, left justify the result, read ADC1(PB2).
	DIDR0=(1<<ADC1D);		// Disable the digital input for ADC1 (PB2).
	ADCSRA=0x96;			// 1001 0110.  8MHz sysclk and prescaler = 1/64, ADC clock = 125kHz, normal conversion ~= 10kHz (13 samples per conversion).  Enable the ADC, no autotrigger or interrupts.
	// ADCSRB controls auto-triggering, which we aren't using right now.
	ADCSRA |= (1<<ADSC);  		// Start the first conversion to initialize the ADC.
	
// Set up system time (50Hz counter)

	PRR&=~(1<<PRTIM0);		// Turn the TMR0 power on.
	TIMSK=0x00;			// Disable all Timer associated interrupts (disables other timer too)
	TIFR=0xFF;			// Clear the interrupt flags by writing ones (clears other timer too)
	TCNT0=0;			// Initialize the counter to 0.
	GTCCR=0;			// Zero any other weird timer functions (affects other timer a bit)
	// Set to CTC mode -- raise flag every 50Hz
	// This is 160,000 cycles.  This is 625 timer overflows with no prescaler.  At /1024, this is 156 overflows, with an error of 0.16% -- probably good enough
	OCR0A=156;			// This means (in CTC mode) that we flag every (n) cycles
	TCCR0A=0x02;			// Ports normal, start setting CTC
	TCCR0B=0x05;			// Start clock at /1024 prescale	

// Servo pulse generation (TIMER1, OC1A)

	PRR&=~(1<<PRTIM1);		// Turn the TMR1 power on.
	TIMSK=0x00;			// Disable all Timer associated interrupts.
	TIFR=0xFF;			// Clear the interrupt flags by writing ones.
	TCNT1=0;			// Initialize the counter to 0.
	GTCCR=0;			// Zero any other weird timer functions.
	OCR1C=0xFF;			// This is the TOP value of the counter

	OCR1A=0xFF;			// Set this high, no compares yet
	TCCR1=0x80;			// Set CTC, stop the timer, and disconnect it from the output pin (pin will go wherever the output driver tells it

	DDRB|=(1<<1);			// Set (PB1) pin to an output
	PORTB&=~(1<<1);			// Clear it

	pulseJustDone=true;		//Get ADC
	adcAccumulator=0;
	servoPulseHigh=false;
	lastServoPosition=0;
	adcCount=66;

	while(1)
	{

		// Check to see if we have enough ticks to do the following, then do it:
		// Read ADC value
		// Got new reading?
		// Handle oversampling/averaging
		// Done oversampling/averaging?
		// Read mode switch.  Based on mode switch, figure next servo value.
		
		if(pulseJustDone==true)				// Last pulse is finished, we have time to do the ADC stuff
		{
			if(ADCSRA&(1<<ADIF))			// Got new ADC?
			{
				temp=ADCH;			// Get high 8 bits of result
				ADCSRA|=(1<<ADIF);		// Clear adc conversion done flag

				adcCount--;			// one more reading done

				if(adcCount<64)			// We need 64 readings, anything above that we throw out for the sake of settling time
				{
					adcAccumulator+=temp;	// Add in this result to the accumulated readings

					if(adcCount==0)		// Done accumulating!
					{
						adcResult=(adcAccumulator/7);			// Get as close as we can to a range of 2400 -- (256*64) / 7
						adcAccumulator=0;
						adcCount=66;					// Throw out some readings next time.
												
						if(PINB&(1<<0))					// Switch is high?	ADC result directly to position
						{
							lastServoPosition=adcResult;		// Keep track of our current position
							servoTime=adcResult/4;			// Scale down
						}
						else						// Switch is low
						{
							temp=adcResult/32;			// Get a value, magnitude based on ADC reading
								
							if(servoDirection)			// Going "forward"
							{
								if((lastServoPosition+temp)>=2400)
								{
									servoTime=2400;		// All the way forward
									servoDirection=false;	// Go back after this
								}
								else
								{
									servoTime=lastServoPosition+temp;	// A bit forward
								}
							}
							else	//  Servo going backwards
							{
								if(temp>lastServoPosition)
								{
									servoTime=0;				// All the way back
									servoDirection=true;			// Go forward after this
								}
								else
								{
									servoTime=lastServoPosition-temp;	// A bit back
								}							
							}							
							lastServoPosition=servoTime;				// Keep track of our old time
							servoTime=(servoTime/4);				// Scale down
						}

						pulseJustDone=false;						// Wait until the pulse is finished to re-do this math
					}
					
				}
							
				ADCSRA |= (1<<ADSC);  								// Next conversion
			}			
		}


		if(TIFR&(1<<OCF0A))			// Hit 50Hz counter?
		{
			TIFR|=(1<<OCF0A);		// Clear the interrupt flag				
			servoPulseHigh=true;		// Servo pulse is counting down
			PORTB|=(1<<1);			// Pin high
			// Set servo pin high
			// Start pulse counter, load correct number of times to run the entire counter
			// Run in normal mode, count overflows

		}		
		else if(servoPulseHigh==true)		// Servo loop high
		{
			// Check for overflow
			// Got overflow?
			// Dec number of overflows
			// Overflows == 0?
			// Set to CTC mode, bring pin low on match, load compare register with remainder
			// Servo pulse high flag to false
			// pulseJustDone == true

			for(temp=0;temp<130;temp++)
			{
				MACRO_DoTenNops
				MACRO_DoTenNops
				MACRO_DoTenNops
				MACRO_DoTenNops
				MACRO_DoTenNops			
			}

			while(servoTime)
			{
				servoTime--;
			}

			PORTB&=~(1<<1);			// Pin low
			servoPulseHigh=false;
			pulseJustDone=true;
		}
	}
	return(0);
}

