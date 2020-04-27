/*
Functions to separate variables into bytes to be sent over the
CAN bus and reassemble them after receiving the data.
*/

#include <mcp_can.h>
#include <TTU_IsoTp.h>
#include <SPI.h>
#include <TTUCAN.h>

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];                        // Array to store serial string

#define CAN0_INT 2                              // Set INT to pin 2
TTUCAN CAN0(10, CAN0_INT, 3, 0);                              // Set CS to pin 10

INT32U preProcess(byte *data, INT32U to_addr, INT32U descriptor); //create data array from variables
void postProcess(byte *data, INT32U receiveID); //update variables from data array

float temp = 17.5;
int count = 325;
double speed_ = 35.5;
float length_ = 126.8;

void setup()
{
  Serial.begin(115200);
  byte data[128];
  //pre-processing data
  preProcess(data,1);

  Serial.println(sizeof(data));

  len = sizeof(data);
  Serial.println(len);
  
  //post-processing data
  postProcess(data, 1);

}

void loop()
{

}

//use this function before send_Msg() in order to fill the data array before transmission
//this function is a template that must be modified for each node/application
//this function can only access and modify global variables
//user must ensure that the array is of appropriate size
INT32U preProcess(byte *data, INT32U to_addr, INT32U descriptor){
  //each case is for a different ID 
  //users should plan ahead and know the contents of each message
  //in order to process information correctly
  INT32U transmitID CAN0.buildMsgID(to_addr, descriptor)
  int _size;
  
  switch(transmitID){
    case 0:
      //reset data array to all zeros
      memset(data,0,sizeof(data)); 
      //begin filling data array
	  _size = sizeof(temp);
      memcpy(data, (byte *)&temp, sizeof(temp));
      memcpy((data+_size), (byte *)&count, sizeof(count));
      _size += sizeof(count);
      memcpy((data+_size), (byte *)&speed_, sizeof(speed_));
      _size += sizeof(speed_);
      memcpy((data+_size), (byte *)&length_, sizeof(length_));
      break;
    default:
      //reset data array to all zeros
      memset(data,0,sizeof(data)); 
      //begin filling data array
      _size = sizeof(temp);
      memcpy(data, (byte *)&temp, sizeof(temp));
      memcpy((data+_size), (byte *)&count, sizeof(count));
      _size += sizeof(count);
      memcpy((data+_size), (byte *)&speed_, sizeof(speed_));
      break;
  }
  return transmitID;
}

//use this function after receive_Msg() in order to process the received data
//this function is a template that must be modified for each node/application
//this function can only access and modify global variables
//user must ensure that the array is of appropriate size
void postProcess(byte *data, INT32U receiveID){
  //each case is for a different ID 
  //users should plan ahead and know the contents of each message
  //in order to process information correctly
  int _size;
  switch(receiveID){
    case 0: 
      _size = sizeof(temp); //starting index of data array
      memcpy(&temp, data, sizeof(temp));
      memcpy(&count, (data +_size), sizeof(count));
      _size +=sizeof(count); //increment index of array by size of count to access the next variable
      memcpy(&speed_, (data +_size), sizeof(speed_));
      _size +=sizeof(speed_);   //increment index of array by size of speed_ to access the next variable
      memcpy(&length_, (data +_size), sizeof(length_)); // store value from array into variable to be used in program
      
      Serial.println(temp);
      Serial.println(count);
      Serial.println(speed_);
      Serial.println(length_);
      break;
    default:
      _size = sizeof(temp);
      memcpy(&temp, data, sizeof(temp));
      memcpy(&count, (data +_size), sizeof(count));
      _size +=sizeof(count);
      memcpy(&speed_, (data +_size), sizeof(speed_));
  
      Serial.println(temp);
      Serial.println(count);
      Serial.println(speed_);
      break;
  }
}
