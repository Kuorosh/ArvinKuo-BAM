
#include <stdio.h>		// for printf
#include <inttypes.h>	

uint8_t  red0[64], red1[64], red2[64], red3[64];	
uint8_t  green0[64], green1[64], green2[64], green3[64];
uint8_t  blue0[64], blue1[64], blue2[64], blue3[64];

void LED( uint8_t level, uint8_t row, uint8_t column, uint8_t red, uint8_t green, uint8_t blue){
	int whichbyte = (((level*64)+(row*8)+column)/8);// which byte the LED belongs to (the whole cube represented with 64 Byte
	int whichbit=(level*64)+(row*8)+column;	//considering the whole cube which bite is the LED 	  
	void bitWrite(int x, int n, uint8_t    v, uint8_t    b) {	
		if (n <= 7 && n >= 0) {	
			if ((v >> b) & 0x01){
				x |= (1u << n); 
				//printf("one\n");
				//printf(" x =: %d\n", x);
			
			}
			else {
				//printf("Zero\n");
				x &= ~(1u << n);
				//printf(" x =: %d\n", x);
			}
		}
	}	//End of bitWrite 
   
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
	printf(" whichbyte =: %d\n", whichbyte);
    printf(" whichbit =: %d\n", whichbit-(8*whichbyte));

}	//End of LED 

int main()
	{
int i = 0;
	for(i = 0; i < 64; i++) {
		red0[i] = red1[i] = red2[i] = red3[i] = 0;
		green0[i] = green1[i] = green2[i] = green3[i] = 0;
		blue0[i] = blue1[i] = blue2[i] = blue3[i] = 0;
	}

	LED(0, 1, 1, 1, 15, 15);
	printf(" color red0[1] value =: %d\n", red0[1]);
	printf(" color red1 value =: %d\n", red1[1]);
	printf(" color red2 value =: %d\n", red2[1]);
	printf(" color red3 value =: %d\n", red3[1]);
	//printf(" color green value =: %d\n", green0[9]);
	//printf(" color blue value =: %d\n", blue0[9]);
	
	return 0;

	}
		
