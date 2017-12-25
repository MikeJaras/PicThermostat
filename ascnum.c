/* ascnum.c convert between variables and ASCII-string */
/* Include processor header file in main program */
/* Include "math16.h" in main program */
/* Declare global string s[11] in main program */
/* Function prototypes is in ascnum.h */
/* FROM BINARY NUMBERS TO ASCII-STRINGS */
/* chartoa: convert "char" unsigned 8 bit variable c to ASCII-characters in global string s[] */
void chartoa(char c){
	char i,temp;
	s[3]='\0';
	for (i = 2; ;i--){
		temp = c % 10;
		temp += '0';
		s[i]=temp;
		if (i==0) break;
		c /= 10;
	}
}

/* inttoa: convert int variable i8 with sign to ASCII-characters in global string s[] */
void inttoa(int i8){
	char i,temp;
	s[4] = '\0';
	s[0] = '+'; if (i8.7 == 1) {i8 = - i8; s[0] = '-';}
	for (i = 3; ;i--){
		temp = i8 % 10;
		temp += '0';
		s[i]=temp;
		if (i==1) break;
		i8 /= 10;
	}
}

/* unslongtoa: convert unsigned long variable u16 to ASCII-characters in global string s[] */
void unslongtoa(unsigned long u16){
	char i,temp;
	s[5] = '\0';
	for (i = 4; ;i--){
		temp = u16 % 10;
		temp += '0';
		s[i]=temp;
		if (i==0) break;
		u16 /= 10;
	}
}

/* longtoa: convert long int variable i16 to ASCII-characters in global string s[] */
void longtoa(long i16){
	char i,temp;
	s[6] = '\0';
	s[0] = '+'; if (i16.15 == 1) {i16 = - i16; s[0] = '-';}
	for (i = 5; ;i--){
		temp = i16 % 10;
		temp += '0';
		s[i]=temp;
		if (i==1) break;
		i16 /= 10;
	}
}

						/* FROM ASCII-STRING TO BINARY NUMBER */
char atochar( void ){ 	/* converts ASCII-inputstring in s[] to char */
						/* Inputstring must have three digits 000 ... 255 */
	char number, digit;
	digit = s[2]; 		/* ones digit */
	digit = digit - '0';
	number = digit;

	digit = s[1]; 		/* tenths digit */
	digit = digit - '0';
	digit = digit*10;
	number = number + digit;

	if( s[0]=='1'){ 	/* hundreds digit */
		number = number + 100;
	}
	if( s[0]=='2' ){	/* hundreds digit */
		number = number + 200;
	}
	return number;
}

