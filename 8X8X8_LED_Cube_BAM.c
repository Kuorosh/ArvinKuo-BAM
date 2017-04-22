/*Note
 * 	 //a Way to measure the Interrupt_Period
		//clock_t start;		
		//double Interrupt_Period;
		//if (level == 1) start = clock();*/
		
				//if (level == 2) {
		
			//Interrupt_Period = (double) (clock() - start) * 1000 / CLOCKS_PER_SEC; // in mSec
			//printf ("Interrupt Period in mSec= %f\n " , Interrupt_Period);
		//}
	
//By default GPIO are all configured as inputs except GPIO 14 & 15. 
//The state of GPIO are whether Pull up or pull down. all the GPIO used here in Program are 
//Pull down (low)except GPIO4 which is pull up	

		
#include "ALT_2.h"
#include <stdio.h>		// for printf
#include <string.h>
#include <errno.h>
#include <sys/time.h>	// required for function "gettimeofday()"
#include <inttypes.h>	//For example, you will get the "PRIu16" macro so that you can printf an uint16_t integer like this:
#include <unistd.h>		//type definitions like size_t (which are often declared in other files, but unistd.h gets them for you)
#include <stdbool.h> 	//for using bool type identifier
#include <stdlib.h>    //for exit 
#include <getopt.h>
#include <time.h>      
#include <wiringPiSPI.h>                                                                                                    
#define delayMicroseconds wiringPi_delayMicroseconds
#include <wiringPi.h>		//used for GOIO drive
#undef delayMicroseconds
#include <bcm2835.h> 	// is used for SPI


#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define ST_BASE (0x20003000) //ST_BASE system Timer
#define TIMER_OFFSET (4)     // is the low level free counter Reg

	//DMA13A (the signal connected to the Black Board through buffer SN74HC541N in order to increase Signal level to 5V 
#define Clck 11			
#define Latch 2			//GPIO2	    
#define Enable 3		 		 	 
#define Mosi 10 		//MOSI red
		
#define Anode_Level0 4		//GPIO4	p7	
#define Anode_Level1 25
#define Anode_Level2 24
#define Anode_Level3 23
#define Anode_Level4 22
#define Anode_Level5 27
#define Anode_Level6 18
#define Anode_Level7 17

#define RXD 15		// GIPO15 RXD is defined as input and connected to the Counter O/P and is used as interrupt 	
#define Reset 14	// Stop the program ( through press switch). connected directly to RasPi from Black Board 
#define pulse(pin) do { \
	GPIO_SET = 1<<pin;\
	nanosleep(&p, NULL);\
	GPIO_CLR = 1<<pin;\
} while (0) 

#define Npulse(pin) do { \
	GPIO_CLR = 1<<4;\
	nanosleep(&p, NULL);\
	GPIO_SET = 1<<pin;\
} while (0) 
// int nanosleep(const struct timespec *req, struct timespec *rem);

#define DMA13A_N 12 	
#define numChannels 16 * DMA13A_N 		
#define NUM_LEVELS_PER_CUBE  8
#define Interrupt_Period 1025/1000	//im mSec ?????

FILE *myFilePt;		
struct timespec p;		
uint16_t FrameTime , FrameNoOfLoop=1;		// It is assumed that the FrameTime given in text file is in msec.

uint16_t GS_Value[NUM_LEVELS_PER_CUBE][numChannels]; // for every channel 2 bytes data reserved

uint8_t gsData[NUM_LEVELS_PER_CUBE][numChannels  *3/2]; //numChannels * 12/8. every channel has 12 bits data
#define Rain_Time 200000

int shift_out;      
uint8_t anode[8];  
uint8_t  red0[64], red1[64], red2[64], red3[64];	/*each color of LED is defined as 4 bits value (0 to 15)  
Note 64 Bytes gives you 512 bits which define the bit value of all 512 LEDs Also the first bit of red0,red1, 
* red2 and red3 define the red color of the first LED and the 2nd. Bit of each array (red0,red1, red2 and red3)
*  define the red colour value of the 2.LED*/
uint8_t  green0[64], green1[64], green2[64], green3[64];
uint8_t  blue0[64], blue1[64], blue2[64], blue3[64];

int anodelevel=0;  
int level=0;   

int BAM_Bit, BAM_Counter=0;    
unsigned long start;    		

void LED(uint8_t level, uint8_t row, uint8_t column, uint8_t red, uint8_t  green, uint8_t  blue);

int setup(void) {  
	
	time_t seconds; //Declare variable to hold seconds on clock.//
	time(&seconds);	//Get value from system clock and place in seconds variable.
	srand((unsigned int) seconds);	// Convert seconds to a unsigned integer.

	if (wiringPiSetupGpio()< 0) {		// so we can use GPIO No in wiringPi functions
      fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
      return 1;
	}//End of if

	if (!bcm2835_init()){
      printf("bcm2835_init failed. Are you running as root??\n");
      return 1;
    }
	
	setup_io(); //  // Set up gpi pointer for direct register access
  
	p.tv_sec = 0 ;
	p.tv_nsec = 30;

		//DMA13A		 
		
	INP_GPIO(Latch); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(Latch);
	
	INP_GPIO(Enable);  
	OUT_GPIO(Enable);
	GPIO_SET = 1<Enable; //disable the O/P
	
	INP_GPIO(Clck);  
	OUT_GPIO(Clck);
		
	INP_GPIO(Mosi);  
	OUT_GPIO(Mosi);
	
	INP_GPIO(Anode_Level0); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(Anode_Level0);
	pullUpDnControl (Anode_Level0, PUD_DOWN ) ;
	GPIO_SET = 1<<Anode_Level0;	//Since we are using PNP transistor Level0 must be ON to switch OFF the PNP Transistor
		
	INP_GPIO(Anode_Level1);  
	OUT_GPIO(Anode_Level1);
	GPIO_SET = 1<<Anode_Level1; //Level1 OFF
	
	INP_GPIO(Anode_Level2);  
	OUT_GPIO(Anode_Level2);
	GPIO_SET = 1<<Anode_Level2; //Level2 OFF
	
	INP_GPIO(Anode_Level3);  
	OUT_GPIO(Anode_Level3);
	GPIO_SET = 1<<Anode_Level3; //Level3 OFF
	
	INP_GPIO(Anode_Level4);  
	OUT_GPIO(Anode_Level4);
	GPIO_SET = 1<<Anode_Level4; //Level4 OFF
	
	INP_GPIO(Anode_Level5);  
	OUT_GPIO(Anode_Level5);
	GPIO_SET = 1<<Anode_Level5; 
	
	INP_GPIO(Anode_Level6);  
	OUT_GPIO(Anode_Level6);
	GPIO_SET = 1<<Anode_Level6;
	
	INP_GPIO(Anode_Level7);  
	OUT_GPIO(Anode_Level7);
	GPIO_SET = 1<<Anode_Level7;
	
	INP_GPIO(Reset); // Stop the program
	pullUpDnControl (Reset, PUD_UP ) ;
		
				//SPI	
	    
	if (!bcm2835_spi_begin()) {
		printf("bcm2835_spi_begin failed. Are you running as root??\n");
		return 1;
    }	//Forces RPi SPI0 pins P1-19 (MOSI), P1-21 (MISO), P1-23 (CLK), P1-24 (CE0) and P1-26 (CE1)
     
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); //All data are clocked in with the MSB first 
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0); // Mode 0 by Rising edge of Clock the data is read , clock low in ideal . OK for SR.Mode 0, 1, 2, 3
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32); // 64 = 256ns = 3.90625MHz  //  32--> 7.8MHZ
	//CLOCK width  should be > 16ns e.g f < 1000/16 <62.5MHZ	Max. frequency for CLOck is 30MHZ
	bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);
	//bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
    //bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default  
    
	return 0;
}//End of Set Up

void LED(uint8_t level, uint8_t row, uint8_t column, uint8_t red, uint8_t  green, uint8_t  blue){
/* By having the position (level, row, column of an LED we set its bit value) red0[], red1[], red2[], red3[],
 *  green0[]..   according to its given location and the value of red, green and blue */
	if(level<0)
		level=0;
	if(level>7)
		level=7;
	if(row<0)
		row=0;
	if(row>7)
		row=7;
	if(column<0)
		column=0;
	if(column>7)
		column=7;  
	if(red<0)
		red=0;
	if(red>15)
		red=15;
	if(green<0)
		green=0;
	if(green>15)
		green=15;
	if(blue<0)
		blue=0;
	if(blue>15)
		blue=15;  
	//Each LED is identified by one bit of 512 bits( the wholle Cube) and its color is defined with red0[],red[]1,.. 
	//considering the whole cube which byte is the LED				 
	int whichbyte = (((level*64)+(row*8)+column)/8);// which byte the LED belongs to (the whole cube represented with 64 Byte
	uint8_t whichbit=(level*64)+(row*8)+column;	//considering the whole cube which bite is the LED (cube has 
			// cube LEDs occupy 512 bits. originally was wholebyte 
	  
	void bitWrite(int x, int n, uint8_t v, uint8_t b) {	//KUO	 x is which byte, n is the which bit
		// v is the value of color and b is the bit (0 to 3)
		if (n <= 7 && n >= 0) {	// check the bit value		 
			if ((v >> b) & 0x01){
				x |= (1u << n); /*You can set the fourth bit of a number by OR-ing it with a value that
				 is zero everywhere except in the fourth bit. This could be done as x |= (1u << 3); */
			}
			else {
				x &= ~(1u << n);
				/*   similarly, you can clear the fourth bit by AND-ing it with a value that
				 *  is one everywhere except in the fourth bit. For example: x &= ~(1u << 3);
				 *    */
			}
		}
	}	//End of bitWrite 
   
		void write_64bit_zero_to(void *address)  //Not use
		{
		*((uint64_t *) address) = 0;
		}

	bitWrite(red0[whichbyte], whichbit-(8*whichbyte), red, 0);// we write the value of bit 0 
	bitWrite(red1[whichbyte], whichbit-(8*whichbyte), red, 1);
	bitWrite(red2[whichbyte], whichbit-(8*whichbyte), red, 2);
	bitWrite(red3[whichbyte], whichbit-(8*whichbyte), red, 3);

	bitWrite(green0[whichbyte], whichbit-(8*whichbyte), green, 0);
	bitWrite(green1[whichbyte], whichbit-(8*whichbyte), green, 1);
	bitWrite(green2[whichbyte], whichbit-(8*whichbyte), green, 2);
	bitWrite(green3[whichbyte], whichbit-(8*whichbyte), green, 3);
	
	bitWrite(blue0[whichbyte], whichbit-(8*whichbyte), blue, 0);
	bitWrite(blue1[whichbyte], whichbit-(8*whichbyte), blue, 1);
	bitWrite(blue2[whichbyte], whichbit-(8*whichbyte), blue, 2);
	bitWrite(blue3[whichbyte], whichbit-(8*whichbyte), blue, 3);
  
}	//End of LED 

void rainVersionTwo(){   			//Animation rainVersionTwo
//printf("Rain");	
int x[64], y[64], z[64], addr, leds=64,ledcolor; 
	int xold[64], yold[64], zold[64];
   	for(addr=0; addr<64; addr++){
		x[addr]=rand() %8;
		y[addr]=rand() %8;
		z[addr]=rand() %8;     
	}

	clock_t start;	
    start = clock(); 
    
	while(clock()-start<Rain_Time){ // Rain_Time is achieved by trial

		if(ledcolor<200){
			for(addr=0; addr<leds; addr++){
				LED(zold[addr], xold[addr], yold[addr], 0, 0, 0);
				if(z[addr]>=7)
					LED(z[addr], x[addr], y[addr], 0, 5, 15);
				if(z[addr]==6)
					LED(z[addr], x[addr], y[addr], 0, 1, 9);
				if(z[addr]==5)
					LED(z[addr], x[addr], y[addr], 0, 0, 10);
				if(z[addr]==4)
					LED(z[addr], x[addr], y[addr], 1, 0, 11); 
				if(z[addr]==3)
					LED(z[addr], x[addr], y[addr], 3, 0, 12);
				if(z[addr]==2)
					LED(z[addr], x[addr], y[addr], 10, 0, 15);
				if(z[addr]==1)
					LED(z[addr], x[addr], y[addr], 10, 0, 10);
				if(z[addr]<=0)           
					LED(z[addr], x[addr], y[addr], 10, 0, 1);
			}
		}				//200

		if(ledcolor>=200&&ledcolor<300){
			for(addr=0; addr<leds; addr++){
				LED(zold[addr], xold[addr], yold[addr], 0, 0, 0);
				if(z[addr]>=7)
					LED(z[addr], x[addr], y[addr], 15, 15, 0);
				if(z[addr]==6)
					LED(z[addr], x[addr], y[addr], 10, 10, 0);
				if(z[addr]==5)
					LED(z[addr], x[addr], y[addr], 15, 5, 0);
				if(z[addr]==4)
					LED(z[addr], x[addr], y[addr], 15, 2, 0); 
				if(z[addr]==3)
					LED(z[addr], x[addr], y[addr], 15, 1, 0);
				if(z[addr]==2)
					LED(z[addr], x[addr], y[addr], 15, 0, 0);
				if(z[addr]==1)
					LED(z[addr], x[addr], y[addr], 12, 0, 0);
				if(z[addr]<=0)
					LED(z[addr], x[addr], y[addr], 10, 0, 0);
			}
		}				//300  
		ledcolor++;
		if(ledcolor>=300)
			ledcolor=0;
	  
		for(addr=0; addr<leds; addr++){
			xold[addr]=x[addr];
			yold[addr]=y[addr];
			zold[addr]=z[addr];
		} 
		delay(15);
		for(addr=0; addr<leds; addr++){

		z[addr] = z[addr]-1;
	  
		if(z[addr]<0) {// %(-100,0)){

			x[addr]=rand() %8;
			y[addr]=rand() %8;

			z[addr]=7; 
   
		}		//-check
	}		//add
  }		//while
 
}		//rainv2

void color_wheel(){		// Animation color  wheel

	int xx, yy, zz,ranx, rany;
  
     start=millis();
      
	while(millis()-start<100000){
		//swiper=rand() %3;;
		ranx=rand() %16;
		rany=rand() %16;
		 
		for(xx=0;xx<8;xx++){
			for(yy=0;yy<8;yy++){ 
				for(zz=0;zz<8;zz++){
		  
					LED(xx, yy, zz,  ranx, 0, rany);
				}
			}
			delay(50);
		}

		 ranx=rand() %16;
		 rany=rand() %16;
		 
		for(xx=7;xx>=0;xx--){ 
			for(yy=0;yy<8;yy++){
				for(zz=0;zz<8;zz++){
					LED(xx,yy, zz, ranx, rany, 0);
				}
			}
			delay(50); 
		}
		ranx=rand() %16;
		rany=rand() %16;
		for(xx=0;xx<8;xx++){ 
			for(yy=0;yy<8;yy++){
				for(zz=0;zz<8;zz++){
					LED(xx,yy, zz, 0, ranx, rany);
				}
			}
			delay(50);
		}
		
		ranx=rand() %16;
		rany=rand() %16;
		for(xx=7;xx>=0;xx--){ 
			for(yy=0;yy<8;yy++){
				for(zz=0;zz<8;zz++){
					LED(xx,yy, zz, rany, ranx, 0);
				}
			}
			delay(50); 
		}
		
	 }		//while
		
}		//color wheel
void loop(){	
	//rainVersionTwo();	

LED(0, 0, 1, 15, 0, 0);

//red0[0] = red1[0] = red2[0] = red3[0] = 8;
 //printf(" colorr value =: %d\n", red1[0]);
//folder();
//sinwaveTwo();
//wipe_out();
//bouncyvTwo();
//color_wheel();
//color_wheelTWO();

} 

void DMA13A(void){ 
	
//printf("DMA1");
		
	//int i = 0;
	//for(i = 0; i < 64; i++) {
		//red0[i] = red1[i] = red2[i] = red3[i] = 15;
		//green0[i] = green1[i] = green2[i] = green3[i] = 0;
		//blue0[i] = blue1[i] = blue2[i] = blue3[i] = 0;
	//}

	//LED(2, 1, 4, 15, 0, 0);
	GPIO_SET = 1<<Enable; //disable the O/P. The first thing we do is turn all of the LEDs OFF	

	//if(BAM_Counter==8)	// so we can go through all 8 levels and Bit0 is on for 8 Interrupt.
		//BAM_Bit++;
	//else
	//if(BAM_Counter==16)	//
		//BAM_Bit++;
	//else
	//if(BAM_Counter==32)
		//BAM_Bit++;
		
	if(BAM_Counter==8)	//KUO// so we can go through all 8 levels and Bit0 is on for 8 Interrupt.
		BAM_Bit++;
	else
	if(BAM_Counter==24)	//
		BAM_Bit++;
	else
	if(BAM_Counter==56)	//
		BAM_Bit++;
			
		/*Note BAM_Bit =0 during BAM_Counter 0 to 7, BAM_Bit =1 during BAM_Counter 8 to 15, BAM_Bit =2 during BAM_Counter
	  16 to 31 and BAM_Bit =3 during BAM_Counter 33 to 63 			*/
	BAM_Counter++;	//Here is where we increment the BAM counter By every interrupt
	
			    //to check the spi ( connecting MISO to MOSI)
    
    //uint8_t send_data = 0x0ff;
    //uint8_t read_data = bcm2835_spi_transfer(send_data);
    //printf("Sent to SPI: 0x%03X. Read back from SPI: 0x%03X.\n", send_data, read_data);
    //if (send_data != read_data)
    //printf("Do you have the loopback from MOSI to MISO connected?\n");
	
		loop();	
	
	switch (BAM_Bit) {		//Note: By each interrupt the level is increased  by 8 and we send 8 Byte of Data	
							//Therefore by 2.nd interrupt we send the data of 2. Anodelevel 
		// Byte[0] to byte[7] --> level0, Byte[8] to byte[15] --> level1, .. Byte[56] to byte[63] --> level7, 
		case 0:  // correspond to the bit 0 of BAM_BIT 
	 
			for(shift_out=level; shift_out<level+8; shift_out++)
					bcm2835_spi_transfer( red0[shift_out]); 
			for(shift_out=level; shift_out<level+8; shift_out++)
					bcm2835_spi_transfer( green0[shift_out]); 
			for(shift_out=level; shift_out<level+8; shift_out++)
					bcm2835_spi_transfer( blue0[shift_out]); 
			//printf("case 0");
			break;

		case 1:  // correspond to the bit 1 of BAM	
			for(shift_out=level; shift_out<level+8; shift_out++)
				bcm2835_spi_transfer( red1[shift_out]); 		
			for(shift_out=level; shift_out<level+8; shift_out++)
				bcm2835_spi_transfer( green1[shift_out]); 
			for(shift_out=level; shift_out<level+8; shift_out++)
				bcm2835_spi_transfer( blue1[shift_out]); 
				//printf("case 1");
			break;

		case 2:		 // correspond to the bit 2 of BAM
			for(shift_out=level; shift_out<level+8; shift_out++)
				bcm2835_spi_transfer( red2[shift_out]); 		
			for(shift_out=level; shift_out<level+8; shift_out++)
				bcm2835_spi_transfer( green2[shift_out]); 
			for(shift_out=level; shift_out<level+8; shift_out++)
				bcm2835_spi_transfer( blue2[shift_out]); 
				//printf("case 2");
			break;

		case 3:	 // correspond to the bit 3 of BAM	
			for(shift_out=level; shift_out<level+8; shift_out++)
				bcm2835_spi_transfer( red3[shift_out]); 		
			for(shift_out=level; shift_out<level+8; shift_out++)
				bcm2835_spi_transfer( green3[shift_out]); 
			for(shift_out=level; shift_out<level+8; shift_out++)
				bcm2835_spi_transfer( blue3[shift_out]); 
				//printf("case 3");	
			//if(BAM_Counter==64){	//so we reset the BAM_Counter and BAM_Bit when BAM_Counter=64 and in case 3
			if(BAM_Counter==120){	// KUO//so we reset the BAM_Counter and BAM_Bit when BAM_Counter=64 and in case 3	
				BAM_Counter=0;
				BAM_Bit=0;		
			}
			break;

	}	// End of Switch Bit
 //printf(" colorr value =: %d\n", red1[0]);
	 //printf("Anodelevel 0 =: %d\n", Anode_Level0);
	GPIO_SET = 1<<Anode_Level0;
	GPIO_SET = 1<<Anode_Level1;
	GPIO_SET = 1<<Anode_Level2;		
	GPIO_SET = 1<<Anode_Level3; 
	GPIO_SET = 1<<Anode_Level4;
	GPIO_SET = 1<<Anode_Level5;
	GPIO_SET = 1<<Anode_Level6;		
	GPIO_SET = 1<<Anode_Level7; 		
	//anodelevel =0;
;
	switch (anodelevel){ 	//setting the Anode level

		case 0:  
		GPIO_CLR = 1<<Anode_Level0;
			//printf("case0=----");
			break;
				
		case 1:  
			GPIO_CLR = 1<<Anode_Level1;

			break;
		case 2:  
			GPIO_CLR = 1<<Anode_Level2;
			break;			  

		case 3:  
			GPIO_CLR = 1<<Anode_Level3;
			break;		 
			 
		case 4:  
			GPIO_CLR = 1<<Anode_Level4;
			break;	
					 
		case 5:  
			GPIO_CLR = 1<<Anode_Level5;
			break;		 

		case 6:  
			GPIO_CLR = 1<<Anode_Level6;
			break;		 
			 
		case 7:  
			GPIO_CLR = 1<<Anode_Level7;
			break;		 
	}	//End of switch	 
		
	pulse(Latch);
	GPIO_CLR = 1<<Enable; //enable pin LOW to turn on the LEDs with the new data 
	
	anodelevel++;		//inrement the anode level
		
	if(anodelevel==8)		//go back to 0 if max is reached
		anodelevel=0;	 
		 
	level = level+8;	//increment the level variable by 8, which is used to shift out out data, since the next
						// level would be the next 8 bytes in the arrays
	//if(level==64)	//if you hit 64 on level, this means you just sent out all 63 bytes, so go back
		if(level==120)	//KUO if you hit 64 on level, this means you just sent out all 63 bytes, so go back
		level=0;

	//if (anodelevel >= 7){ 	
		//if (FrameNoOfLoop >= (FrameTime /( 8 * Interrupt_Period))) {		
			//FrameNoOfLoop = 1;			
		//}
		//else {
			//FrameNoOfLoop += 1;	
		//}		
	//}
		 
	//if (FrameNoOfLoop == 1){	// at the beginnig of the program anmd when the frame time is finished FrameNoOfLoop =1 and level =0	
			
		//if (anodelevel == 0){ 

			//fscanf(myFilePt,"%hu,", &FrameTime); // read the frame time	
		//}
		
			
		////int n; 	
		////for(n = 0; n < 192; n++) {   
			////fscanf(myFilePt, "%hu,", &GS_Value[level][n]); // the 1. 64 elements are red then green and last 64 elements are for blue LEDs
		////}        
	//}	//if frameNoofLoop
	   	
}//End DMA13A

int main(int argc, char **argv ) {  /* The name of the variable argc stands for "argument count"; argc contains the number
	 of arguments passed to the program. The name of the variable argv standsfor "argument vector". and argv is a one-dimensional
	 array of strings. Each string is one of the arguments that was passed to the program.
	  we pass the argument --f "RGB_LED_Cube.txt"  to program to run*/

	myFilePt = stdin;			// FILE *myFilePt also myFilePt is a filepointer
	int c;
	while (1)		//each time we read an option
		{
		static struct option long_options[] =  /* This structure (struct option) describes a single long option name
		 for the sake of getopt_long. The argument longopts must be an array of these structures, one for each long option.
		 Terminate the array with an element containing all zeros*/
			{
	/* each of These array element has four element: (const char *name),(int has_arg), (int *flag) and (int val)
	we have two array element here*/
          
			{"file",    required_argument, 0, 'f'}, 	// In 1. array element the value of zero for the flag means 
														//that the getopt_long return the val Value
														
			{0, 0, 0, 0}								//2. all zero means the end of array
        }; 
        
      /* getopt_long stores the option index here.  int getopt(int argc, char * const argv[],
           const char *optstring); */
      int option_index = 0;

	  //"" because we expect only the long option 
      c = getopt_long (argc, argv, "", long_options, &option_index);
      
      /* Detect the end of the options. e.g no more option  */
      if (c == -1)
        break;	//out of while loop

      switch (c)		// If find an option 
        {
        case 'f':
			printf ("option -f with value `%s'\n", optarg);
			
			myFilePt = fopen(optarg , "r");   //
  
			if (myFilePt == NULL) // check if the file is ok to open
			{
				printf("Error Reading File\n");
       
			}
			
			break;

        case '?':
			/* getopt_long already printed an error message. */
			break;

        default: // none of the above cases
          abort ();
        }
    } //End of the 1. while (1)

  /* Print any remaining command line arguments (not options). */

	if (optind < argc)
    {
		printf ("non-option ARGV-elements: ");
		while (optind < argc)
        printf ("%s ", argv[optind++]);
		putchar ('\n');
    }   

   	setup();
 
	if ( wiringPiISR (RXD, INT_EDGE_RISING, &DMA13A) < 0 ) {
          fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
          return 1;
	}
	
 	while (bcm2835_gpio_lev (Reset) )
 			
 	GPIO_SET = 1<<Anode_Level0; //Level0 OFF
	GPIO_SET = 1<<Anode_Level1; 
	GPIO_SET = 1<<Anode_Level2; 
	GPIO_SET = 1<<Anode_Level3;  
	GPIO_SET = 1<<Anode_Level4; 
	GPIO_SET = 1<<Anode_Level5; 
	GPIO_SET = 1<<Anode_Level6; 
	GPIO_SET = 1<<Anode_Level7; 
	
	GPIO_SET = 1<<Enable; 
	bcm2835_spi_end();		// the SPI pins will all revert to inputs and can then be configured and controled with the usual bcm2835_gpio_* calls
	bcm2835_close();
	fclose(myFilePt);

	return 0;
	
}// end of Main
