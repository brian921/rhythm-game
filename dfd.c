/*
 * rhythm.c
 *
 * Created: 5/18/2015 10:21:37 AM
 *  Author: briannguyen
 */ 


#include <avr/io.h>
#include "io.c"
#include "timer.h"

void set_PWM(double frequency) {
	
	
	// Keeps track of the currently set frequency
	// Will only update the registers when the frequency
	// changes, plays music uninterrupted.
	static double current_frequency;
	if (frequency != current_frequency) {

		if (!frequency) TCCR3B &= 0x08; //stops timer/counter
		else TCCR3B |= 0x03; // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) OCR3A = 0xFFFF;
		
		// prevents OCR3A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) OCR3A = 0x0000;
		
		// set OCR3A based on desired frequency
		else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

		TCNT3 = 0; // resets counter
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

//global variables used to communicae between different tick functions /////////////
char startFlag = 0; //tells all tick functions that the game will be starting

char button0 = 0x00;	//the buttons to the user interface
char button1 = 0x00;
char button2 = 0x00;
char button3 = 0x00;

unsigned score = 0; //total score of the game
char updateFlag = 0; //flag for when score is updated

char multiplier = 1; //multiplier for combos
					//10 hits -> mult. => 2
					//25 hits -> mult. => 3
					//50 hits -> mult. =>4
					//100 hits -> mult. => 6
unsigned short combo = 0;
char comboFlag = 0;

char buttonHit = 0; //checks whether or not user hits button at right time or not
//char LEDArray[] = {0x01, 0x02, 0x04, 0x08}; //test array for LED and button sync
//char LEDIndex = 0;
/////////////////////////////////////////////////////////////////////////////////


enum buttonStates
{
	startB,
	wait,
	press0,
	waitpress0,
	press1,
	waitpress1,
	press2,
	waitpress2,
	press3,
	waitpress3
}bState;

enum LCDStates
{
	startLCD,
	displayScore,
	IncScore
}LCDState;

enum SM1_States {sm1_display};

/*void transmit_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTD = 0x10;
		// set SER = next bit of data to be sent.
		PORTD |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTD |= 0x04;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTD |= 0x08;
	// clears all lines in preparation of a new transmission
	PORTD = 0x00;
}*/

/*int SM1_Tick(int state) {
	// === Local Variables ===
	static unsigned char column_val = 0x01; // sets the pattern displayed on columns
	static unsigned char column_sel = 0x7F; // grounds column to display pattern

	// === Transitions ===
	switch (state) {
		case sm1_display: break;
		default: state = sm1_display;
		break;
	}

	// === Actions ===
	switch (state) {
		case sm1_display: // If illuminated LED in bottom right corner
		if (column_sel == 0xFE && column_val == 0x80) {
			column_sel = 0x7F; // display far left column
			column_val = 0x01; // pattern illuminates top row
		}
		// else if far right column was last to display (grounded)
		else if (column_sel == 0xFE) {
			column_sel = 0x7F; // resets display column to far left column
			column_val = column_val << 1; // shift down illuminated LED one row
		}
		// else Shift displayed column one to the right
		else {
			column_sel = (column_sel >> 1) | 0x80;
		}
		break;
		default: break;
	}
	transmit_data(column_val);
	PORTD = column_val; // PORTA displays column pattern
	PORTA = column_sel; // PORTB selects column to display pattern
	return state;
};
/*enum LEDStates
{
	startLED,
	lightLED
}LEDState;*/

/*int LEDtickfct(int state)
{
	switch( LEDState )
	{
		case startLED:
			if( startFlag )
				LEDState = lightLED;
			else
				LEDState = startLED;
			break;
		case lightLED:
			LEDState = startLED;
			break;
		default:
			LEDState = startLED;
			break;
	}
	
	switch( LEDState )
	{
		case startLED:
			break;
		case lightLED:
			if( LEDIndex < 4 )
			{
				PORTA = LEDArray[ LEDIndex ];
				LEDIndex ++;
			}
			else
				LEDIndex = 0;

			break;
		default:
			break;
	}
}*/

int LCDtickfct(int state)		//keep fixing hits and misses
{	
	switch( LCDState )
	{
		case startLCD:
			if( button0 || button1 || button2 || button3 )	//press any button to continue
				LCDState = displayScore;
			else
				LCDState = startLCD;	//if no button is pressed continue to display "Press Any Key to Play
			break;
		case displayScore:
			if( button0 )					//fix this shit later
				LCDState = IncScore;
			else if( button1 )
				LCDState = IncScore;
			else if( button2 )
				LCDState = IncScore;
			else if( button3 )
				LCDState = IncScore;
			else
				LCDState = displayScore;	//displays the score otherwise*/
			break;
		case IncScore:
			LCDState = displayScore;
			break;
		default:
			LCDState = startLCD;
	}
	
	switch( LCDState )
	{
		case startLCD:
			break;		
		case displayScore:
			startFlag = 1;
			
			if( updateFlag )
			{
				LCD_ClearScreen();
				LCD_Cursor(1);
							
				LCD_DisplayString( 1, "Score: ");
				
				LCD_WriteData( score/1000000 + '0' );
				LCD_WriteData( score%1000000 / 100000 + '0' );
				LCD_WriteData( score%1000000 % 100000 /10000 + '0' );
				LCD_WriteData( score%1000000 % 100000 % 10000 /1000 + '0' );
				LCD_WriteData( score%1000000 % 100000 % 10000 % 1000 /100 + '0' );
				LCD_WriteData( score%1000000 % 100000 % 10000 % 1000 % 100 /10 + '0' );
				LCD_WriteData( score%1000000 % 100000 % 10000 % 1000 % 100 % 10 + '0' );
							
				LCD_Cursor( 17 );	//combo display
				LCD_WriteData( 'C' );
				LCD_WriteData( 'o' );
				LCD_WriteData( 'm' );
				LCD_WriteData( 'b' );
				LCD_WriteData( 'o' );
				LCD_WriteData( ':' );
				LCD_WriteData( ' ' );
							
				LCD_WriteData( combo/1000000 + '0' );
				LCD_WriteData( combo%1000000 / 100000 + '0' );
				LCD_WriteData( combo%1000000 % 100000 /10000 + '0' );
				LCD_WriteData( combo%1000000 % 100000 % 10000 /1000 + '0' );
				LCD_WriteData( combo%1000000 % 100000 % 10000 % 1000 /100 + '0' );
				LCD_WriteData( combo%1000000 % 100000 % 10000 % 1000 % 100 /10 + '0' );
				LCD_WriteData( combo%1000000 % 100000 % 10000 % 1000 % 100 % 10 + '0' );
							
				updateFlag = 0;
				comboFlag = 1;
				combo++;
			}

			break;
			
		case IncScore:
			score = multiplier*10 + score;
			switch( combo )
			{
				case 10:
					multiplier = 2;
					break;
				case 25:
					multiplier++;
					break;
				case 50:
					multiplier++;
					break;
				case 100:
					multiplier = 6;
					break;
			}
			break;
		default:
			break;
	}
	
}

int buttonTickfct(int state)
{
	button0 = ~PINB & 0x01;
	button1 = ~PINB & 0x02;
	button2 = ~PINB & 0x04;
	button3 = ~PINB & 0x08;
	
	switch( bState )
	{
		case startB:
			if( !button0 )
				bState = wait;
			else if( !button1 )
				bState = wait;
			else if( !button2 )
				bState = wait;
			else if( !button3 )
				bState = wait;
			else
				bState = startB;
			break;
			
		case wait:
			if( button0 && !button1 && !button2 && !button3 )
				bState = press0;
			else if( button1 && !button0 && !button2 && !button3 )
				bState = press1;
			else if( button2 && !button0 && !button1 && !button3 )
				bState = press2;
			else if( button3 && !button0 && !button1 && !button2 )
				bState = press3;
			else
				bState = wait;
			break;
		
		case press0:
			if( button0 && !button1 && !button2 && !button3 )
				bState = waitpress0;
			else
				bState = wait;
			break;
		
		case waitpress0:
			if( button0 && !button1 && !button2 && !button3 )
				bState = waitpress0;
			else
				bState = wait;
			break;
			
		case press1:
			if( button1 && !button0 && !button2 && !button3 )
				bState = waitpress1;
			else
				bState = wait;
			break;
		
		case waitpress1:
			if( button1 && !button0 && !button2 && !button3 )
				bState = waitpress1;
			else
				bState = wait;
			break;
			
		case press2:
			if( button2 && !button0 && !button1 && !button3 )
				bState = waitpress2;
			else
				bState = wait;
			break;
		
		case waitpress2:
			if( button2 && !button0 && !button1 && !button3 )
				bState = waitpress2;
			else
				bState = wait;
			break;
			
		case press3:
			if( button3 && !button0 && !button1 && !button2 )
				bState = waitpress3;
			else
				bState = wait;
			break;
		
		case waitpress3:
			if( button3 && !button0 && !button1 && !button2 )
				bState = waitpress3;
			else
				bState = wait;
			break;
	
		default:
			bState = startB;
			break;
	}
	switch( bState )
	{
		case startB:
			break;
		case wait:
			break;
		case press0:
			if( !startFlag )	//change later when using matrix
				updateFlag = 1;
			/*else 
			{
				combo = 0;
				updateFlag = 0;
			}*/
			break;
			
		case waitpress0:
			break;
			
		case press1:
			if( !startFlag )
				updateFlag = 1;
			/*else
			{
				combo = 0;
				updateFlag = 1;
			}*/
			break;
		
		case waitpress1:
			break;
			
		case press2:
			if( !startFlag )
				updateFlag = 1;
			/*else
			{
				combo = 0;
				updateFlag = 1;
			}*/
			break;
		
		case waitpress2:
			break;
			
		case press3:
			if( !startFlag )
				updateFlag = 1;
			/*else
			{
				combo = 0;
				updateFlag = 1;
			}*/
			break;
		
		case waitpress3:
			break;
			
		default:
			break;
	}
};

typedef struct task {
	int state; // Current state of the task
	unsigned long period; // Rate at which the task should tick
	unsigned long elapsedTime; // Time since task's previous tick
	int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[2];
const unsigned char tasksNum = 2;
const unsigned long tasksPeriodGCD = 1;

int main(void)
{
	DDRB = 0x40; PORTB = 0xBF;
	DDRA = 0xFF; PORTA = 0x00;
	DDRC = 0xFF; PORTC = 0x00; //LCD data lines
	DDRD = 0xFF; PORTD = 0x00; //LCD control lines
	
	PWM_on();
	
	LCD_init();
	LCD_ClearScreen();
	LCD_DisplayString(1, "Press Any Key to Play");

	TimerSet(1);
	TimerOn();
	
	
	unsigned char i=0;
	tasks[i].state = startB;
	tasks[i].period = 1;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &buttonTickfct; 
	i++;
	tasks[i].state = startLCD;
	tasks[i].period = 1;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &LCDtickfct;
	/*i++;
	tasks[i].state = sm1_display;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM1_Tick;*/
	
	TimerSet(tasksPeriodGCD);
	TimerOn();
	
	
    while(1)
    {	
		for (i=0; i < tasksNum; i++) {
			if (tasks[i].elapsedTime >= tasks[i].period){
				// Task is ready to tick, so call its tick function
				tasks[i].state = tasks[i].TickFct(tasks[i].state);
				tasks[i].elapsedTime = 0; // Reset the elapsed time
			}
			tasks[i].elapsedTime += tasksPeriodGCD;
		}

		while (!TimerFlag);
		TimerFlag = 0;
	}
	
}