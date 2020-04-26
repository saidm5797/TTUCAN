// CAN Send and Receive Example
#include <mcp_can.h>
#include <TTU_IsoTp.h>
#include <SPI.h>
#include <TTUCAN.h>

long unsigned int rxId, txId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];                        // Array to store serial string
byte data[8];

//example variables
float temp = 17.5;
int count = 325;
double speed_ = 35.5;
float length_ = 126.8;
int passengers = 15;
int gallons = 8;
float miles = 5858.7;

#define CAN0_INT 2                              // Set INT to pin 2
TTUCAN CAN0(10, CAN0_INT, 2, 0);                               // Set CS to pin 10

INT32U preProcess(byte *data, INT32U to_addr, INT32U descriptor); //create data array from variables
void postProcess(byte *data, INT32U receiveID); //update variables from data array

void setup()
{
  Serial.begin(115200);
  pinMode(CAN0_INT, INPUT);                            // Configuring pin for /INT input
  CAN0.TTU_begin();
  CAN0.addFilter(1,3); //be able to see messages sent to node 1 on filter register 3
}

void loop()
{
  
  if(!digitalRead(CAN0_INT))                         // If CAN0_INT pin is low, read receive buffer
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

  txId = preProcess(data, 3, 7); //(data array,to_address, descriptor)
  CAN0.send_Msg(txId, 0, data, sizeof(data)); //(CAN ID, rtr, data array, len of data)
  delay(random(100,250)); //wait random time between 100-250 milliseconds
}

INT32U preProcess(byte *data, INT32U to_addr, INT32U descriptor){
  //each case is for a different ID 
  //users should plan ahead and know the contents of each message
  //in order to process information correctly
  INT32U transmitID = CAN0.buildTransmitID(to_addr, descriptor);
  int _size;
  
  switch(transmitID){
    case 0x197: //sent from node 2 to node 3
      _size = sizeof(count);
      memcpy(data, (byte *)&count, sizeof(count));
      memcpy((data+_size), (byte *)&length_, sizeof(length_));
      _size += sizeof(length_);
      break;
    default: //just a template
      /*
      _size = sizeof(temp);
      memcpy(data, (byte *)&temp, sizeof(temp));
      memcpy((data+_size), (byte *)&count, sizeof(count));
      _size += sizeof(count);
      memcpy((data+_size), (byte *)&speed_, sizeof(speed_));
      _size += sizeof(speed_);
      memcpy((data+_size), (byte *)&length_, sizeof(length_));
      */
      break;
  }
  return transmitID;
}

void postProcess(byte *data, INT32U receiveID){
  //each case is for a different ID 
  //users should plan ahead and know the contents of each message
  //in order to process information correctly
  int _size;
  switch(receiveID){
    case 0x10B: //sent from node 1 to node 2
      _size = sizeof(temp); //starting index of data array
      memcpy(&temp, data, sizeof(temp));
      memcpy(&speed_, (data +_size), sizeof(speed_));
      _size +=sizeof(speed_); //increment index of array by size of count to access the next variable

      Serial.print("Temp: ");
      Serial.println(temp);
      Serial.print("Speed: ");
      Serial.println(speed_);
      break;
    case 0x99: //sent from node 3 to node 1 (use addFilter() to be able to see messages sent to another node)
      _size = sizeof(passengers); //starting index of data array
      memcpy(&passengers, data, sizeof(passengers));
      memcpy(&gallons, (data +_size), sizeof(gallons));
      _size +=sizeof(gallons); //increment index of array by size of count to access the next variable
      memcpy(&miles, (data +_size), sizeof(miles));
      _size +=sizeof(miles);   //increment index of array by size of speed_ to access the next variable

      Serial.print("Passengers: ");
      Serial.println(passengers);
      Serial.print("Gallons: ");
      Serial.println(gallons);
      Serial.print("Miles: ");
      Serial.println(miles);
    default: //just a template
      /*
      _size = sizeof(temp);
      memcpy(&temp, data, sizeof(temp));
      memcpy(&count, (data +_size), sizeof(count));
      _size +=sizeof(count);
      memcpy(&speed_, (data +_size), sizeof(speed_));
  
      Serial.println(temp);
      Serial.println(count);
      Serial.println(speed_);
      */
      break;
  }
}
