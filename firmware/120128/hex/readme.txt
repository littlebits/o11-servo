littleBits Servo Controller
Attiny45 (8-pin DIP/SOIC/TSSOP package)


Fuses
EByte: 0xFF (don't touch this one)
HBYTE: 0xDD
LBYTE: 0xCF



Wiring

Port		Pin		Function
-------		----		--------			

PORTB,0		5		Mode Switch
PORTB,1		6		OC1A -- servo pulse waveform out
PORTB,2		7		ADC CV input (ADC1)
PORTB,3		2		XTAL
PORTB,4		3		XTAL
PORTB,5		1		RESET (tie high)



Pin Overview (looking at the top of the chip):
----------------------------------------------

			 ---------------
		RESET ---  *		--- VCC		
			|		|
	   	 XTAL ---		--- ADC input (ADC1)
			|		|
		 XTAL ---		--- OC1A out to servo control line
			|		|	
		  GND ---		--- Mode Switch
			 ---------------
			
