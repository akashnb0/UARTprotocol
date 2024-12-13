#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <SoftwareSerial.h>

#define CPU_FREQ 16000000UL
#define BAUD 2400
#define UBRR_VALUE (CPU_FREQ/16/BAUD-1)
#define BUFFSPACE 1024
volatile char receiveBuffer[BUFFSPACE];
volatile uint16_t head=0;
volatile uint16_t tail=0;
volatile uint32_t transmitb=0;
volatile uint32_t receiveb=0;
char conv_buffer[16];
SoftwareSerial serialcnn(10,11); // Rx Tx pins virtual terminal 2 proteus

void initializeUART() 
{
    unsigned int ubrr=UBRR_VALUE;
    UBRR0H=(unsigned char)(ubrr >>8);
    UBRR0L=(unsigned char)ubrr;
    UCSR0B=(1<<RXEN0)| (1<<TXEN0) |(1<<RXCIE0);
    UCSR0C=(1<<UCSZ01) |(1<<UCSZ00);
    sei(); 
}

void sendNymbleByte(unsigned char data) 
{
    while(!(UCSR0A&(1<<UDRE0))) {}
    UDR0= data; 
    transmitb+=8; 
}

void sendNymbleString(const char* s) 
{
    while(*s) {
        sendNymbleByte(*s++);
    }
}

char* convertIntToString(int n,char* s,int base) 
{
    int ind =0;
    if(n==0) 
    {
        s[ind++]='0';
    } 
    else 
    {
        while(n) 
        {
            s[ind++]="0123456789ABCDEF"[n%base];
            n/=base;
        }
    }
    s[ind]='\0'; 
    for(int start=0,end=ind -1;start<end;start++,end--) 
    {
        char temp=s[start];
        s[start]=s[end];
        s[end]=temp;
    }
    return s;
}

ISR(USART_RX_vect) 
{
    char inp=UDR0;
    uint16_t nextHead=(head+1) % BUFFSPACE;
    if(nextHead!=tail) 
    { 
        receiveBuffer[head]=inp;
        head=nextHead;
        receiveb +=8;
    }
}

ISR(TIMER1_COMPA_vect)
{
    uint32_t t_speed=transmitb;
    uint32_t r_speed=receiveb;
    transmitb= 0; 
    receiveb=0;

    serialcnn.println("\r\nData Transmission Speed:");
    serialcnn.print("TX Speed: ");
    serialcnn.println(t_speed);
    serialcnn.print("RX Speed: ");
    serialcnn.println(r_speed);
}

void initializeTimer() 
{
    TCCR1B=(1<<WGM12) | (1<<CS12); 
    OCR1A= 62500; 
    TIMSK1=(1<<OCIE1A); 
}

int readCharFromUART(char* data) 
{
    if(head==tail) return 0;
    *data=receiveBuffer[tail];
    tail=(tail+1)%BUFFSPACE; 
    return 1; // reading data here
}

int main() 
{
    initializeUART();
    serialcnn.begin(2400);
    initializeTimer();
    sendNymbleString("MCU is ready to receive data...\r\n\r\n");
    char characterInput;
    uint16_t eepromi =0;

    while(1) 
    {
        while(readCharFromUART(&characterInput)) 
        {
            if(characterInput== '#') 
            {
                sendNymbleString("\r\nAll data received\r\n\r\n");
                sendNymbleString("Stored Successfully\r\n");

                sendNymbleString("\r\nMCU to PC\r\n\r\n");
                sendNymbleString("This is what is received:\r\n");

                for(uint16_t i=0;i<eepromi;i++) 
                {
                    sendNymbleByte(eeprom_read_byte((uint8_t*)i));
                }
                sendNymbleString("\r\n\r\n");
                eepromi=0; 
            } 
            else 
            {
                eeprom_write_byte((uint8_t*)eepromi,characterInput); // storing in rom
                sendNymbleString("Storing: ");
                sendNymbleByte(characterInput);
                sendNymbleString("\r\n");

                eepromi++; 
                if(eepromi>=BUFFSPACE) 
                {
                    sendNymbleString("\r\nEEPROM is full, stopping...\r\n");
                    break; 
                }
            }
        }
    }
    return 0; 
}