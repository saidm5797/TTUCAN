// Control interface for car alarm and automatic headlights
#include <mcp_can.h>
#include <TTU_IsoTp.h>
#include <SPI.h>
#include <TTUCAN.h>

long unsigned int txId;
byte data[8];

//define variables
int alarm = 0, headlight_on = 0, auto_on = 0, brights_on = 0;
float volts = 0.0;

#define CAN0_INT 2                              // Set INT to pin 2
TTUCAN CAN0(10, CAN0_INT, 3, 0);                               // Set CS to pin 10

INT32U preProcess(byte *data, INT32U to_addr, INT32U descriptor); //create data array from variables

ISR(INT5_vect){
  Serial.println("Alarm on!");
  alarm = 1;
  txId = preProcess(data, 2, 0); //get ID for on_off status message
  CAN0.send_Msg(txId, 0, data, sizeof(data)); //send on_off status
}

void setup()
{
  Serial.begin(115200);
  DDRE &= 0xCF; //set PE4/PE5 as inputs
  CAN0.TTU_begin();

  cli(); //clear interupts
  EICRB = 0x0C; //trigger on rising edge at PE5
  sei(); //enable interrupts
  
  DDRA = 0xF0; //A0-A3 are inputs for switches
  PORTA = 0x0F; //pull up resistors on A0-A3
  
  // Set MUX 4-0 as 00001 for ADC1 (and ADC9)
  ADMUX = 0b01000001; //ADC1, 5V VCC as AREF
  // Enable ADC, 16 prescale
  ADCSRA = 0b10010100;
  ADCSRB = 0b00000000; // for ADC1
}

void loop()
{
  //Define Variables
  int armed = 0;
  int armed_mask = 0xFE;
  int headlight_mask = 0xFD;
  int auto_mask = 0xFB;
  int brights_mask = 0xF7;
  
  int value_ADC1 = 0;
    
  while(1){
    if((PINA | armed_mask) == armed_mask){    //adjust left mirror
      delay(250); //wait 250ms for debouncing
      Serial.print("armed ");
      armed ^= 0x01; //toggle armed status
      Serial.println(armed);
      if(armed){
        //enable external interrupt on PE5
        EIMSK = 0x20;
      }
      else{
        //disable external interrupt on PE5
        EIMSK = 0x00;
        alarm = 0;
        Serial.println("Alarm off.");
      }
    }    
    if((PINA | headlight_mask) == headlight_mask){   //adjust right mirror
      delay(250); //wait 250ms for debouncing
      Serial.print("headlights ");
      headlight_on ^= 0x01; //toggle headlights
      Serial.println(headlight_on);
      if(!headlight_on){
        auto_on = 0;
        brights_on = 0;
      }
    }    
    if((PINA | auto_mask) == auto_mask){   //adjust right mirror
      delay(250); //wait 250ms for debouncing
      Serial.print("auto ");
      if(headlight_on){
        auto_on ^= 0x01; //toggle auto mode  
        brights_on = 0;
      }
      Serial.println(auto_on);
    }
    if((PINA | brights_mask) == brights_mask){   //adjust right mirror
      delay(250); //wait 250ms for debouncing
      Serial.print("brights ");
      if(headlight_on){
        brights_on ^= 0x01; //toggle brights mode
        auto_on = 0;
      }
      Serial.println(brights_on);
    }
    if(auto_on){
      // read channel ADC1
      ADCSRA |= 0b01000000; // start ADC conversion
      while(! (ADCSRA & 0b00010000)); //wait until conversion ends
      value_ADC1 = ADC;
      volts = float (value_ADC1)*5.0/1023.0;
      Serial.print("voltage");Serial.print("\t");Serial.println(volts);
    }   
    delay(100);
    txId = preProcess(data, 2, 0); //get ID for on_off status message
    CAN0.send_Msg(txId, 0, data, sizeof(data)); //send on_off status
    delay(100);
    txId = preProcess(data, 2, 1); //get ID for motion control info message
    CAN0.send_Msg(txId, 0, data, sizeof(data)); //send motion control info
  }
}


INT32U preProcess(byte *data, INT32U to_addr, INT32U descriptor){
  //each case is for a different ID 
  //users should plan ahead and know the contents of each message
  //in order to process information correctly
  INT32U transmitID = CAN0.buildTransmitID(to_addr, descriptor);
  int _size;
  
  switch(transmitID){
    case 0x118: //sent from node 3 to node 2
      //reset data array to all zeros
      memset(data,0,sizeof(data)); 
      //begin filling data array 
      memcpy(data, (byte *)&alarm, sizeof(alarm));
      _size = sizeof(alarm);
      memcpy((data+_size), (byte *)&headlight_on, sizeof(headlight_on));
      break;
    case 0x119: //sent from node 3 to node 2
      //reset data array to all zeros
      memset(data,0,sizeof(data)); 
      //begin filling data array
      memcpy(data, (byte *)&volts, sizeof(volts));
      _size = sizeof(volts);
      memcpy((data+_size), (byte *)&auto_on, sizeof(auto_on));
      _size += sizeof(auto_on);
      memcpy((data+_size), (byte *)&brights_on, sizeof(brights_on));     
      break;
  }
  return transmitID;
}
