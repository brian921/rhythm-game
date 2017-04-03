/*
 * rhythm.0xF3
 *
 * Created: 5/18/2015 10:21:37 AM
 *  Author: briannguyen
 */ 


#include <avr/io.h>
#include <stdlib.h>
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
	// CS31 & CS30: Set 0xF3 prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

//global variables used to communicate between different tick functions /////////////
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
char LEDIndex = 0;
char finishedLineFlag = 1;
char gameDone = 0;


char hitArray[] = 
{
	0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF, 0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF,
	0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F, 0xFC, 0xFC, 0xFC, 0xFC, 0xCF, 0xCF, 0xF3, 0xF3,
	0xCF, 0xCF, 0xCF, 0xCF, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0xFC, 0xFC,
	0x3F, 0x3F, 0xCF, 0xCF, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
	
	0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF, 0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF,
	0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F, 0xFC, 0xFC, 0xFC, 0xFC, 0xCF, 0xCF, 0xF3, 0xF3,
	0x3F, 0x3F, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF,
	0xCF, 0xCF, 0xFC, 0xFC, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
	
	0xFC, 0xFC, 0x3F, 0x3F, 0xFC, 0xFC, 0xFC, 0xFC, 0xCF, 0xCF, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF,
	0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
	0xFC, 0xFC, 0xCF, 0xCF, 0xF3, 0xF3, 0xF3, 0xF3, 0xCF, 0xCF, 0xF3, 0xF3, 0xFC, 0xFC, 0xFC, 0xFC,
	0x3F, 0x3F, 0xFC, 0xFC, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF,
	
	0xFC, 0xFC, 0x3F, 0x3F, 0xFC, 0xFC, 0xFC, 0xFC, 0xCF, 0xCF, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF,
	0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
	0xFC, 0xFC, 0x3F, 0x3F, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF, 0xF3, 0xF3, 0xF3, 0xF3, 0xCF, 0xCF,
	0xFC, 0xFC, 0xF3, 0xF3, 0x3F, 0x3F, 0xFC, 0xFC, 0xFC, 0xFC, 0x3F, 0x3F, 0x3F, 0xFC, 0xFC, 0xFC,
	
	0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
	0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F
};
char i = 0;
/////////////////////////////////////////////////////////////////////////////////


enum buttonStates
{
	startB,
	wait,
	press0,
	press1,
	press2,
	press3,
}bState;

enum LCDStates
{
	startLCD,
	IncScore
}LCDState;

enum LEDStates
{
	startLED,
	lightLED
}LEDState;

#define C 261.63
#define Db 277.18
#define D 293.66
#define Eb 311.13
#define E 329.63
#define F 349.23
#define Gb 369.99
#define G 392.00
#define Ab 415.30
#define A 440.00
#define Bb 466.16
#define B 493.88

typedef unsigned char byte;

double music[] =
{
	F, F, A, A, B, B, B, B, F, F, A, A, B, B, B, B,
	F, F, A, A, B, B, E, E, D, D, D, D, B, B, C, C,
	B, B, G, G, E, E, E, E, E, E, E, E, E, E, D, D,
	E, E, G, G, E, E, E, E, E, E, E, E, E, E, E, E,
	
	F, F, A, A, B, B, B, B, F, F, A, A, B, B, B, B,
	F, F, A, A, B, B, E, E, D, D, D, D, B, B, C, C,
	E, E, B, B, G, G, G, G, G, G, G, G, G, G, B, B,
	G, G, D, D, E, E, E, E, E, E, E, E, E, E, E, E,
	
	D, D, E, E, F, F, F, F, G, G, A, A, B, B, B, B,
	C, C, B, B, E, E, E, E, E, E, E, E, E, E, E, E,
	F, F, G, G, A, A, A, A, B, B, C, C, D, D, D, D,
	E, E, F, F, G, G, G, G, G, G, G, G, G, G, G, G,
	
	D, D, E, E, F, F, F, F, G, G, A, A, B, B, B, B,
	C, C, B, B, E, E, E, E, E, E, E, E, E, E, E, E,
	F, F, E, E, A, A, G, G, B, B, A, A, C, C, B, B,
	D, D, C, C, E, E, D, D, F, F, E, E, E, F, F, D,
	
	E, E, E, E, E, E, E, E, E, E, E, E, E, E, E, E,
	E, E, E, E, E, E, E, E, E, E, E, E, E, E, E, E
};

unsigned octave[] =
{
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 5, 5, 6, 6,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 5, 5, 6, 6,
	6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

byte on[] =
{
	1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0,
	1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0,
	1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0,
	1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	
	1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0,
	1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0,
	1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0,
	1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	
	1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0,
	1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0,
	1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	
	1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0,
	1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0,
	1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1,
	
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

typedef enum
{
	MUS_INIT,
	MUS_PLAY
} SM_Music;

SM_Music state;
unsigned musicTimer;
unsigned cnt;
const unsigned MUSIC_LENGTH = 288;
const unsigned BPM = 110;

void play_note(double note, unsigned oct, byte flag)
{
	if (flag)
	set_PWM(note * (1 << (oct - 4)));
	else
	set_PWM(0);
}

int  music_tick( int state )
{
	switch (state)
	{
		case MUS_INIT:
			if (startFlag)
				state = MUS_PLAY;
			break;
			
		case MUS_PLAY:
				cnt++;
		
			if (cnt == 115)
			{
				musicTimer++;
				cnt = 0;
			}
		
			if (musicTimer == MUSIC_LENGTH)
			{
				state = MUS_INIT;
			}
			break;
		
		default:
		break;
	}
	switch (state)
	{
		case MUS_PLAY:
			play_note(music[musicTimer], octave[musicTimer], on[musicTimer]);
			break;
		
		default:
			set_PWM(0);
			break;
	}
	
	return state;
}

void transmit_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTD = 0x08;
		// set SER = next bit of data to be sent.
		PORTD |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTD |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTD |= 0x04;
	// clears all lines in preparation of 0xF3 new transmission
	PORTD = 0x00;
}

enum SM1_States {sm1_display}; 
	
int SM1_Tick(int state) {
	// === Local Variables ===
	static unsigned char column_val = 0x01; // sets the pattern displayed on columns
	static unsigned char column_sel = 0xFC; // grounds column to display pattern
		
	// === Transitions ===
	switch (state) {
		case sm1_display: break;
		default: state = sm1_display;
		break;
	}

	// === Actions ===
	switch (state) {
		case sm1_display: 
		if( startFlag && !gameDone )
		{
			if( finishedLineFlag )
			{
				column_sel = hitArray[i];
				finishedLineFlag = 0;
			}
			
			if( column_val == 0x01 )
				column_val = 0x02;
			else if( column_val == 0x02 )
				column_val = 0x08;
			else if( column_val == 0x08 )
				column_val = 0x10;
			else if( column_val == 0x10 )
				column_val = 0x20;
			else if( column_val == 0x20 )
				column_val = 0x40;
			else if( column_val == 0x40 )
			{
				column_val = 0x80;
				finishedLineFlag = 1;
			}
			else
			{
				column_val = 0x01;
				if( i < 287 )
					i++;
				else
				{
					gameDone = 1;
					startFlag = 0;
				}
			}
		}
		
			
		break;
		default: break;
	}
	transmit_data(column_val);
	PORTD = column_val; // PORTA displays column pattern
	PORTA = column_sel; // PORTB selects column to display pattern
	return state;
};

int LCDtickfct(int state)		//keep fixing hits and misses
{	
	switch( LCDState )
	{
		case startLCD:
			if( updateFlag && startFlag )
				LCDState = IncScore;
			else
				LCDState = startLCD;	//if no button is pressed continue to display "Press Any Key to Play
			break;
		case IncScore:
			LCDState = startLCD;
			break;
		default:
			LCDState = startLCD;
	}
	
	switch( LCDState )
	{
		case startLCD:
			if( button0 || button1 || button2 || button3 )
				startFlag = 1;
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
			
			combo++;
			updateFlag = 0;
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
				bState = wait;
			break;
		
		case press1:
				bState = wait;
			break;
			
		case press2:
				bState = wait;
			break;

		case press3:
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
			if( ( ( PINA == 0x3F && PIND == 0x80 ) || !startFlag ) )	//change later when using matrix
			{
				updateFlag = 1;
				set_PWM( music[i]);
			}
			break;
			
		case press1:
			if( ( ( PINA == 0xCF && PIND == 0x80 ) ||  !startFlag ) )
			{
				updateFlag = 1;
				set_PWM( music[i]);
			}
			break;
			
		case press2:
			if( ( ( PINA == 0xF3 && PIND == 0x80 ) || !startFlag ) )
			{
				updateFlag = 1;
				set_PWM( music[i]);
			}
			break;
			
		case press3:
			if( ( ( PINA == 0xFC && PIND == 0x80 ) || !startFlag ) )
			{
				updateFlag = 1;
				set_PWM( music[i]);
			}
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

task tasks[3];
const unsigned char tasksNum = 3;
const unsigned long tasksPeriodGCD = 1;

int main(void)
{
	DDRB = 0x70; PORTB = 0xBF;
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
	tasks[i].period = 20;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &buttonTickfct; 
	i++;
	tasks[i].state = startLCD;
	tasks[i].period = 1;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &LCDtickfct;
	i++;
	tasks[i].state = sm1_display;
	tasks[i].period = 50;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM1_Tick;
	/*i++;
	tasks[i].state = MUS_INIT;
	tasks[i].period = 1;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &state;*/
	
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