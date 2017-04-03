/*
 * rhythm.0xF3
 *
 * Created: 5/18/2015 10:21:37 AM
 *  Author: briannguyen
 */ 


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include "io.c"
#include "timer.h"
#include "transdata.c"
#include "PMW.c"

//global variables used to communicate between different tick functions /////////////
typedef struct task
{
	int state; // Current state of the task
	unsigned long period; // Rate at which the task should tick
	unsigned long elapsedTime; // Time since task's previous tick
	int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[3];
const unsigned char tasksNum = 3;
const unsigned char tasksPeriodGCD = 1;

char startFlag = 0; //tells all tick functions that the game will be starting

char button0 = 0x00;	//the buttons to the user interface
char button1 = 0x00;
char button2 = 0x00;
char button3 = 0x00;

unsigned score = 0; //total score of the game
unsigned highscore = 0;
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


unsigned short hitArray[] = 
{
	0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF, 0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF,
	0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F, 0xFC, 0xFC, 0xFC, 0xFC, 0xCF, 0xCF, 0xF3, 0xF3,
	0xCF, 0xCF, 0xCF, 0xCF, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0xFC, 0xFC,
	0x3F, 0x3F, 0xCF, 0xCF, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0xCF, 0x3F, 0xCF, 0xCF, 0x3F, 0xCF,
	
	0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF, 0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0xCF, 0xCF,
	0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F, 0xF3, 0xCF, 0x3F, 0xF3, 0xCF, 0x3F, 0xF3, 0xF3,
	0x3F, 0x3F, 0xCF, 0xCF
};
unsigned short hitIndex = 0; //index of what the current pattern will be
unsigned short matrixPeriod = 400; //period that will be used for LED matrix, this will increase accordingly
unsigned short lcdPeriod = 1;
unsigned short maxHits = 99;

char updatePeriod = 0;
char trueStartFlag = 0;

char stopInc = 0;
char scoredisplayFlag = 0;

char column_val = 0x01; // sets the pattern displayed on columns
char column_sel = 0xFC; // grounds column to display pattern
char resetFlag = 0;
char segments = 0; //segements of letter grade

const uint8_t omega[] PROGMEM =
{
	0b00000,
	0b01110,
	0b10001,
	0b10001,
	0b10001,
	0b01010,
	0b11011,
	0b00000
};

void LCDdefinechar( const uint8_t* pc, uint8_t char_code )
{
	uint8_t a, pcc;
	uint16_t i;
	
	a = ( char_code << 3 )| 0x40;
	
	for( i = 0; i < 8; i++ )
	{
		pcc = pgm_read_byte( & pc[i] );
		LCD_WriteCommand( a++ );
		LCD_WriteData(pcc);
	}
}

char beatHighScoreFlag = 0;
/////////////////////////////////////////////////////////////////////////////////


//states for all ticks//////////////////////////////////////////////////////////
enum buttonStates
{
	startB,
	wait,
	resetB,
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

enum SM1_States {sm1_display}; 
//////////////////////////////////////////////////////////////////////////

void reset()
{
	score = 0;
	combo = 0;
	startFlag = 0;
	gameDone = 0;
	
	column_val = 0x00;
	column_sel = 0xFC;
	hitIndex = 0;
	matrixPeriod = 400;
	updatePeriod = 1;
	lcdPeriod = 1;
	
	LCD_ClearScreen();
	LCD_DisplayString(1, "Press Any Key to Play");
	
	multiplier = 1;
	resetFlag = 1;
	beatHighScoreFlag = 0;
}

int SM1_Tick(int state) 
{	
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
				column_sel = hitArray[ hitIndex ];
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
				
				if( hitIndex < maxHits )
				{
					hitIndex ++;
					
					switch( hitIndex )
					{
						case 9:
							matrixPeriod = 350;
							updatePeriod = 1;
							break;
						case 19:
							matrixPeriod = 300;
							updatePeriod = 1;
							break;
						case 29:
							matrixPeriod = 250;
							updatePeriod = 1;
							break;
						case 39:
							matrixPeriod = 200;
							updatePeriod = 1;
							break;
						case 49:
							matrixPeriod = 150;
							updatePeriod = 1;
							break;
						case 59:
							matrixPeriod = 100;
							updatePeriod = 1;
							break;
						case 89:
							matrixPeriod = 50;
							updatePeriod = 1;
							break;
					}
				}
				else
				{
					gameDone = 1;
					startFlag = 0;
					
					lcdPeriod = 3000;
					
					if( score > highscore )
					{
						highscore = score;
						beatHighScoreFlag = 1;
					}
					
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
			if( button0 || button1 || button2 || button3){
				startFlag = 1;
			}
			
			if( gameDone )
			{
				startFlag = 0;
				
				if( !scoredisplayFlag )
				{
					LCD_DisplayString( 1, "Score: ");
					
					LCD_WriteData( score%1000000 % 100000 /10000 + '0' );
					LCD_WriteData( score%1000000 % 100000 % 10000 /1000 + '0' );
					LCD_WriteData( score%1000000 % 100000 % 10000 % 1000 /100 + '0' );
					LCD_WriteData( score%1000000 % 100000 % 10000 % 1000 % 100 /10 + '0' );
					LCD_WriteData( score%1000000 % 100000 % 10000 % 1000 % 100 % 10 + '0' );
					scoredisplayFlag = 1;
					
					if( beatHighScoreFlag )
					{
						LCDdefinechar( omega, 1 );
						LCD_Cursor(16);
						LCD_WriteData( 0b00001001 );
					}
					if( score < 12000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'F' );
						
						if( beatHighScoreFlag )
						{
							LCDdefinechar( omega, 1 );
							LCD_Cursor(32);
							LCD_WriteData( 0b00001001 );
						}
				
					}
					else if( score >= 12000 && score < 14000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'D' );
						
						if( beatHighScoreFlag )
						{
							LCDdefinechar( omega, 1 );
							LCD_Cursor(32);
							LCD_WriteData( 0b00001001 );
						}
					}
					else if( score >= 14000  && score < 16000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'C' );
						
						if( beatHighScoreFlag )
						{
							LCDdefinechar( omega, 1 );
							LCD_Cursor(32);
							LCD_WriteData( 0b00001001 );
						}
					}
					else if( score >= 16000 && score < 18000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'B' );
						
						if( beatHighScoreFlag )
						{
							LCDdefinechar( omega, 1 );
							LCD_Cursor(32);
							LCD_WriteData( 0b00001001 );
						}
					}
					else if( score >= 18000 && score < 20000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'A' );
					}
					else if( score >= 20000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'S' );
						
						if( beatHighScoreFlag )
						{
							LCDdefinechar( omega, 1 );
							LCD_Cursor(32);
							LCD_WriteData( 0b00001001 );
						}
					}
					scoredisplayFlag = 1;
				}
				else if( scoredisplayFlag == 1 )
				{
					LCD_DisplayString( 1, "H. Score: ");
					LCD_WriteData( highscore%1000000 % 100000 /10000 + '0' );
					LCD_WriteData( highscore%1000000 % 100000 % 10000 /1000 + '0' );
					LCD_WriteData( highscore%1000000 % 100000 % 10000 % 1000 /100 + '0' );
					LCD_WriteData( highscore%1000000 % 100000 % 10000 % 1000 % 100 /10 + '0' );
					LCD_WriteData( highscore%1000000 % 100000 % 10000 % 1000 % 100 % 10 + '0' );
					
					if( highscore < 12000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'F' );
					}
					else if( highscore >= 12000 && highscore < 14000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'D' );
					}
					else if( highscore >= 14000  && highscore < 16000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'C' );
					}
					else if( highscore >= 16000 && highscore < 18000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'B' );
					}
					else if( highscore >= 18000 && highscore < 20000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'A' );
					}
					else if( highscore >= 20000 )
					{
						LCD_Cursor(17);
						LCD_WriteData( 'G' );
						LCD_WriteData( 'r' );
						LCD_WriteData( 'a' );
						LCD_WriteData( 'd' );
						LCD_WriteData( 'e' );
						LCD_WriteData( ':' );
						LCD_WriteData( ' ' );
						LCD_WriteData( 'S' );
					}
					scoredisplayFlag = 2;
				}
				else if( scoredisplayFlag == 2 )
				{
					LCD_DisplayString(1,"Play Again?");
					scoredisplayFlag = 3;
				}
				else
				{
					LCD_DisplayString(1, "Press 4 buttons to restart");
					scoredisplayFlag = 0;
				}
				
			}
			break;		
					
		case IncScore:
				if( trueStartFlag )
					score = multiplier*10 + score;
				switch( combo )
				{
					case 10:
						multiplier = 10;
						break;
					case 25:
						multiplier = 20;
						break;
					case 50:
						multiplier = 30;
						break;
					case 100:
						multiplier = 40;
						break;
				}
				
				//SM1_Tick(state);
				LCD_ClearScreen();
				LCD_Cursor(1);
			
				LCD_DisplayString( 1, "Score: ");
			
				//LCD_WriteData( score/1000000 + '0' );
				LCD_WriteData( score%1000000 / 100000 + '0' );
				LCD_WriteData( score%1000000 % 100000 /10000 + '0' );
				LCD_WriteData( score%1000000 % 100000 % 10000 /1000 + '0' );
				LCD_WriteData( score%1000000 % 100000 % 10000 % 1000 /100 + '0' );
				LCD_WriteData( score%1000000 % 100000 % 10000 % 1000 % 100 /10 + '0' );
				LCD_WriteData( score%1000000 % 100000 % 10000 % 1000 % 100 % 10 + '0' );
			
				LCD_Cursor( 17 );	//combo display
				LCD_WriteData( 'H' );
				LCD_WriteData( 'i' );
				LCD_WriteData( 't' );
				LCD_WriteData( 's' );
				LCD_WriteData( ':' );
				LCD_WriteData( ' ' );
			
			//	LCD_WriteData( combo/1000000 + '0' );
				//LCD_WriteData( combo%1000000 / 100000 + '0' );
				//LCD_WriteData( combo%1000000 % 100000 /10000 + '0' );
				//LCD_WriteData( combo%1000000 % 100000 % 10000 /1000 + '0' );
				LCD_WriteData( combo%1000000 % 100000 % 10000 % 1000 /100 + '0' );
				LCD_WriteData( combo%1000000 % 100000 % 10000 % 1000 % 100 /10 + '0' );
				LCD_WriteData( combo%1000000 % 100000 % 10000 % 1000 % 100 % 10 + '0' );
				
				LCD_WriteData( '/' );
				LCD_WriteData( '1');
				LCD_WriteData( '0');
				LCD_WriteData( '0');
				combo++;
				updateFlag = 0;
				trueStartFlag = 1;

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
			if( button0 && button1 && button2 && button3 )
				bState = resetB;
			else if( button0 && !button1 && !button2 && !button3 && !stopInc )
				bState = press0;
			else if( button1 && !button0 && !button2 && !button3 && !stopInc )
				bState = press1;
			else if( button2 && !button0 && !button1 && !button3 && !stopInc )
				bState = press2;
			else if( button3 && !button0 && !button1 && !button2 && !stopInc )
				bState = press3;
			else
				bState = wait;
			break;
		
		case resetB:
			bState = startB;
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
			if( !button0 && !button1 && !button2 && !button3 )
				stopInc = 0;
			break;
			
		case resetB:
			reset();
			stopInc = 1;
			break;
			
		case press0:
			if( ( ( PINA == 0x3F && PIND == 0x80 ) || !startFlag )  && !gameDone )	
			{
				updateFlag = 1;
				stopInc = 1;
			}
			break;
			
		case press1:
			if( ( ( PINA == 0xCF && PIND == 0x80 ) ||  !startFlag ) && !gameDone )
			{
				updateFlag = 1;
				stopInc = 1;
			}
			break;
			
		case press2:
			if( ( ( PINA == 0xF3 && PIND == 0x80 ) || !startFlag ) && !gameDone )
			{
				updateFlag = 1;
				stopInc = 1;
			}
			break;
			
		case press3:
			if( ( ( PINA == 0xFC && PIND == 0x80 ) || !startFlag ) && !gameDone )
			{
				updateFlag = 1;
				stopInc = 1;
			}
			break;
						
		default:
			break;
	}
};

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
	tasks[i].state = sm1_display;
	tasks[i].period = matrixPeriod;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM1_Tick;
	i++;
	tasks[i].state = startB;
	tasks[i].period = 1;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &buttonTickfct; 
	i++;
	tasks[i].state = startLCD;
	tasks[i].period = lcdPeriod;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &LCDtickfct;
	
	TimerSet(tasksPeriodGCD);
	TimerOn();
	
    while(1)
    {	
		if( updatePeriod )
		{
			tasks[0].period = matrixPeriod;
			updatePeriod = 0;
		}
		
		if( resetFlag )
		{
			column_val = 0x01;
			startFlag = 0;
			resetFlag = 0;
		}
		
			tasks[2].period = lcdPeriod;
			
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