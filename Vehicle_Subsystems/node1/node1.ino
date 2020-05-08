// Control interface for mirror position control 
#include <mcp_can.h>
#include <TTU_IsoTp.h>
#include <SPI.h>
#include <TTUCAN.h>

long unsigned int txId;
byte data[8];

//define variables
int on_off = 0, left = 0, right = 0;
int move_x = 0, move_y = 0;

#define CAN0_INT 2                              // Set INT to pin 2
TTUCAN CAN0(10, CAN0_INT, 1, 0);                               // Set CS to pin 10

INT32U preProcess(byte *data, INT32U to_addr, INT32U descriptor); //create data array from variables

void setup()
{
  Serial.begin(115200);
  DDRE &= 0xEF; //set PE4 as input for CAN interrupt
  
  DDRA = 0xF8; //A0/A1 are inputs for switches
  PORTA = 0x07; //pull up resistors on A0/A1
  
  // Set MUX 4-0 as 00001 for ADC1 (and ADC9)
  ADMUX = 0b01000001; //ADC1, 5V VCC as AREF
  // Enable ADC, 16 prescale
  ADCSRA = 0b10010100;
  ADCSRB = 0b00000000; // for ADC1
  
  CAN0.TTU_begin();
}

void loop()
{
  //Define Variables
  int left_mask = 0xFE;
  int right_mask = 0xFD;
  int onOff_mask = 0xFB;
  int value_ADC1 = 0, value_ADC9 = 0;
  int prev_x=0, prev_y=0;
  float volts_x =0.0, volts_y = 0.0;
    
  while(1){
    if((PINA | left_mask) == left_mask){    //adjust left mirror
      delay(250); //wait 250ms for debouncing
      Serial.print("left ");
      if(on_off){  //only update if the device is on
        left = 1;
        right = 0;
      }
      Serial.println(left);
    }    
    if((PINA | right_mask) == right_mask){   //adjust right mirror
      delay(250); //wait 250ms for debouncing
      Serial.print("right ");
      if(on_off){  //only update if the device is on
        left = 0;
        right = 1;
      }
      Serial.println(right);
    }    
    if((PINA | onOff_mask) == onOff_mask){   //power on and off
      delay(250); //wait 250ms for debouncing
      Serial.print("Power ");
      on_off ^= 0x01; //toggle power
      Serial.println(on_off);
      if(!on_off){
        left = 0;
        right = 0;
      }
    }
    if(on_off){
      // read channel ADC1
      ADCSRB = 0b00000000; // set channel to ADC1
      ADCSRA |= 0b01000000; // start ADC conversion
      while(! (ADCSRA & 0b00010000)); //wait until conversion ends
      value_ADC1 = ADC;
      volts_x = float (value_ADC1)*5.0/1023.0;
      Serial.print("X axis");Serial.print("\t");Serial.println(volts_x);
          
      // read channel ADC9
      ADCSRB = 0b00001000; // set channel to ADC8
      ADCSRA |= 0b01000000; // starting Adc Conversions
      while(! (ADCSRA & 0b00010000)); //wait until conversion ends
      value_ADC9 = ADC;
      volts_y = float (value_ADC9)*5.0/1023.0;
      Serial.print("y axis");Serial.print("\t");Serial.println(volts_y);
      if(volts_x < 1){
        move_x = 1;
      }
      else if(volts_x > 4){
        move_x = -1;
      }
      else{
        move_x = 0;
      }
      if(volts_y < 1){
        move_y = 1;
      }
      else if(volts_y > 4){
        move_y = -1;
      }
      else{
        move_y = 0;
      }
      Serial.print("x ");
      Serial.println(move_x);
      Serial.print("y ");
      Serial.println(move_y);
      if((move_x != prev_x) || (move_y != prev_y)){
        Serial.println("Sent");
        prev_x = move_x;
        prev_y = move_y;
        txId = preProcess(data, 0, 1); //get ID for motion control info message
        CAN0.send_Msg(txId, 0, data, sizeof(data)); //send motion control info
        delay(250);
      }
    }
    txId = preProcess(data, 0, 0); //get ID for on_off status message
    CAN0.send_Msg(txId, 0, data, sizeof(data)); //send on_off status
  }
}

INT32U preProcess(byte *data, INT32U to_addr, INT32U descriptor){
  //each case is for a different ID 
  //users should plan ahead and know the contents of each message
  //in order to process information correctly
  INT32U transmitID = CAN0.buildTransmitID(to_addr, descriptor);
  int _size=0;
  
  switch(transmitID){
    case 0x008: //sent from node 1 to node 0
      //reset data array to all zeros
      memset(data,0,sizeof(data)); 
      //begin filling data array
      memcpy(data, (byte *)&on_off, sizeof(on_off));
      _size += sizeof(on_off);
      memcpy((data+_size), (byte *)&right, sizeof(right));
      _size += sizeof(right);
      memcpy((data+_size), (byte *)&left, sizeof(left));      
      break;
    case 0x009: //sent from node 1 to node 0
      //reset data array to all zeros
      memset(data,0,sizeof(data)); 
      //begin filling data array
      memcpy(data, (byte *)&move_x, sizeof(move_x));
      _size += sizeof(move_x);
      memcpy((data+_size), (byte *)&move_y, sizeof(move_y));      
      break;
  }
  return transmitID;
}
