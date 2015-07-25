//MicroEYE - Code for the microcontroller
#include <string.h>
#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "therm_ds18b20.h"

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define F_CPU  8000000

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

//This function is used to initialize the USART
//at a given UBRR value
void USARTInit(uint16_t ubrr_value)
{
   //Set Baud rate
   UBRRL = ubrr_value;
   UBRRH = (ubrr_value>>8);

   /*Set Frame Format
   >> Asynchronous mode
   >> No Parity
   >> 1 StopBit
   >> char size 8
   */

   UCSRC=(1<<URSEL)|(3<<UCSZ0);

   //Enable The receiver and transmitter
   UCSRB=(1<<RXEN)|(1<<TXEN);
   
}


//This function is used to read the available data
//from USART. This function will wait untill data is
//available.
char USARTReadChar(void)
{
   //Wait untill a data is available
   while(!(UCSRA & (1<<RXC)))
   {
      //Do nothing
   }

   //Now USART has got data from host
   //and is available is buffer

   return UDR;
}


//This fuction writes the given "data" to
//the USART which then transmit it via TX line
void USARTWriteChar(char data)
{
   //Wait untill the transmitter is ready
   while(!(UCSRA & (1<<UDRE)))
   {
      //Do nothing
   }

   //Now write the data to USART buffer

   UDR=data;
}


int main(void)
{
	char data;

	USARTInit(MYUBRR);
	
	TCCR1A  |= (1<<COM1A1) | (1<<WGM11);
	TCCR1B |=  (1<<WGM13) | (1<<WGM12) | (1<<CS11);
	
	ICR1 = 20000;
	
	DDRB |= (1<<PB1);
	
	set_output(DDRB, PB0); 

	OCR1A=1400;

   while(1)
   {
		char x[20] = {0};
		therm_read_temperature(x);
		
		for(int i=0; i<5; i++){
		   USARTWriteChar(x[i]);
		   for (int i=0; i<100; i++) _delay_ms(10);
		}
	   
		float num = (x[1]-48)*10+(x[2]-48)+ (x[4]-48)*0.1;
		
		if(num>=27.0)
			output_low(PORTB, PB0); 
			
		if(num<27.0)
			output_high(PORTB, PB0);
		
		data=USARTReadChar(); 
		
		if (data==0x31)
		{
			//todo (move servo to 2000)
			for(int j=0; j<10;j++){
				OCR1A +=50;
				for(int i=0;i<300;i++)
				_delay_ms(10);
			}
			
			USARTWriteChar('S');
			
		}
		
		if (data==0x30)
		{
			
			
		}    
		
		if(OCR1A>1400)
		{
			while(OCR1A>1400){
				OCR1A -=50;
				for(int i=0;i<300;i++)
				_delay_ms(10);
			}
			OCR1A=1400;
			for(int i=0;i<1000;i++)
			_delay_ms(10);
		}
		
   }
   
   return 0;
}