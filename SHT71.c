#include <18f452.h>
#include <my_register.h>
#use delay(clock=20000000)
#use rs232(baud=9600, parity=N, xmit=PIN_C6, rcv=PIN_C7)
#include <stdlib.h>

#bit TRIS_data = TRISC.4
#bit TRIS_clock = TRISC.3
#bit data_bit = PORTC.4
#bit clock_bit = PORTC.3

#define READ_TEMP 0x03
#define READ_HUM 0x05


//Reset Vector Address
#build(reset=0x200)
//Intterupts Vector Address
#build(interrupt=0x208)

//Bootloader Area
#org 0x0000,0x01ff
void bootloader() 
{
#asm  
  nop //No Operation
#endasm
} 

void i2c_master_setting(long baud)
{
   TRIS_DATA = 1; //i2c setting(TRISC.4)
   TRIS_CLOCK = 1; //i2c setting(TRISC.3)
   
   SSPCON = 0x08;
   SSPCON2=0x00;
   SSPADD=(20000000/(4*baud))-1; //SSPADD=Fosc/(baud*4)-1
   SSPSTAT=0x00; //status register setting (Standard speed mode)

   SSPIE=1;
   SSPIF=0;
}

void Send_Start_Condition() //sensor start condition
{
   TRIS_data = 0; //TRISC.4 output
   TRIS_clock = 0; //TRISC.3 output
   
   data_bit = 1; //SDA=1
   
   clock_bit = 1; //SCL=0->1 (SDA=1)
   delay_us(1);
   
   data_bit = 0; //SDA=1->0 (SCL=1) **
   delay_us(1);
   
   clock_bit = 0; //SCL=1->0 (SDA=0)
   delay_us(1);
   
   clock_bit = 1; //SCL=0->1 (SDA=0)
   delay_us(1);
   
   data_bit = 1; //SDA=0->1 (SCL=1) **
   delay_us(1);
}

void master_start_condition() //i2c start condition
{
   TRIS_DATA = 1; //i2c setting(TRISC.4)
   TRIS_CLOCK = 1; //i2c setting(TRISC.3)
   SSPEN=1;
   
   SEN=1; //inite I2C START condition
   while(!SSPIF);
   SSPIF=0;
}

void send_address(unsigned char address)
{
   TRIS_DATA = 1; //i2c setting(TRISC.4)
   TRIS_CLOCK = 1; //i2c setting(TRISC.3)
   SSPEN = 1; //SCL on(i2c module automatically drive TRISC.3 and TRISC.4)
   
   SSPBUF=address; //write to SSPBUF occurs Start XMIT
   while(!SSPIF);
   SSPEN=0;
   if(ACKDT==0)
   {
      PORTD=0xff;
      //
   }
   else
   {
      PORTD=0xDD;
   }
   SSPIF = 0;
}

void I2CACK(void)
{
  ACKDT = 0; // 0 -> ACK
  ACKEN = 1; // Send ACK
}

void I2CNACK(void)
{
  ACKDT = 1; // 1 -> NACK
  ACKEN = 1; // Send NACK
}

void master_stop_condition()
{
   SSPEN=1;
   PEN=1;
   while(!SSPIF);
   SSPIF=0;
   PORTD=0xCC;
}

long delay;
long bufHigh=0x00, bufLow=0x00;
long buf=0;
long measure_XXX(unsigned char command)
{
   //send START condition
   send_start_condition(); //START
   //Master START condition
   master_start_condition();

   //send command to sensor
   send_address(command); //command = measure Temperature
   
   
   ////high byte////
   RCEN=1; //start receiver
   
   while(data_bit); //the controller must wait for "data ready" signal
   PORTD=0xee;
   
   SSPEN=1; //SCL on
   while(!BF); //last bit shifted into SSPBUF and contents are unloaded into SSPBUF
   while(!SSPIF);
   SSPEN=0;
   SSPIF=0;
   SSPEN=1;
   I2CACK(); //send ACK
   while(!SSPIF);
   SSPEN=0;
   PORTD=0xcc;
   SSPIF=0;
   bufHigh = SSPBUF; //read data high 8bit
   
   
   
   //SSPEN=0; //wait for read
   //
   /*
   RCEN=1; //Master configured as a receiver
   ////low byte////
   //
   SSPEN=1; //SCL on
   while(!BF);
   PORTD=0x01;
   SSPEN=0;
   SSPIF=0;
   SSPEN=1;
   I2CNACK(); //(send NACK)STOP   ** NACK = sensor STOP condition
   while(!SSPIF);
   SSPEN=0;
   SSPIF=0;
   while(!SSPIF);
   
   bufLow = SSPBUF; //read data low 8bit
   SSPEN=0; //wait for read
   //
   SSPIF=0;
   */
   printf("%lu\n\r",bufHigh);
   //printf("%lu\n\r",bufLow);
   
   
   PORTD=0xDD;

   master_stop_condition();
   
   ////high byte + low byte////
   buf=(int16)(bufHigh&0xff)*0x100+(bufLow&0xff);
   
   return buf;
}

void main()
{
   long Temp=0;
   float Temperature=0.0;

   //LED SETTING
   TRISD = 0X00;
   PORTD = 0X00;
   
   //I2C SETTING
   i2c_master_setting(400000);
   
   Temp = measure_XXX(READ_TEMP); //measure Temperature
   Temperature=((float)Temp*0.01)-40.00;
   printf("Temperature = %3.4f\n\r", Temperature); //show values
}
