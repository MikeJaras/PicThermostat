///////////////////////////////////////////////////////////////////////
//                                           MikeJaras 2017-12-22    //
//		Originally J.Oinonen 2005-10-05								 //
//                                                                   //
// Thermostat using PIC 16f628 and DS18B20 thermometer				 //
// 									 12 bit resolution				 //
// Possible future improvements:                                     //
// *Retrieving FAN ON temperature from EEPROM so that                // 
//  changes are not lost in a power failure.  - Completed -          //
//                                                                   //
// *Add some logic to allow negative temperatures on max/min         //
//  Temperatures.                                                    //
//                                                                   //
// *Skip leading zeros on max/min menu.                              //
//                                                                   //
// *Add a routine for handling temperature variables with the        //
//  negative bit set in the LSB.                                     //
//                                                                   //
// *Improve accuracy on the FAN on temp and max min. Point above is  //
//  required for this. When the variables are stored currently       //
//  The 0.5 C bit is trashed.                                        //
//                                                                   //
// Compiles with B. Knudsen CC5X 3.6 free version					 //
///////////////////////////////////////////////////////////////////////
                                                                     
#include "16F628.h"
#include "int16Cxx.h"


#pragma config |= 0x2170   // use internal 4MHz oscillator (3ff0)
// 21f0 100001.1111.0000
// 2170	100001.0111.0000 bit7 LVP controls PORTB.4
#pragma bit MENU 	@ PORTB.1	// Menu-button
#pragma bit REDLED 	@ PORTB.4	// The REDLED symbolizes a fan
#pragma bit LED 	@ PORTB.2	// The REDLED symbolizes a fan
#pragma bit S1 		@ PORTB.6	// Increase highest temp allowed
#pragma bit S3 		@ PORTB.7	// decrease highest temp allowed


bit savetoeeprom;
char fantemp;			// Global variable to trigger temperature
// char lowtemp;			// Lowest, see above (not yet implemented)

 
//////////////////////////////////////////////////////////////////////
// This interrupt is used to read button presses on S1 and S3.      //
// Changes the value of highest temp allowed.                       //
// This routine needs to be here otherwise it wont compile.	    //
//////////////////////////////////////////////////////////////////////

#pragma origin 4

//Interrupt to read button S1 o S3 
interrupt int_server( void ){
	int_save_registers					// W, STATUS (and PCLATH)
	char i,j;
	if( RBIF == 1 )  
		{              
		if(S3)
			{	fantemp++;
			for(i =0; i < 50; i++ )    		// Debounce
			for(j =0; j < 250; j++ );
			}
	
		if(S1)
				{	fantemp--;
				for(i =0; i < 50; i++ )    		// Debounce
				for(j =0; j < 250; j++ );
			}
			RBIF = 0;  
		}
	savetoeeprom=1;	
	int_restore_registers					// W, STATUS (and PCLATH)
}

#include "ds1wire.c"			// All one wire routines are here
#include "delays.c"				// delay() ms

#pragma bit RS  @ PORTB.5 			//
#pragma bit EN  @ PORTB.3 			//
#pragma bit D7  @ PORTA.3 			// Pins used by LCD
#pragma bit D6  @ PORTA.2 			//
#pragma bit D5  @ PORTA.1 			//
#pragma bit D4  @ PORTA.0 			//

void delay( char ); // ms delay function from file delays.c
void lcd_init( void );
void lcd_putchar( char );
void chartoa( char );
void print_temp( void );
void readtemp( void );
void readromcode( void );
char text1( char );
char text2( char );
char text3( char );
char text4( char );
char hexchar( char );
void blink(uns8 blinkN);
void problem( void );
void cleanzeroes( void );
uns8 p;							//Variable used in blink
char getchar_eedata( char address );
void putchar_eedata( char data, char address );

char buffer[12];		// Used to store 12 bytes read from DS18B20
int temperature;		// Same as above, only need one of these global. Future improvement.
char s[11];				// S[11] is used by most of the string handling routines, see also ascnum.c
#include "math16.h"
#include "ascnum.c"		// String handling - chartoa inttoa unslongtoa longtoa atochar


 
void main( void){
	CM0=1; CM1=1; CM2=1;	// no comparators on
	int testing;
	int highest=22;				
	int lowest=22;
	char i,n,x;			// Some unused variables, needs cleaning up.
	char lsb,msb;
	char t1,t2;
	char romcode=0;
	// Use EEPROM
	fantemp=getchar_eedata(0);
	if (fantemp==255 || fantemp==0) {	// 0xFF is value when chip is programmed.
		fantemp=20;
		savetoeeprom=1;
	}
	
	OPTION.7 = 0;     // internal pullup resistors on 

  
		//  0=DQ pin DS18B20 in/out
		//  1=Menu(btn) in
		//  2=LED 		out 1wire-present used in ds1wire.c
		//  3=EN(lcd) 	out
		//  4=REDLED	out fan
		//  5=RS(lcd) 	out
		//  6=S1(btn) 	in
		//  7=S3(btn) 	in
		//             76 5 4 3 2 10
    TRISB  = 0b1100.0011; 	// 0-> output to RB2,RB3,RB4,RB5  -> RS and EN ( and Tx )
    TRISA  = 0b1111.0000;  	// 0-> output to RA0-RA3 		  -> D4-D7

	lcd_init();
	blink(5);  	//Flash LED on startup	
	Reset();	//Detect 1-wire sensor
	if (PORTB.2)
		problem();
		
	
	// trigger logic analyzer (Pulseview was used)
	delay(2);
	PORTB.2=1;		// To trigger analyzer
	delay(1);
	PORTB.2=0;		// Set low
	
	while(1){	// Read and present temp, wait for button

		if (savetoeeprom) {				// Store value in eeprom if new value is 
			putchar_eedata(fantemp,00);	// set in interrupt routine.
			savetoeeprom=0;				//
		}								//
		
		readtemp();						// Get temperature

		// buffer[1]=0b1111.1111;		// test negative value
		// buffer[0]=0b0101.0000;		// 1111.1111.0101.0000 -> -10
		// buffer[1]=0b0000.0000;		// test zero value
		// buffer[0]=0b0001.0000;		// 
		// buffer[1]=0b1111.1111;		// test -0.5 value
		// buffer[0]=0b1111.1000;		// 1111.1111.1111.1000 -> -0.5 
									// 1111.1111.1110.1000 -> -1.5


		if(MENU){						// If NOT menu button pressed; 	display main menu

			RS = 0;  					// LCD in command-mode
			lcd_putchar(0x02);			// Set Cursor position to the beginning
			RS=1;

			for(i=0; i<8; i++) 			// Display "Temp is:"
			   lcd_putchar(text1(i));

			print_temp();				// Display the temperature
			lcd_putchar(0b11011111);	// Centigrade character
			lcd_putchar('C');			// 'C'
			lcd_putchar(' ');
			RS = 0;  					// LCD in command-mode
			lcd_putchar(0xC0);			// Reposition to second line on LCD
			RS=1;
			for(i=0; i<8; i++) 
				lcd_putchar(text2(i)); 	// Display "Fan ON: "

			chartoa(fantemp);			// Convert fantemp to ascii
			lcd_putchar('+');			// Plus sign. 
			// cleanzeroes();
			for(i=1; i<3; i++){			// Display temperature
				lcd_putchar(s[i]);  
			}
			
			lcd_putchar('.');
			lcd_putchar('0');
			lcd_putchar(0b11011111);	// Centigrade character
			lcd_putchar('C');
			lcd_putchar(' ');			// Overwrite text in case menu 2 left any garbage
		}
		else{							// If Menu pressed, display max/min temperatures
			RS = 0;						// LCD in command-mode
			lcd_putchar(0x02);			// Reposition to beginning of first line
			lcd_putchar(0x01);			// Clear screen
			RS=1;

			for(i=0; i<8; i++) 
			   lcd_putchar(text3(i));	// Display "Highest:"

			inttoa(highest);
			// lcd_putchar('+');	
			cleanzeroes();
			for(i=1; i<4; i++)
			   lcd_putchar(s[i]);  		

			lcd_putchar('.');
			lcd_putchar('0');
			lcd_putchar(0b11011111);	// Centigrade character
			lcd_putchar('C');

			RS = 0;  					// LCD in command-mode
			lcd_putchar(0xC0);			// Reposition to beginning of second line
			RS=1;

			for(i=0; i<8; i++) 
			  lcd_putchar(text4(i));	// Display "Lowest: "

			inttoa(lowest);				// Convert int to ascii
			// lcd_putchar('+');	
			cleanzeroes();
			for(i=1; i<4; i++)			
			   lcd_putchar(s[i]);  

			lcd_putchar('.');
			lcd_putchar('0');
			lcd_putchar(0b11011111);	//Centigrade character
			lcd_putchar('C');
			
			// print rom code
			if(romcode) {					// 1->show rom code
				delay(250);			// 
				delay(250);     	//
				delay(250);			//
				delay(250);			//
				
				readromcode();
					
				RS = 0;  					// LCD in command-mode
				lcd_putchar(0x02);			// Reposition to beginning of first line
				lcd_putchar(0x01);			// Clear screen
				RS=1;

				for(x=0; x<8; x++){			// Display 6 byte rom code"
					lsb=buffer[x]&0x0F;
					msb=buffer[x]>>4;
					t1=hexchar(lsb);
					t2=hexchar(msb);
					lcd_putchar(t2);
					lcd_putchar(t1);
				}
				
				RS = 0;  					// LCD in command-mode
				lcd_putchar(0xC0);			// Reposition to beginning of second line
				RS=1;
				
				int testing=-005;
				
				inttoa(testing);			//' '=0x20 '-'=0x2D  '0'=0x30 '+'=0x2B
				cleanzeroes();
				
				for(i=0; i<4; i++) 
						lcd_putchar(s[i]);	//
				  // lcd_putchar(' ');		//

			}
			delay(250);			//
			delay(250);     	//
			delay(250);			//
				
		}

		if(temperature>highest)		
			highest=temperature;		// If new highest replace old temperature
		if(temperature<lowest)
			lowest=temperature;
		

		if(temperature>=fantemp)		// If temperature over highest permitted start fan
			REDLED=1;					// REDLED symbolizes a Fan
		else{
			REDLED=0;
		}
		
		RBIE     = 1;   		// Turn on interrupts.
		GIE      = 1;   				
		
		RBIE     = 0;   		// Turn off interrupts
		GIE      = 0;   				
	}
}

 

//////////////////////////////////////////////////////////////////////
// This function reads a temperature from the DS18B20.             	//
// 12 bytes are read but only the 2 first are used.                	//
// The result is placed in buffer[].                               	//
//////////////////////////////////////////////////////////////////////
void readtemp() {
	char i,n;
    Reset();			// Reset is required before any command is issued
	Write(0xCC);  		// Skip Rom
	Write(0x44);		// Tell DS18B20 to Start Temp. Conv

	RBIE     = 1;   	// Turn on interrupts while waiting
	GIE      = 1;   			
	delay(250);			//
	delay(250);     	// 750ms delay required
	delay(250);			// for temperature conversion

	RBIE     = 0;   	/* local disable  */	// Turn of interrupts before speaking to 1 wire device
	GIE      = 0;   	/* global disable */
	Reset();
	Write(0xCC);		// Skip Rom
	Write(0xBE);		// Read the result of the temperature conversion.

	for(i=0; i<12 ;i++){	// Read 12 bytes, only the first two bytes are needed.
		n=Read();			// n = the read byte, one of 12
		buffer[i]=n;
	}
}

void print_temp() {
	char lsb;
	char msb;

	RS = 1;  // LCD in character-mode
	char i;
	
	// Test for negative value
	if(buffer[1].3==1) {					// The bit determines the sign of the temperature
		lsb=buffer[0]>>4;
		msb=buffer[1]<<4;
		temperature=msb+lsb;
		// temperature=temperature>>1;		// Divide the result by 2
		temperature +=1;
		inttoa(temperature);	
		// s[0]='-';
		
		cleanzeroes();
		// display the 8 char text1() sentence
		// lcd_putchar('-');				// Display - sign
		for(i=1; i<4; i++) 
			lcd_putchar(s[i]);
	}
	else { 	// If positive value.
		lsb=buffer[0]>>4;
		msb=buffer[1]<<4;
		temperature=msb+lsb;
		// lcd_putchar('+');				// Display + sign
		inttoa(temperature);			// Convert char to ascii
		cleanzeroes();
		for(i=1; i<4; i++) 		
			lcd_putchar(s[i]);  

		//Display the temperature on the LCD
	}

	
// debug
// putchar_eedata(s[0],01);
// putchar_eedata(s[1],02);
// putchar_eedata(s[2],03);
// putchar_eedata(s[3],04);
// putchar_eedata(s[4],05);
	
	// Test for 0.5 value. 
	if(buffer[0].3==1) {					// 0b0000.1000 = 0.5 C
			lcd_putchar('.');
			lcd_putchar('5');
		}
	else {									// 0b0000.0000 = 0.0 C
			lcd_putchar('.');
			lcd_putchar('0');
		}
}
 
//////////////////////////////////////////////////////////////////////
// This function reads the unique code from the DS18B20.          	//
// The result is placed in buffer[].                               	//
//////////////////////////////////////////////////////////////////////
void readromcode() {
	char i,n;
    Reset();			// Reset is required before any command is issued
	Write(0x33);		// Tell DS18B20 to send its 64-bit ROM code

	for(i=0; i<8 ;i++){	// Read 8 bytes, family code, serial nr, crc 
		n=Read();			// n = the read byte, one of 8
		buffer[i]=n;
	}
}

/* ----------------------------------- */
/* this is the way to store a sentence */
/* ----------------------------------- */

// - temp is
char text1( char x) {
   skip(x); /* internal function CC5x.  */
   #pragma return[] = "Temp is:"    // 8 chars max!
}

// - fan on
char text2( char x) {
   skip(x); /* internal function CC5x.  */
   #pragma return[] = "Fan ON: "    // 8 chars max!
}

// - highest
char text3( char x) {
   skip(x); /* internal function CC5x.  */
   #pragma return[] = "Highest:"    // 8 chars max!
}

// - lowest
char text4( char x) {
   skip(x); /* internal function CC5x.  */
   #pragma return[] = "Lowest: "    // 8 chars max!
}


// must be run once before using the display
void lcd_init( void ) {
  delay(40);  // give LCD time to settle
  RS = 0;     // LCD in command-mode
  lcd_putchar(0b0011.0011); /* LCD starts in 8 bit mode          */
  lcd_putchar(0b0011.0010); /* change to 4 bit mode              */
  lcd_putchar(0b00101000);  /* two line (8+8 chars in the row)   */ 
  lcd_putchar(0b00001100);  /* display on, cursor off, blink off */
  lcd_putchar(0b00000001);  /* display clear                     */
  lcd_putchar(0b00000110);  /* increment mode, shift off         */
  RS = 1;    // LCD in character-mode
             // initialization is done!
}

 
void lcd_putchar( char data ) {
  // must set LCD-mode before calling this function!
  // RS = 1 LCD in character-mode
  // RS = 0 LCD in command-mode
  // upper Nybble
  D7 = data.7;
  D6 = data.6;
  D5 = data.5;
  D4 = data.4;
  EN = 0;
  nop();
  EN = 1;
  delay(5);
  // lower Nybble
  D7 = data.3;
  D6 = data.2;
  D5 = data.1;
  D4 = data.0;
  EN = 0;
  nop();
  EN = 1;
  delay(5);
}


void putchar_eedata( char data, char address ) {
      /* Put char in specific EEPROM-address */
      /* Write EEPROM-data sequence                           */
      EEADR = address;    /* EEPROM-data address 0x00 => 0x40 */
      // EEPGD = 0;          /* Data, not Program memory  (16690 only )  */  
      EEDATA = data;      /* data to be written               */
      WREN = 1;           /* write enable                     */
      EECON2 = 0x55;      /*  first Byte in command sequence  */
      EECON2 = 0xAA;      /* second Byte in command sequence  */
      WR = 1;             /* write                            */
      while( EEIF == 0) ; /* wait for done (EEIF=1)           */
      WR = 0;
      WREN = 0;           /* write disable - safety first    */
      EEIF = 0;           /* Reset EEIF bit in software      */
      /* End of write EEPROM-data sequence                   */
}


char getchar_eedata( char address ) {
      /* Get char from specific EEPROM-address */
      /* Start of read EEPROM-data sequence                */
      char temp;
      EEADR = address;  /* EEPROM-data address 0x00 => 0x40  */ 
      // EEPGD = 0;       /* Data not Program -memory         */      
      RD = 1;          /* Read                             */
      temp = EEDATA;
      RD = 0;
      return temp;     /* data to be read                  */
      /* End of read EEPROM-data sequence                  */  
}

char hexchar(char hex){
	char result;

	switch(hex){
		case 0:
			result='0';
			break;
		case 1:
			result='1';
			break;
		case 2:
			result='2';
			break;
		case 3:
			result='3';
			break;
		case 4:
			result='4';
			break;
		case 5:
			result='5';
			break;
		case 6:
			result='6';
			break;
		case 7:
			result='7';
			break;
		case 8:
			result='8';
			break;
		case 9:
			result='9';
			break;
		case 10:
			result='A';
			break;
		case 11:
			result='B';
			break;
		case 12:
			result='C';
			break;
		case 13:
			result='D';
			break;
		case 14:
			result='E';
			break;
		case 15:
			result='F';
			break;
		default:
			result='*';
	}

return result;
}

void cleanzeroes( void ){
	if(s[0]=='-' && s[1]=='0' && s[2]!='0'){				// Catch -0XX
		s[0]=' '; s[1]='-';	}	  							// ->     -XX    
	if(s[0]=='-' && s[1]=='0' && s[2]=='0'){				// Catch -00X
		s[0]=' '; s[1]=' '; s[2]='-'; }						// ->      -X
	if(s[0]=='+' && s[1]=='0' && s[2]=='0' && s[3]=='0'){	// Catch +000
		s[0]=' '; s[1]=' '; s[2]='-'; }						// ->     +00
	if(s[0]==' ' && s[1]=='+' && s[2]=='0' && s[3]=='0'){	// Catch _+00
		s[0]=' '; s[1]='+'; }								// ->      +0
	if(s[0]=='+' && s[1]=='0' && s[2]=='0' && s[3]!='0'){	// Catch +00X
		s[0]=' '; s[1]='+'; }								// ->     +0X
	if(s[0]=='+' && s[1]=='0' && s[2]!='0' && s[3]!='0'){	// Catch +0XX
		s[0]=' '; s[1]='+'; }								// ->     +XX
	if(s[0]=='+' && s[1]=='0' && s[2]!='0' && s[3]=='0'){	// Catch +0X0
		s[0]=' '; s[1]='+'; }								// ->     +XX
}	

//Flash LED n times 	
void blink(uns8 blinkN)	{
		for (p=0; p<blinkN; p++){
				LED=0;
				delay(250);
				LED=1;
				delay(250);
		}
}		

//Flash LED on problem 	
void problem( void ) {
		while(1){
				LED=0;
				delay(50);
				LED=1;
				delay(50);
		}	
}

/* *********************************** */
/*            HARDWARE                 */
/* *********************************** */
/*
---    PK2(1) Vpp 	 4
---    PK2(2) +5v	14
---    PK2(3)  0v	 5
---    PK2(4) Data	13
---    PK2(5) Clock

            ___________  ___________
           |           \/           |
     D6 -<-|RA2      16F628      RA1|->- D5
     D7 -<-|RA3                  RA0|->- D4
           |RA4                  RA7|
  PK2(1)---|RA5/Vpp              RA6|
  PK2(3)---|Vss                  Vdd|---+5v PK2(2)
DS18B20(2)-|RB0              RB7/PGD|---    PK2(4)
    SW1 ->-|RB1              RB6/PGC|---    PK2(5)
    LED -<-|RB2                  RB5|->- RS
     EN -<-|RB3                  RB4|->- LED
           |________________________| 
   

    ---- DS18B20 -----
	1 GND			-> GND
	2 DQ			-> RB0 - Pin 6
	  Pullup resistor 4.7K (internal pullup is used, OPTION.7)
	3 Vdd/+5v   	-> +5v

	---- LED ----
	RB2 led			-> Anod -Pin 8
	
	---- Buttons ----
	SW1     RB7			Pin 13 	Decrease
	SW2     RB1			Pin 7 	MENU
	SW3     RB6 		Pin	12	Increase
	
	RS  	@ PORTB.5 	Pin11	//
	EN  	@ PORTB.3 	Pin 9	//
	D7  	@ PORTA.3 	Pin 2	// Pins used by LCD
	D6  	@ PORTA.2 	Pin 1	//
	D5  	@ PORTA.1 	Pin18	//
	D4  	@ PORTA.0 	Pin17	//
	
	MENU 	@ PORTB.1
	REDLED 	@ PORTB.4 			// The REDLED symbolizes a fan
	S1 		@ PORTB.6			// Increase highest temp allowed
	S3 		@ PORTB.7			// Decrease highest temp allowed
	
    LCD two lines, Line length 16 characters
    Internal ic: HD44780A00		   
           _______________
          |               |
          |         Vss  1|--- GND  
          |         Vdd  2|--- +5V
          |    Contrast  3|-<- Pot
          |          RS  4|-<- RB4
          |      RD/!WR  5|--- 0, GND
          |          EN  6|-<- RB6
          |          D0  7|
          |          D1  8|
          |          D2  9|
          |          D3 10|
          |          D4 11|-<- RC0
          |          D5 12|-<- RC1 
          |          D6 13|-<- RC2
          |          D7 14|-<- RC3 
          |          D7 15|-<- Backlight + 
          |          D7 16|-<- Backlight - 
          |_______________|						  
*/