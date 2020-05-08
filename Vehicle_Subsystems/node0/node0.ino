// Actuators for mirror position control
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
int on_off = 0, left = 0, right = 0;
int move_x = 0, move_y = 0;
//variables for stepper motor control
int sequence[] = {0x80, 0xC0, 0x40, 0x60, 0x20, 0x30, 0x10, 0x90, 0x00};
int _stepr=0, _stepl=0;
int countr=0, countl=0;
int rdir=1, ldir=1, prevRdir=1, prevLdir=1;

#define CAN0_INT 2                              // Set INT to pin 2
TTUCAN CAN0(10, CAN0_INT, 0, 0);                               // Set CS to pin 10

void leftServo();
void rightServo();
void leftStepper();
void rightStepper();
void postProcess(byte *data, INT32U receiveID); //update variables from data array

void setup()
{
  Serial.begin(115200);
  DDRE &= 0xEF; //set PE4 as input for CAN interrupt
  DDRA = 0xF0; //left stepper
  DDRC = 0xF0; //right stepper
  //use OC3A - PE3 as pwm pin left servo output
  DDRE = 0b00001000;
  //use OC5B - PL4 as pwm pin right servo output
  DDRL = 0b00010000; 
  //fast pwm 10 bit
  TCCR3A = 0b10000011; //OCR3A
  TCCR5A = 0b00100011; //OCR5B
  //prescale of 256; 60 cnts = 1ms
  TCCR3B = 0b00001100; //OCR3A
  TCCR5B = 0b00001100; //OCR5B

  //set initial servo positions
  OCR3A = 105; //move_y 6 steps, low is 160, high is 50, increment 18
  OCR5B = 95; //move_y 6 steps, low is 160, high is 30, increment 21
  delay(1500);
  
  CAN0.TTU_begin();
}

void loop()
{
  if(!(PORTE | 0xEF))                         // If CAN0_INT pin is low, read receive buffer
  {
    CAN0.receive_Msg(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    Serial.println();
    postProcess(rxBuf, rxId); //post process data to use information received
  }
  if(on_off){
    if(left){
      leftServo();
      leftStepper();
    }
    else if(right){
      rightServo();
      rightStepper();
    }
  }
}

void postProcess(byte *data, INT32U receiveID){
  //each case is for a different ID 
  //users should plan ahead and know the contents of each message
  //in order to process information correctly
  int _size=0;
  switch(receiveID){
    case 0x008: //sent from node 3 to node 1 
      memcpy(&on_off, data, sizeof(on_off));
      _size += sizeof(on_off);
      memcpy(&right, (data +_size), sizeof(right));
      _size += sizeof(right); //starting index of data array     
      memcpy(&left, (data +_size), sizeof(left));
      Serial.print("Power ");
      Serial.println(on_off);
      Serial.print("right ");
      Serial.println(right);
      Serial.print("left ");
      Serial.println(left);
      break;
    case 0x009: //sent from node 2 to node 3 (use addFilter() to be able to see messages sent to another node)
      memcpy(&move_x, data, sizeof(move_x));
      _size += sizeof(move_x); //starting index of data array     
      memcpy(&move_y, (data +_size), sizeof(move_y));
      Serial.print("X ");
      Serial.println(move_x);
      Serial.print("Y ");
      Serial.println(move_y);      
      break;
  }
}

void leftServo(){
  //servo needs value between 50-160 for high time of pwm signal
  int increment = 18;
  if(move_y == 1){
    if(OCR3A > 51){
      OCR3A -= increment;
    }
  }
  else if(move_y == -1){
    if(OCR3A < 159){
      OCR3A += increment;
    }
  }
  move_y=0; //reset
}

void rightServo(){
  //servo needs value between 30-160 for high time of pwm signal 
  int increment = 21;
  if(move_y == 1){
    if(OCR5B > 32){
      OCR5B -= increment;
    }
  }
  else if(move_y == -1){
    if(OCR5B < 158){
      OCR5B += increment;
    }
  }
  move_y=0; //reset
}

void leftStepper(){
 int increment = 375; //6 steps
 if(move_x != 0){
  if(move_x == 1){
    ldir = 1;
    if(prevLdir == 0){
      countl = 2250-countl;
      prevLdir = 1;
    }
  }
  else{
    ldir = 0;
    if(prevLdir == 1){
      countl = 2250-countl;
      prevLdir = 0;
    }
  }
  if(countl < 2250){
   countl+=increment;
   for(int i=0;i<increment;i++){
    PORTA = sequence[_stepl];
    if(ldir){
     _stepl++;
     }else{
     _stepl--;
     }
     if(_stepl>7){
     _stepl=0;
     }
     if(_stepl<0){
     _stepl=7;
     }
    delay(1);
   }
  }
 }
 move_x=0; //reset
}

void rightStepper(){
 int increment = 375; //6 steps
 if(move_x != 0){
  if(move_x == 1){
    rdir = 1;
    if(prevRdir == 0){
      countr = 2250-countr;
      prevRdir = 1;
    }
  }
  else{
    rdir = 0;
    if(prevRdir == 1){
      countr = 2250-countr;
      prevRdir = 0;
    }
  }
  if(countr < 2250){
   countr+=increment;
   for(int i=0;i<increment;i++){
    PORTC = sequence[_stepr];
    if(rdir){
     _stepr++;
     }else{
     _stepr--;
     }
     if(_stepr>7){
     _stepr=0;
     }
     if(_stepr<0){
     _stepr=7;
     }
    delay(1);
   }
  }
 }
 move_x=0; //reset
}
