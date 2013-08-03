/*
 * sdaAccel.c
 *
 * Created: 7/25/2013 11:52:31 PM
 *  Author: andre_000
 
 This sample program will read the x-axis reading from a 12-bit precision accelerometer and transmit it to the serial line. Using putty one can test the measurements.
 If you haven't looked at the serGPS code yet, I recommend you do because I will not explain the serial communication part of this code. It is basically copied from there.
 */ 


#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h> //SDA is also known as Two Wire Interface, TWI.
#include <stdio.h>	//Needed for Printf function.
#include "accel.h"	//Contains definitions specific to the accel. Don't check this file yet before I tell you below.

void ioinit(void);
char USARTReadChar(void);
void USARTWriteChar(char data, FILE *steam);
void USARTInit(unsigned int ubrr_value);
void TWI_init_master(void);	//Initializes the I2C as a master device
void TWI_start(void);	//Sends a start condition. Check Sparkfun's explanation on the timing diagram to better understand the protocol: https://learn.sparkfun.com/tutorials/i2c/protocol.
void TWI_write(char data);	//Requests to write to the slave device.
char TWI_read_data(void);		//Requests to read from the slave device.
void TWI_stop(void);			//Sends a stop condition, ending the transmission.

static FILE mystdout = FDEV_SETUP_STREAM(USARTWriteChar, NULL, _FDEV_SETUP_WRITE);


int main(void)
{
	char xh=0; //Variable that will hold the first 8 MSB (Most significant bit) of the x-axis accelerometer reading.
	char xl=0;	//Variable that will hold the low bits for the x-axis.
	unsigned int x=0;	//Integer that will hold 16bit value of the x-axis reading. If we don't make it unsigned, the MSB will hold information on the sign of the integer. Since our integer is positive, we don't need that.
	

	ioinit();
	USARTInit(10);
	TWI_init_master();	//We first initialize the TWI as a master device. This automatically turns SDL, the clock pin, into an output port and starts sending clock signal out.
	
	/*Now we need to enable the Accel and this is done by writing to its control register #1. Take a look a the datasheet on page 24. That control register has 8 bits:
	ODR3 ODR2 ODR1 ODR0 LPen Zen Yen Xen
	By default the device start with the ODR0-ODR3 as 0 and that means the device is off. We need to write anything but 0s in there, so let's write 1 to ODR0. We also want to enable the x y and z axis.
	Lastly, we don't want to enable LPen, low power mode. In the end, we want to write the following on that address: 0b00010111.
	
	Now, I suggest you read page 19-20 of the datasheet to understand how we can write to the registers of the accel. I will summarize it here.
	First we need to send a start condition (which is basically pulling SDA low, but check the function below main to see how we do that by writing to the TWI registers).
	*/	
	
	TWI_start();
	TWI_write(0b00110010);	//Then we need to talk specifically to our accelerometer device. We do that by writing it's 7-bit address on the SDA line, which is 0011001. Now if we want to write to it we add a 0 at the end or a 1 to read, making it an 8-bit number.
							//In this case we want to write to it. Check table 11 on page 19 of the datasheet.
	TWI_write(0b0100000);	//Then we send the address of the register we want to write to. In our case that is 0b0100000, control register #1.
	TWI_write(0b01010111);	//Finally we write what we want to that address, which we discussed above to turn on the device, namely 0b01010111.
	TWI_stop();				//To finish transmission we send a stop condition. (This is important if there are other devices on the SDA/SCL bus line because no slave device is allowed to do anything while the master is talking to a device. So they wait for the stop condition. We could also not send a stop condition and just do another start later on, called a repeated start where we never gave up control of the SDA line. That makes the slave devices unable to request anything... mean huh? :)
	
	/*So that was a successful transmission. I must admit this is not trivial so please ask questions if it's hard to grasp this protocol, and again take a look at at 11 on page 19 because things will make more sense.
	At every step of the way, the accel. was actually acknowledging each of our transmissions of data, as seen on table 11. We could have probed the SDA line to check if transmission was actually successful after TWI_start
	and TWI_write, so that we would retransmit if the slave device hadn't acknowledged any of them. I didn't do this here to keep it simple but it could easily be added.
	My last comment on this transmission protocol, I promise. You can see it is very messy. No I'm not talking about all the green comments surrounding the poor code. I mean the data we send TWI_write is very hard for someone
	not familiar with the device to understand. I left it that way here so you could see the numbers... but a better approach would be to include a header file with all of these numbers written already to make the code more readable. I will do that with the next transmissions.
	Go check the accel.h header file now.
	*/
	
    while(1)
    {
      TWI_start();
	  TWI_write(SDA_ACCEL_WRITE);
	  TWI_write(X_AXIS_OUT_LOW);
	  TWI_start();					//This is an example of a repeated start since we never send a stop condition.
	  TWI_write(SDA_ACCEL_READ);	//Notice how the transmission goes. We gave the accel the address for which we wanted access to but since we want to read it we need to start again and send a read request.
	  xl = TWI_read_data();			//The accel will send us the value of the register so we write it to our 8-bit variable x low.
	  TWI_stop();
									//Much cleaner with the #defines in place huh?
	  TWI_start();
	  TWI_write(SDA_ACCEL_WRITE);
	  TWI_write(X_AXIS_OUT_HIGH);
	  TWI_start();
	  TWI_write(SDA_ACCEL_READ);
	  xh = TWI_read_data();
	  TWI_stop();
	  
	  x = xh<<8 | xl;				//Here we move xh 8 bits to the left, for example if xh was 10101010 we would now have 1010101000000000, which has added space for xl, so we just add that. If xl was 11111111 for example, x would then equal 101010111111111 where we successfully pieced together a 16bit number by joining it's two 8bit counterparts.
	  
	 
	  printf("%u !", x);			//Finally we print the data on the serial line and then repeat the measurement.
	  	  
	  
	  _delay_ms(500);

    }
}

void ioinit(void){
	//Not needed. I could have removed it but in case we expand the code we already have this in here.
}

void TWI_init_master(void) // Function to initialize master
{
	TWBR=0x5c;    // Set bit rate
	TWSR=(0<<TWPS1)|(0<<TWPS0);    // Set prescalar bits
	// SCL freq= F_CPU/(16+2(TWBR).(TWPS)) This gives us a frequency of 20MHz/200=100KHz which is within the capabilities of the Accel.
}

void TWI_start(void)
{
	// Clear TWI interrupt flag, Put start condition on SDA, Enable TWI
	TWCR= (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while((TWCR & (1<<TWINT)) == 0); // Wait till start condition is transmitted. This is important or else later we could end up trying to transmit again while this transmission hadn't ended... a collision of data would happen.
}

void TWI_write(char data)
{
	TWDR = data;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while ((TWCR & (1<<TWINT)) == 0);
}


char TWI_read_data(void)
{
	    TWCR = (1<<TWINT)|(1<<TWEN);
	    while ((TWCR & (1<<TWINT)) == 0);
	    return TWDR;
}

void TWI_stop(void)
{
	// Clear TWI interrupt flag, Put stop condition on SDA, Enable TWI
	TWCR= (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	while(!(TWCR & (1<<TWSTO)));  // Wait till stop condition is transmitted
}

void USARTInit(uint16_t ubrr_value){

	//Set Baud rate.
	   
	UBRR0L = ubrr_value;
	UBRR0H = (ubrr_value>>8);

	/*Set Frame Format.

	>> Asynchronous mode
	>> No Parity
	>> 1 StopBit

	>> char size 8
	*/
   
	UCSR0C= (1<<UCSZ00)|(1<<UCSZ01); //8-bit frame


	UCSR0B=(1<<RXEN0)|(1<<TXEN0); //Enable The receiver and transmitter

	stdout = &mystdout; //Sets the stream we created at the top of the code to be the standard stream out. So by default printf function will send chars to this string.

}

char USARTReadChar()
{
   //Wait until data is available

   while(!(UCSR0A & (1<<RXC0)))
   {
      //Do nothing
   }

   //Now USART has got data from device and is available inside the buffer.

   return UDR0;
}

void USARTWriteChar(char data, FILE *stream)
{
   //Wait until the transmitter is ready. (This is important if, for example, the USART was sending data out from the buffer... we can't just write the buffer in the middle of a transmission.)

   while(!(UCSR0A & (1<<UDRE0)))
   {
      //Do nothing
   }

   //Now write the data to USART buffer

   UDR0=data;
}