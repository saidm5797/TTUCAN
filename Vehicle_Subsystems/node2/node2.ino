// Actuators for car alarm and automatic headlights
#include <mcp_can.h>
#include <TTU_IsoTp.h>
#include <SPI.h>
#include <TTUCAN.h>

long unsigned int rxId, txId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];                        // Array to store serial string
byte data[8];

//define variables
int alarm, headlight_on, auto_on, brights_on;
int rti_count = 0, cycles = 0, Hzsignal = 1;
float volts;


#define CAN0_INT 2                              // Set INT to pin 2
TTUCAN CAN0(10, CAN0_INT, 2, 0);                               // Set CS to pin 10

void postProcess(byte *data, INT32U receiveID); //update variables from data array

ISR(TIMER1_COMPA_vect){
  OCR1A += 16384;
  rti_count++;
  if(Hzsignal == 1){
    PORTB ^= 0b00100000;   //toggle PB5
    if(rti_count == 2){
       cycles++;              //count how many cycles to control duration
       rti_count=0;
    }
    if(cycles == 250){        //500Hz signal for 0.5 sec
         cycles=0;
         Hzsignal=0; //change to 250Hz signal
         rti_count=0;
    }
  } 
  else{
     if(rti_count==2 || rti_count==4){
      PORTB ^= 0b00100000;   //toggle PB5
     }
     if(rti_count==4){
      cycles++;             //count how many cycles to control duration
      rti_count=0;
     }
     if(cycles == 125){        //250Hz signal for 0.5 sec
         cycles=0;
         Hzsignal=1; //change to 500Hz signal
         rti_count=0;
     }
  }
}

void setup()
{
  Serial.begin(115200);
  DDRE &= 0xEF; //set PE4 as input for CAN interrupt
  DDRA = 0x0F; //LEDs output
  PORTA = 0x00;
  cli(); //clear interrupts
  //use OC1A - PE3 as pwm pin output for buzzer
  DDRB |= 0x20; //PB5 as output for buzzer signal
  //output compare match interrupt on OC1A
  TCCR1A = 0b00000000;
  //prescale of 1;
  TCCR1B = 0b00000001;
  OCR1A = 16384; //interrupt once each millisecond
  sei(); //enable interrupts
  CAN0.TTU_begin();
}

void loop()
{
  
  if(!(PORTE | 0xEF))                         // If CAN0_INT pin is low, read receive buffer
  {
    CAN0.receive_Msg(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    
    if((rxId & 0x80000000) == 0x80000000)     // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
    else
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);
  
    Serial.print(msgString);
  
    if((rxId & 0x40000000) == 0x40000000){    // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    } else {
      for(byte i = 0; i<len; i++){
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        Serial.print(msgString);
      }
    }  
    Serial.println();
    postProcess(rxBuf, rxId); //post process data to use information received
  }
  if(alarm){
     TIMSK1 = 0x02; //enable output compare match interrupt
  }
  else{
      TIMSK1 = 0x00; //enable output compare match interrupt
  }
  if(headlight_on){
    if(auto_on){
      if(volts < 0.30){
        PORTA = 0x0A; //lights on
      }
      else{
        PORTA = 0x00; //lights off
      }
    }
    else if(brights_on){
      PORTA = 0x0F; //all lights on
    }
    else{
      PORTA = 0x0A; //lights on
    }
  }
  else{
    PORTA = 0x00; //lights off
  }
}


void postProcess(byte *data, INT32U receiveID){
  //each case is for a different ID 
  //users should plan ahead and know the contents of each message
  //in order to process information correctly
  int _size;
  switch(receiveID){
    case 0x118: //sent from node 3 to node 2
      memcpy(&alarm, data, sizeof(alarm));
      _size = sizeof(alarm); //starting index of data array
      memcpy(&headlight_on, (data +_size), sizeof(headlight_on));
      Serial.print("alarm ");
      Serial.println(alarm);
      Serial.print("headlight ");
      Serial.println(headlight_on);
      break;
    case 0x119: //sent from node 3 to node 2
      memcpy(&volts, data, sizeof(volts));
      _size = sizeof(volts); //starting index of data array
      memcpy(&auto_on, (data +_size), sizeof(auto_on));
      _size +=sizeof(auto_on); //increment index of array by size of count to access the next variable
      memcpy(&brights_on, (data +_size), sizeof(brights_on));
      break;
  }
}
