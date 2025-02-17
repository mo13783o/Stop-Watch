#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>



unsigned char ticks = 0;
unsigned char sec = 0;
unsigned char min = 0;
unsigned char hour = 0;
unsigned char reset_flag=0;
unsigned char pause_flag = 0;
unsigned char resume_flag =0;
unsigned char count_down_flag=0;
unsigned char adjust_flag=0;
unsigned char button_pressed = 0;



void count_up(void);
void count_down (void);
void timer_1_setup(void);
void INT0_Init(void);
void INT1_Init(void);
void INT2_Init(void);
void WDT_OFF(void);
void WDT_ON(void);
void reset (void);
void pause(void);
void resume(void);
void set_up(void);
void adjust (void);

int main(void)
{

	DDRC = 0x0F; // 7_segment_numbers _pins
	DDRA = 0x3F; //(7-segment enable)
	DDRD |= (1 << PD4); // Red led indicate count up
	DDRD |=(1<<7);// LED INDICATE TIMER INT WORKING
	DDRD |=(1<<5);//YELLOW LED INDICATE FOR COUNT DOWN
	DDRB &= ~((1 << PB0) | (1 << PB1) | (1 << PB3) | (1 << PB4) | (1 << PB5) | (1 << PB6));// control switches
	DDRD |=(1<<1);
	PORTD &= ~(1<<5);//MAKE THE LED OFF AT FIRST
	PORTC &= ~(0x0F); // Clear lower nibble of PORTC
	PORTD &= ~(1 << PD4); // Ensure LED is OFF initially
	PORTB = 0X7F;
	SREG |= (1 << 7); // Enable global interrupts
	timer_1_setup();  // Start Timer1
	INT0_Init();// Initialize INT0
	INT1_Init();
	INT2_Init();

	while (1) {
		if (PINB &(1<<7))
		{
			count_down_flag=1;
			count_down();
		}
		else
		{
			count_up();
		}
		adjust();
		reset();
		pause();
		resume();
	}
}
void timer_1_setup(void)
{
	TCNT1 = 0;  // Start counting from 0
	TCCR1A = (1 << FOC1A); // Enable Compare A Mode
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // Pre-scaler 1024, CTC Mode
	OCR1A = 15624; // Compare value for 1 second
	TIMSK |= (1 << OCIE1A); // Enable Timer1 Compare Interrupt
}

ISR(TIMER1_COMPA_vect)
{
	if (count_down_flag)
	{
		ticks--;
	}
	else
	{
		ticks++;
	}

	PORTD |=(1<<7);
}

void count_up(void)
{
	if (ticks >= 60) // If 60 seconds pass, reset and increment minutes
	{
		ticks = 0;
		min++;
	}
	if (min >= 60) // If 60 minutes pass, reset and increment hours
	{
		min = 0;
		hour++;
	}
	if (hour >= 24) // If 24 hours pass, reset to 0
	{
		hour = 0;
	}

	// Extract digits for each display
	unsigned char sec_ones  = ticks % 10;
	unsigned char sec_tens  = (ticks / 10) % 10;
	unsigned char min_ones  = min % 10;
	unsigned char min_tens  = (min / 10) % 10;
	unsigned char hour_ones = hour % 10;
	unsigned char hour_tens = (hour / 10) % 10;

	// Turn on LED to indicate counting
	PORTD |= (1 << PD4);

	// Refresh each 7-segment display
	PORTA = (1 << 5);  // Enable 1st display (Seconds, ones)
	PORTC = (PORTC & 0xF0) | sec_ones;
	_delay_ms(2);

	PORTA = (1 << 4);  // Enable 2nd display (Seconds, tens)
	PORTC = (PORTC & 0xF0) | sec_tens;
	_delay_ms(2);

	PORTA = (1 << 3);  // Enable 3rd display (Minutes, ones)
	PORTC = (PORTC & 0xF0) | min_ones;
	_delay_ms(2);

	PORTA = (1 << 2);  // Enable 4th display (Minutes, tens)
	PORTC = (PORTC & 0xF0) | min_tens;
	_delay_ms(2);

	PORTA = (1 << 1);  // Enable 5th display (Hours, ones)
	PORTC = (PORTC & 0xF0) | hour_ones;
	_delay_ms(2);

	PORTA = (1 << 0);  // Enable 6th display (Hours, tens)
	PORTC = (PORTC & 0xF0) | hour_tens;
	_delay_ms(2);
}

void INT0_Init(void)
{
	DDRD &= ~(1 << PD2); // Configure INT0 (PD2) as input
	PORTD |= (1 << PD2); // Enable internal pull-up resistor on PD2
	MCUCR |= (1 << ISC01); // Falling edge triggers INT0
	GICR |= (1 << INT0);   // Enable external interrupt for INT0
}
void INT1_Init(void)
{
	DDRD &= ~(1 << PD3); // Configure INT1 (PD3) as input // Configure INT0 (PD2) as input
	//PORTD |= (1 << PD3);
	MCUCR |= (1 << ISC11)|(1<<ISC10); // REISING edge triggers
	GICR |= (1 << INT1);   // Enable external interrupt for INT0
}
void INT2_Init(void)
{
	DDRB &= ~(1 << PB2); // Configure INT0 (PD2) as input
	PORTB |= (1 << PB2); // Enable internal pull-up resistor on PD2
	MCUCSR &= ~(1 << ISC2); // Falling edge triggers INT2
	GICR |= (1 << INT2);   // Enable external interrupt for INT0
}
void WDT_ON(void)
{
	// Watch_dog timer enables with timeout period 2.1 second.
	WDTCR = (1<<WDE)|(1<<WDP2)|(1<<WDP1)|(1<<WDP0);
}

// 	function to disable Watch_dog timer.
void WDT_OFF(void)
{
	// Set the WDTOE & WDE bits in the same operation
	WDTCR = (1<<WDTOE)|(1<<WDE);
	// Wait 4 cycles before clear the WDE bit
	_delay_us(4);
	WDTCR = 0x00;
}
ISR(INT0_vect)
{
	set_up();
	reset_flag=1;
	GIFR |= (1 << INTF0); // Clear interrupt flag

}
ISR(INT1_vect)
{

	pause_flag = 1; // Set pause flag
	GIFR |= (1 << INTF1); // Clear interrupt flag
}
ISR(INT2_vect)
{

	resume_flag=1;
	GIFR |= (1 << INTF2);
}

void reset(void) {
	if (reset_flag) {
		ticks = 0;
		min = 0;
		hour = 0;
		count_down_flag = 0;
		PORTD &= ~(1 << PD5); // Turn off countdown LED
		PORTD &= ~(1 << PD6); // Turn off alarm
		// Trigger watchdog reset
		WDT_ON();
		while(1); // Wait for watchdog to reset
	}
}
void pause(void)
{
	if (pause_flag)
	{
		TCCR1B &= ~((1 << CS12) | (1 << CS10)| (1 << CS11)); // Stop Timer1
		pause_flag = 0; // Clear the flag
		adjust_flag=1;
		PORTD &= ~(1<<7);
	}
}
void resume(void)
{
	if (resume_flag)
	{
		TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
		resume_flag=0;
		adjust_flag=0;
	}

}
void count_down(void)
{
	if (count_down_flag)
	{
		if (hour == 0 && min == 0 && ticks == 0) {
			PORTD |= (1 << PD0);    // Signal end of countdown
			count_down_flag = 0;     // Disable countdown mode

			// *** STOP TIMER COMPLETELY ***
			TCCR1B = 0;              // Disable Timer1 by clearing all prescaler bits
			TIMSK &= ~(1 << OCIE1A); // Disable Timer1 Compare Match Interrupt
			TIFR |= (1 << OCF1A);


			return; // Exit function to prevent further counting
		}

		if (ticks == 0)
		{
			if (min == 0)
			{
				if (hour == 0)
				{
					// *** STOP TIMER WHEN TIME REACHES ZERO ***
					ticks = 0;
					min = 0;
					hour = 0;
					TCCR1B = 0;               // Disable Timer1 completely
					TIMSK &= ~(1 << OCIE1A);  // Disable Timer Interrupt
					PORTD |= (1 << PD0);// Signal that countdown finished
					PORTD |= (1 << PD1);
					return;                   // Exit function
				}
				else
				{
					min = 59;
					hour--;
				}
			}
			else
			{
				ticks = 59;
				min--;
			}
		}

		// Extract digits for each display
		unsigned char sec_ones  = ticks % 10;
		unsigned char sec_tens  = (ticks / 10) % 10;
		unsigned char min_ones  = min % 10;
		unsigned char min_tens  = (min / 10) % 10;
		unsigned char hour_ones = hour % 10;
		unsigned char hour_tens = (hour / 10) % 10;


		PORTD |= (1 << PD5); // Yellow LED for countdown

		// Refresh each 7-segment display
		PORTA = (1 << 5);  // Enable 1st display (Seconds, ones)
		PORTC = (PORTC & 0xF0) | sec_ones;
		_delay_ms(2);

		PORTA = (1 << 4);  // Enable 2nd display (Seconds, tens)
		PORTC = (PORTC & 0xF0) | sec_tens;
		_delay_ms(2);

		PORTA = (1 << 3);  // Enable 3rd display (Minutes, ones)
		PORTC = (PORTC & 0xF0) | min_ones;
		_delay_ms(2);

		PORTA = (1 << 2);  // Enable 4th display (Minutes, tens)
		PORTC = (PORTC & 0xF0) | min_tens;
		_delay_ms(2);

		PORTA = (1 << 1);  // Enable 5th display (Hours, ones)
		PORTC = (PORTC & 0xF0) | hour_ones;
		_delay_ms(2);

		PORTA = (1 << 0);  // Enable 6th display (Hours, tens)
		PORTC = (PORTC & 0xF0) | hour_tens;
		_delay_ms(2);
	}
}

void set_up(void)
{
	pause_flag = 0;
	resume_flag =0;
	count_down_flag=0;
	timer_1_setup();
}

void adjust(void) {
	if (adjust_flag) {
		if (!(PINB & (1 << PB6))) {  // Check if button is pressed (active LOW)
			if (!button_pressed) {  // Ensure the flag prevents multiple increments per press
				button_pressed = 1;  // Set flag to indicate button is handled

				if (ticks == 59) {
					ticks = 0;  // Reset to 0 if max is reached
				} else {
					ticks++;  // Increment the counter
				}

				// Display the required number on the 7-segment display
				PORTA = (1 << 5);
				PORTC = (PORTC & 0xF0) | (ticks % 10);
				_delay_ms(2);

				PORTA = (1 << 4);
				PORTC = (PORTC & 0xF0) | ((ticks / 10) % 10);
				_delay_ms(2);
			}
		}
		else if (!(PINB & (1 << PB5))) {  // Check if another button (PB7) is pressed for decrement
			if (!button_pressed) {
				button_pressed = 1;

				if (ticks == 0) {
					ticks = 59;  // Reset to 59 if 0 is reached
				} else {
					ticks--;  // Decrement the counter
				}

				// Display the required number on the 7-segment display
				PORTA = (1 << 5);
				PORTC = (PORTC & 0xF0) | (ticks % 10);
				_delay_ms(2);

				PORTA = (1 << 4);
				PORTC = (PORTC & 0xF0) | ((ticks / 10) % 10);
				_delay_ms(2);
			}
		}

		else if (!(PINB & (1 << PB4)))
		{
			if (!button_pressed) {
				button_pressed = 1;
				if (min == 59) {
					min = 0;  // Reset to 0 if max is reached
				} else {
					min++;  // Increment the counter
				}

				PORTA = (1 << 3);  // Enable 3rd display (Minutes, ones)
				PORTC = (PORTC & 0xF0) |(min % 10) ;
				_delay_ms(2);

				PORTA = (1 << 2);  // Enable 4th display (Minutes, tens)
				PORTC = (PORTC & 0xF0) | ((min / 10) % 10);
				_delay_ms(2);

			}
		}
		else if (!(PINB & (1 << PB3)))
		{
			if (!button_pressed) {
				button_pressed = 1;
				if (min == 0) {
					min = 59;  // Reset to 0 if max is reached
				} else {
					min--;  // Increment the counter
				}

				PORTA = (1 << 3);  // Enable 3rd display (Minutes, ones)
				PORTC = (PORTC & 0xF0) |(min % 10) ;
				_delay_ms(2);

				PORTA = (1 << 2);  // Enable 4th display (Minutes, tens)
				PORTC = (PORTC & 0xF0) | ((min / 10) % 10);
				_delay_ms(2);

			}
		}
		else if (!(PINB & (1 << PB1)))
		{
			if (!button_pressed) {
				button_pressed = 1;
				if (hour == 24) {
					hour = 0;  // Reset to 0 if max is reached
				} else {
					hour++;  // Increment the counter
				}
				PORTA = (1 << 1);  // Enable 5th display (Hours, ones)
				PORTC = (PORTC & 0xF0) | (hour % 10) ;
				_delay_ms(2);

				PORTA = (1 << 0);  // Enable 6th display (Hours, tens)
				PORTC = (PORTC & 0xF0) | ((hour / 10) % 10);
				_delay_ms(2);

			}
		}
		else if (!(PINB & (1 << PB0)))
		{
			if (!button_pressed) {
				button_pressed = 1;
				if (hour == 0) {
					hour = 0;  // Reset to 0 if max is reached
				} else {
					hour--;  // Increment the counter
				}
				PORTA = (1 << 1);  // Enable 5th display (Hours, ones)
				PORTC = (PORTC & 0xF0) | (hour % 10) ;
				_delay_ms(2);

				PORTA = (1 << 0);  // Enable 6th display (Hours, tens)
				PORTC = (PORTC & 0xF0) | ((hour / 10) % 10);
				_delay_ms(2);

			}
		}
		else {
			button_pressed = 0;  // Reset flag when no button is pressed
		}

	}
}


