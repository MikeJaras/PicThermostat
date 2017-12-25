//////////////////////////////////////////////////////////////////////
//                                           J.Oinonen 2005-09-29   //
//                                                                  //
//                                                                  //
// Dallas 1 wire routines interfacing with the DS1820 temperature   //
// sensor.                                                          //
//                                                                  //
// Possible future improvements:                                    //
// -One universal delay routine. (to get rid of all do while        // 
//  statements)                                                     //
// -Support for multiple sensors (currently only one allowed)       //
// -Support for sensor addressing                                   //
//                                                                  //
// Compiles with B. Knudsen CC5X(Trial version/without optimization)//
// Timings are critical!                                            //
//////////////////////////////////////////////////////////////////////


#define DQ_TRI TRISB.0		//Tristate register of  DQ pin
#define DQ_PIN  PORTB.0		//Pin connected to DQ on DS1820

//////////////////////////////////////////////////////////////////////
// Main delay routine. Delays the uprocessor for 5 us.              //
//    2   +   1  +    2   = 5                                       //
//   GOTO +  NOP + RETURN = 5us                                     //
//////////////////////////////////////////////////////////////////////

void Delay5us(){
	nop();
}

/*void delay( char millisec)
{
do{
	char t=200;
	do
	{
		Delay5us();
	}while(--t);
}
while(--millisec);

}
*/

//////////////////////////////////////////////////////////////////////
// This function Resets the 1 wire bus and checks for presence of a //
// 1 Wire device. If no device is detected, Led on Portb.2 is lit   //
//////////////////////////////////////////////////////////////////////

void Reset(void){
	char count;
	bit  present;
	DQ_TRI=1;			// Set the Correct port pin settings
	DQ_PIN=0;

	DQ_TRI=0;	   		// Pull Line Low to start reset pulse

//	count=40; //original
	count=50; 
	do
		Delay5us();		// reset should be like 480µS Delay (tRSTL)
	while (-- count > 0);

	DQ_TRI=1;      		// Release the line

	count=3;  //count=12
	do		    		//  = 60us delay to middle of presence pulse (tPDIH)
		Delay5us();
	while (-- count > 0);

	present=DQ_PIN;  	// Get Presence on pin, DS1820 pulls it low if it's there

	count=45; //count=34
	do
		Delay5us();		// delay should be like 480µS to finish reset (tRSTH)
	while (-- count > 0);

	if(present==1){
	 PORTB.2=1;		// If no device, Indicate error on LED
	}
	else{ //do flash
	 PORTB.2=1;		// Set high
	 Delay5us();
	 PORTB.2=0;		// Set low
	}


	Delay5us();
	Delay5us();			// Rise + Min Space = 10 us

}

//////////////////////////////////////////////////////////////////////
// This function Writes a byte to the 1 wire bus.                   //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void Write(char Data){
	char count=8,n;
	bit  DQdata;
	

	 

	for(;count>0;count--){
		
		DQdata= Data.0;	// Put the bit to be written in the right place
		DQ_PIN=0;
		
		DQ_TRI=0;		// Lower the port
		nop();			// Time slot start time 5us according
		DQ_TRI=1;

		
		if(DQdata== 1)	// Output data to the port
			DQ_TRI = 1;
		else
			DQ_TRI = 0;

						// All write time-slots must be a minimum of 60μs in duration with a 
						// minimum of a 1μs recovery time between individual write slots.  
		n=6; //10
		do
			Delay5us();		//50us to finish timeslot
		while (-- n > 0);
		
		DQ_TRI=1;        	// Make sure port is released
		
		nop();		  		// Some Delay Between Bits. Minimum 1µs
		nop();
		
		Data=Data>>1;		// Shift next bit into place
							// Future improvement using rr(Data)
	}

	// Delay5us();				// Rise + Min Space = 5 us
	
	 PORTB.2=1;		// Set high
	 Delay5us();
	 PORTB.2=0;		// Set low
	 Delay5us();
	 PORTB.2=1;		// Set high
	 Delay5us();
	 PORTB.2=0;		// Set low
}

 

//////////////////////////////////////////////////////////////////////
// This function Reads a byte from the 1 Wire bus and returns the   //
// byte read.                                                       //
//////////////////////////////////////////////////////////////////////

char Read(void)
	{
	// do flash
	PORTB.2=1;		// Set high
	Delay5us();
	PORTB.2=0;		// Set low
	Delay5us();
	PORTB.2=1;		// Set high
	Delay5us();
	PORTB.2=0;		// Set low
	Delay5us();
	PORTB.2=1;		// Set high
	Delay5us();
	PORTB.2=0;		// Set low

	char count=8,data=0,n;
	bit  DQdata;
	for(;count>0;count--)
	{

		DQ_PIN=0;
		DQ_TRI=0;           // Lower port
		Delay5us();			// Timeslot start time
							// Output from the  DS18B20  is  valid  for  15μs  after  the  falling  edge  
							// that initiated  the  read  time  slot.  
		DQ_TRI=1;			// Set port for reading
		// Delay5us();		// Wait until center of timeslot
		PORTB.2=1;			// Set high for debug
		PORTB.2=0;			// Set low

		DQdata=DQ_PIN;		// Read the bit on the bus

		n=4;
		do
			Delay5us();		//55us to finish timeslot
		while (-- n > 0);

		/* ------------------------------------ */
		/* Lets put the bit stream into a byte	*/
		/* First DQdata is bit0					*/
		data = data >> 1;  		// Shift previous bits (future improvement rr..)
		if(DQdata==1)         	// Add 1 if the data on the bus was 1
			data +=0x80;		// Originally 0x80=0b1000.0000
		 
		//Delay5us();  			//-- Delay Between Bits
	}

	Delay5us();				// Rise + Min Space = 5 us
	return(data);
}