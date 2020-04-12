//TTUCAN.cpp
#include "Arduino.h"
#include "TTUCAN.h"
#include <iso-tp-derived.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>


TTUCAN::TTUCAN(INT8U _CS, uint8_t mcp_int, INT32U _address, INT8U Ext) : IsoTP(_CS, mcp_int)
{
	if(Ext != 0 && Ext != 1){
		Serial.println("ERROR: Ext must be either 0 or 1. CAN is not initialized!");
		return 0;
	}
	else if(Ext &&(_address > 255 || _address < 0)){
		Serial.println("ERROR: Adresses must be between 0-255 for _extended configuration. CAN is not initialized!");
		return 0;
	}
	else if(!Ext &&(_address > 15 || _address < 0)){
		Serial.println("ERROR: Adresses must be between 0-15 for standard configuration. CAN is not initialized!");
		return 0;
	}
	
	nodeAddress = _address;
	_ext = Ext;
	
	if(!_ext){ //standard
		numNodes=15; //don't include home node
		homeAddress = 15;
		pingID = ((((nodeAddress << 27)|(nodeAddress << 23))|0x00700000)>>20);
		pingResponse = ((((homeAddress << 27)|(nodeAddress << 23))|0x00700000)>>20);
		networkStatusID = 0x7FF;
	}
	else{
		numNodes=255; //don't include home node
		homeAddress = 255;
		pingID = (((nodeAddress << 21)|(nodeAddress << 13))|0x001FFF) | 0x80000000;
		pingResponse = (((homeAddress << 21)|(nodeAddress << 13))|0x001FFF) | 0x80000000;
		networkStatusID = 0x1FFFFFFF | 0x80000000;
	}
}


int TTUCAN::TTU_begin(){
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (nodeAddress == 15 || nodeAddress == 255){                      //create HOME node with no filters
    if(begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK)
      Serial.println("MCP2515 Initialized Successfully!");
    else
      Serial.println("Error Initializing MCP2515..."); 
      return 0;                  
    disOneShotTX();
    setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.   
    return 1; 
  }

  //initialize all masks and filters - masks should not be changed, filters 0-2 should not be changed. 
  //filters 3-5 can be changed to accept messages addressed to three other nodes (use add_filter() to chnage these filters)
  switch(_ext){
    case 0: //standard - 16 nodes max, numbered 0-15
      if(begin(MCP_STD_ext, CAN_500KBPS, MCP_16MHZ) == CAN_OK)
        Serial.println("MCP2515 Initialized Successfully! - Standard");
      else
        Serial.println("Error Initializing MCP2515..."); 
      disOneShotTX();
      
      init_Mask(0,0,0x7FF00000);                // Init first mask - all ID bits must match
      init_Filt(0,0,0x7FF00000);                // Init first filter to general network ping
      init_Filt(1,0,((((nodeAddress << 27)|(nodeAddress << 23))|0x00700000)>>20)); // Init second filter to direct ping message
      
      init_Mask(1,0,0x78000000);                // Init second mask - first 4 ID bits must match 
      init_Filt(2,0,((nodeAddress << 27)));                // Init third filter to accept msgs directed to this node
      //if these filters aren't used, default is to be the same as second filter
      init_Filt(3,0,((nodeAddress << 27)));
      init_Filt(4,0,((nodeAddress << 27)));
      init_Filt(5,0,((nodeAddress << 27)));    
      break;
    case 1: //_extended - 256 nodes max, numbered 0-255
      if(begin(MCP_STD_ext, CAN_500KBPS, MCP_16MHZ) == CAN_OK)
        Serial.println("MCP2515 Initialized Successfully! - _extended");
      else
        Serial.println("Error Initializing MCP2515..."); 
      disOneShotTX();
      
      init_Mask(0,1,0x1FFFFFFF);                // Init first mask...
      init_Filt(0,1,0x1FFFFFFF);                // Init first filter to general network ping
      init_Filt(1,1,(((nodeAddress << 21)|(nodeAddress << 13))|0x001FFF));  // Init second filter to direct ping message
      
      init_Mask(1,1,0x1FE00000);                // Init second mask... 
      init_Filt(2,1,(nodeAddress << 21));                // Init third filter to accept msgs directed to this node
      //if these filters aren't used, default is to be the same as second filter
      init_Filt(3,1,(nodeAddress << 21));
      init_Filt(4,1,(nodeAddress << 21));
      init_Filt(5,1,(nodeAddress << 21));
      break;
    default:
      Serial.println("ERROR: _ext must be equal to 0 or 1. CAN is not initialized!");
      return 0;
  }
 
  setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.
  return 1;
}

int TTUCAN::addFilter(INT32U filterAddress, INT8U _ext, INT8U filterRegister){
   if(_ext &&(filterAddress > 255 || filterAddress < 0)){
    Serial.println("ERROR: Adresses must be between 0-255 for _extended configuration. Filter is not initialized!");
    return 0;
  }
  else if(!_ext &&(filterAddress > 15 || filterAddress < 0)){
    Serial.println("ERROR: Adresses must be between 0-15 for standard configuration. Filter is not initialized!");
    return 0;
  }
  else if(filterRegister > 5 || filterRegister < 3){
    Serial.println("ERROR: Only filters 3, 4, and 5 can be altered. Filter is not initialized!");
    return 0;
  }
  
  // Init filter to also accept msgs directed to node specified by address
  switch(_ext){
    case 0:
      init_Filt(filterRegister,0,(filterAddress << 27));
      break;
    case 1:
      init_Filt(filterRegister,1,(filterAddress << 21));
      break;
    default:
      Serial.println("ERROR: _ext must be equal to 0 or 1. Filter is not initialized!");
      return 0;    
  }
  return 1; 
}

INT32U TTUCAN::buildMsgID(INT32U to_addr, INT32U descriptor){
  if(_ext &&(to_addr > 255 || to_addr < 0)){
    Serial.println("ERROR: Adresses must be between 0-255 for _extended configuration.");
    return 0;
  }
  else if(!_ext &&(to_addr > 15 || to_addr < 0)){
    Serial.println("ERROR: Adresses must be between 0-15 for standard configuration.");
    return 0;
  }
  else if(_ext &&(descriptor > 8191 || descriptor < 0)){
    Serial.println("ERROR: Data descriptor value must be between 0-15 for standard configuration.");
    return 0;
  }
  else if(!_ext &&(descriptor > 7 || descriptor < 0)){
    Serial.println("ERROR: Data descriptor value must be between 0-15 for standard configuration.");
    return 0;
  }
  //Create CAN ID to be sent with message
  INT32U msgID; 
  switch(_ext){
    case 0:
      msgID = (((to_addr << 27)|(nodeAddress << 23))|(descriptor << 20))>>20;
	  return msgID;
      break;
    case 1:
      msgID = ((to_addr << 21)|(nodeAddress << 13))|(descriptor) | 0x80000000; //recognize as _extended frame
	  return msgID;
      break;
    default:
      Serial.println("ERROR: _ext must be equal to 0 or 1. Message not sent!");
      return 0;    
  }  
  
}

int TTUCAN::send_Msg(INT32U to_addr, INT8U rtr, INT32U descriptor, INT8U *data, INT32U len){
  if(rtr != 0 && rtr != 1){
    Serial.println("ERROR: rtr must be equal to 0 or 1. Message not sent!");
    return 0;     
  }
  else if(_ext &&(to_addr > 255 || to_addr < 0)){
    Serial.println("ERROR: Adresses must be between 0-255 for _extended configuration. Message not sent!");
    return 0;
  }
  else if(!_ext &&(to_addr > 15 || to_addr < 0)){
    Serial.println("ERROR: Adresses must be between 0-15 for standard configuration. Message not sent!");
    return 0;
  }
  else if(_ext &&(descriptor > 8191 || descriptor < 0)){
    Serial.println("ERROR: Data descriptor value must be between 0-15 for standard configuration. Message not sent!");
    return 0;
  }
  else if(!_ext &&(descriptor > 7 || descriptor < 0)){
    Serial.println("ERROR: Data descriptor value must be between 0-15 for standard configuration. Message not sent!");
    return 0;
  }

  INT32U msgID; //CAN ID to be sent with message
  switch(_ext){
    case 0:
      msgID = (((to_addr << 27)|(nodeAddress << 23))|(descriptor << 20))>>20;
      break;
    case 1:
      msgID = ((to_addr << 21)|(nodeAddress << 13))|(descriptor) | 0x80000000; //recognize as _extended frame
      break;
    default:
      Serial.println("ERROR: _ext must be equal to 0 or 1. Message not sent!");
      return 0;    
  }
  if(rtr){
    msgID |= 0x40000000; //write as remote request
  }

  byte sndStat = sendMsgBuf(msgID, len, data);
  if(sndStat == CAN_OK){
    Serial.println("Message Sent Successfully!");
    return 1;
  } else {
    Serial.println("Error Sending Message...");
    return 0;
  }
}

int TTUCAN::send_Msg(INT32U msgID, INT8U rtr, INT8U *data, INT32U len){
  if(rtr != 0 && rtr != 1){
    Serial.println("ERROR: rtr must be equal to 0 or 1. Message not sent!");
    return 0;     
  }
  
  if(rtr){
    msgID |= 0x40000000; //write as remote request
  }

  byte sndStat = sendMsgBuf(msgID, len, data);
  if(sndStat == CAN_OK){
    Serial.println("Message Sent Successfully!");
    return 1;
  } else {
    Serial.println("Error Sending Message...");
    return 0;
  }
}

int TTUCAN::receive_Msg(INT32U *id, INT8U *len, INT8U *buf){
	int result;
	memset(rxBuffer,0,sizeof(rxBuffer));
	result = readMsgBuf(&rxId, &rxLen, rxBuffer);
	
	*id = rxId;
	*len = rxLen;
	for(int i = 0; i<rxLen; i++){
        buf[i] = rxBuffer[i];
	}
	
	if(rxId == pingID){
		byte data[] = {0xFF};
		for(int i=0; i<1; i++){
			sendMsgBuf(pingResponse, 1, data); //send msg, len = 1
			//delay(20);
		}
	}
	return result; //status of receive function
}

INT8U TTUCAN::check_Receive(void)
{
    INT8U res;
    res = checkReceive();
    return res;
}

INT8U TTUCAN::check_Error(void)
{
    INT8U eflg = checkError();
    return eflg;
}

//************************************************ Home Node functions *************************************************//

void TTUCAN::homeMenu(){
	if(nodeAddress != homeAddress){
		Serial.println("This function is reserved for the HOME Node only!");
		return;
	}
  Serial.println();
  Serial.println("****** Command Options: ******");
  Serial.println("*  display - Continuously display messages sent over bus. Type \"s\" to end function.");
  Serial.println("*  network - Check if the network has been established.");
  Serial.println("*  nodes - Check which nodes are connected to the network. Type \"s\" to end function.");
  Serial.println("*  (integer value) - Ping node address. Input should be 0-14 for standard or 0-254 for _extended.");
  Serial.println();
  Serial.println("Input command at the top of the Serial Monitor. Make sure line ending is set to Newline.");
}

//display all messages sent on the bus
void TTUCAN::displayActivity(){
	if(nodeAddress != homeAddress){
		Serial.println("This function is reserved for the HOME Node only!");
		return;
	}
	Serial.println("Displaying bus activity.  Type \"s\" to end function.");
    delay(1000);
	
  while(1){
    //check for stop command
    if(Serial.available() > 0){
      rxByte = Serial.read(); //get one character
      if(rxByte != '\n'){
        if(rxByte == 's'){
          break;
        } 
      }
      else{
        rxByte = 0;
      }
    }
    // If CAN0_INT pin is low, read receive buffer
    if(!digitalRead(CAN0_INT))                         
    {
      readMsgBuf(&rxId, &rxLen, rxBuffer);      // Read data: rxLen = data length, buf = data byte(s)
      
      if((rxId & 0x80000000) == 0x80000000)     // Determine if ID is standard (11 bits) or _extended (29 bits)
        sprintf(msgString, "_extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), rxLen);
      else
        sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, rxLen);
    
      Serial.print(msgString);
    
      if((rxId & 0x40000000) == 0x40000000){    // Determine if message is a remote request frame.
        sprintf(msgString, " REMOTE REQUEST FRAME");
        Serial.print(msgString);
      } else {
        for(byte i = 0; i<rxLen; i++){
          sprintf(msgString, " 0x%.2X", rxBuffer[i]);
          Serial.print(msgString);
        }
      }
          
      Serial.println();  
    }
  }
}

//send lowest priority message to check if there is a network
int TTUCAN::networkStatus(INT8U _ext){ 
	if(nodeAddress != homeAddress){
		Serial.println("This function is reserved for the HOME Node only!");
		return;
	}
	Serial.println("Checking network status.");
  byte data[] = {0xFF};
  int len = sizeof(data);
  byte sndStat = sendMsgBuf(networkStatusID, len, data);
  if(sndStat == CAN_OK){
    Serial.println("Network is functioning!");
    return 1;
  } else {
    Serial.println("Network Error: check connections...");
    return 0;
  }  
}

//ping all nodes to get a list of functioning nodes
int TTUCAN::checkNodes(INT8U _ext){
	if(nodeAddress != homeAddress){
		Serial.println("This function is reserved for the HOME Node only!");
		return;
	}
	Serial.println("Checking which nodes are functioning.  Type \"s\" to end function.");
    delay(1000);
  int currentTime, sentTime;
  INT32U ping_address = 0; //start with node 0
  INT32U ID, return_ID; 
  byte data[] = {0xFF};
  int TimeOut = 5000;

  for(int i=0; i<numNodes; i++){
    //check for stop command
    if(Serial.available() > 0){
      rxByte = Serial.read(); //get one character
      if(rxByte != '\n'){
        if(rxByte == 's'){
          break;
        } 
      }
      else{
        rxByte = 0;
      }
    }
    //create message ID addressed to each node
    Serial.print("Checking node ");
    Serial.print(i);
    Serial.println("...");
    if(!_ext){ //standard message
      ID = ((((ping_address << 27)|(ping_address << 23))|0x00700000)>>20);
      return_ID = ((((homeAddress << 27)|(ping_address << 23))|0x00700000)>>20);
    }
    else{ //_extended message
      ID = (((ping_address << 21)|(ping_address << 13))|0x001FFF);
      ID |= 0x80000000;
      return_ID = (((homeAddress << 21)|(ping_address << 13))|0x001FFF);
      return_ID |= 0x80000000;
    }
    //Serial.println(ID, HEX);
    ID |= 0x40000000; //send as remote request
    sendMsgBuf(ID, 1, data); //send msg 
    sentTime = millis();   
    //wait for response - need to decide timeout
    while(1){
      readMsgBuf(&rxId, &rxLen, rxBuffer);
      if(rxId == return_ID){
        sprintf(msgString, "Node %d is functioning.", i);
        Serial.println(msgString);
        break; //end while loop
      }
      else{
        currentTime = millis() - sentTime; //check timeout
        if(currentTime >= TimeOut){ //5 second timeout to wait for return message
          sprintf(msgString, "Node %d is not functioning.", i);
          Serial.println(msgString);
          break; //end while loop
        }
      }
    } //end while loop
    ping_address++; //move on to n_ext address
  } //end for loop

  return 1;
}

//ping one node directly to check if it is functioning
int TTUCAN::pingNode(INT32U ping_address, INT8U _ext){
	if(nodeAddress != homeAddress){
		Serial.println("This function is reserved for the HOME Node only!");
		return;
	}
	sprintf(msgString, "Ping node: %d", address);
    Serial.println(msgString);
	
  int currentTime, sentTime;
  INT32U ID, return_ID, homeAddress; 
  byte data[] = {0xFF};
  int TimeOut = 5000;

  if(_ext && (ping_address > 255 || ping_address < 0)){
    Serial.println("ERROR: Adresses must be between 0-255 for _extended configuration.");
    return 0;
  }
  else if(!_ext && (ping_address > 15 || ping_address < 0)){
    Serial.println("ERROR: Adresses must be between 0-15 for standard configuration.");
    return 0;
  }
  if (ping_address == 15 || ping_address == 255){
    Serial.println("ERROR: Cannot ping home node!");
  }
  sprintf(msgString, "Pinging node %d...", ping_address);
  Serial.println(msgString);

  if(!_ext){ //standard
    homeAddress = 15;
  }
  else{
    homeAddress = 255;
  }

  if(!_ext){ //standard message
    ID = ((((ping_address << 27)|(ping_address << 23))|0x00700000)>>20);
    return_ID = ((((homeAddress << 27)|(ping_address << 23))|0x00700000)>>20);
  }
  else{ //_extended message
    ID = (((ping_address << 21)|(ping_address << 13))|0x001FFF);
    ID |= 0x80000000;
    return_ID = (((homeAddress << 21)|(ping_address << 13))|0x001FFF);
    return_ID |= 0x80000000;
  }
  //Serial.println(ID, HEX);
  ID |= 0x40000000; //send as remote request
  sendMsgBuf(ID, 1, data); //send msg 
  sentTime = millis();   
  //wait for response - need to decide timeout
  while(1){
    readMsgBuf(&rxId, &rxLen, rxBuffer);
    if(rxId == return_ID){
      sprintf(msgString, "Node %d is functioning.", ping_address);
      Serial.println(msgString);
      //Serial.println("Node is functioning.");
      break; //end while loop
    }
    else{
      currentTime = millis() - sentTime; //check timeout
      if(currentTime >= TimeOut){ //5 second timeout to wait for return message
        sprintf(msgString, "Node %d is not functioning.", ping_address);
        Serial.println(msgString);
        //Serial.println("Node is not functioning.");
        break; //end while loop
      }
    }
  } //end while loop
  return 1;
}