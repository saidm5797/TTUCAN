//TTUCAN.cpp
#include "Arduino.h"
#include "TTUCAN.h"
#include <TTU_IsoTp.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>

/*********************************************************************************************************
** Function name:           TTUCAN
** Descriptions:            Public function to declare CAN class
** Arguments:               CS pin, Interrupt pin, node address, ext (0 or 1)               
*********************************************************************************************************/
TTUCAN::TTUCAN(INT8U _CS, uint8_t mcp_int, INT32U _address, INT8U Ext) : IsoTp(_CS, mcp_int)
{
	if(Ext != 0 && Ext != 1){
		Serial.println("ERROR: Ext must be either 0 or 1. CAN is not initialized!");
		return;
	}
	else if(Ext &&(_address > 255 || _address < 0)){
		Serial.println("ERROR: Adresses must be between 0-255 for _extended configuration. CAN is not initialized!");
		return;
	}
	else if(!Ext &&(_address > 15 || _address < 0)){
		Serial.println("ERROR: Adresses must be between 0-15 for standard configuration. CAN is not initialized!");
		return;
	}
	
	nodeAddress = _address;
	_ext = Ext;
	
	if(!_ext){ //standard
		numNodes=15; //don't count home node
		homeAddress = 15;
		pingID = ((((nodeAddress << 23)|(nodeAddress << 19))|0x00070000)>>16) | 0x40000000;
		pingFilter = ((((nodeAddress << 23)|(nodeAddress << 19))|0x00070000));
		nodeFilter = (nodeAddress << 23);
		pingResponse = ((((homeAddress << 27)|(nodeAddress << 23))|0x00700000)>>20);
		networkStatusID = 0x07FF0000;
	}
	else{
		numNodes=255; //don't count home node
		homeAddress = 255;
		pingID = (((nodeAddress << 21)|(nodeAddress << 13))|0x00001FFF) | 0x80000000 | 0x40000000;
		nodeFilter = (nodeAddress << 21);		
		pingFilter = (((nodeAddress << 21)|(nodeAddress << 13))|0x00001FFF);
		pingResponse = (((homeAddress << 21)|(nodeAddress << 13))|0x00001FFF) | 0x80000000;
		networkStatusID = 0x1FFFFFFF | 0x80000000;
	}
}

/*********************************************************************************************************
** Function name:           TTU_begin
** Descriptions:            Public function to initialize CAN object/set up initial filters
** Arguments:               default: idmodeset = MCP_STDEXT, speedset = CAN_500KBPS, clockset = MCP_16MHZ
** Returns:                 1 if initialized, 0 if failed
*********************************************************************************************************/
int TTUCAN::TTU_begin(INT8U idmodeset, INT8U speedset, INT8U clockset){
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (nodeAddress == homeAddress){                      //create HOME node with no filters
    Serial.println("Home Node");
	if(begin(MCP_ANY, speedset, clockset) == CAN_OK){
      Serial.println("MCP2515 Initialized Successfully!");
	}
    else{
      Serial.println("Error Initializing MCP2515..."); 
      return 0; 
	}
    disOneShotTX();
    setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.   
	return 1; 
  }
  Serial.print("Node ");
  Serial.println(nodeAddress);
  if(begin(idmodeset, speedset, clockset) == CAN_OK){
    if(_ext){
	  Serial.println("MCP2515 Initialized Successfully! - Extended");
	}
	else{
	  Serial.println("MCP2515 Initialized Successfully! - Standard");
	}
  }
  else{
    Serial.println("Error Initializing MCP2515..."); 
  }
  disOneShotTX();
  //initialize all masks and filters - masks should not be changed, filters 0-2 should not be changed. 
  //filters 3-5 can be changed to accept messages addressed to three other nodes (use add_filter() to chnage these filters)
  switch(_ext){
    case 0: //standard - 16 nodes max, numbered 0-15
      init_Mask(0,0,0x07FF0000);                // Init first mask - all ID bits must match
      init_Filt(0,0,networkStatusID);                // Init first filter to general network ping
      init_Filt(1,0,pingFilter); // Init second filter to direct ping message

	  init_Mask(1,0,0x07800000);                // Init second mask - first 4 ID bits must match 
      init_Filt(2,0,nodeFilter);                // Init third filter to accept msgs directed to this node
      //if these filters aren't used, default is to be the same as second filter
      init_Filt(3,0,nodeFilter);
      init_Filt(4,0,nodeFilter);
      init_Filt(5,0,nodeFilter);    
      break;
    case 1: //_extended - 256 nodes max, numbered 0-255
      init_Mask(0,1,0x1FFFFFFF);                // Init first mask...
      init_Filt(0,1,0x1FFFFFFF);                // Init first filter to general network ping
      init_Filt(1,1,pingFilter);  // Init second filter to direct ping message
      
      init_Mask(1,1,0x1FE00000);                // Init second mask... 
      init_Filt(2,1,nodeFilter);                // Init third filter to accept msgs directed to this node
      //if these filters aren't used, default is to be the same as second filter
      init_Filt(3,1,nodeFilter);
      init_Filt(4,1,nodeFilter);
      init_Filt(5,1,nodeFilter);
      break;
    default:
      Serial.println("ERROR: _ext must be equal to 0 or 1. CAN is not initialized!");
      return 0;
  }
 
  setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.
  return 1;
}

/*********************************************************************************************************
** Function name:           addFilter
** Descriptions:            Public function to add optional filters 
** Arguments:               filterAddress - read messages to this node, filterRegister - initialize this register
** Returns:                 1 if initialized, 0 if failed
*********************************************************************************************************/
int TTUCAN::addFilter(INT32U filterAddress, INT8U filterRegister){
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
      init_Filt(filterRegister,0,(filterAddress << 23));
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

/*********************************************************************************************************
** Function name:           buildTransmitID
** Descriptions:            Public function to create ID for messages to be sent 
** Arguments:               address to send to, data descriptor
** Returns:                 created message ID
*********************************************************************************************************/
INT32U TTUCAN::buildTransmitID(INT32U to_addr, INT32U descriptor){
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
	  TxMsg.rx_id = (((nodeAddress << 27)|(to_addr << 23))|(descriptor << 20))>>20; //received ID with FC frame will be adressed to this node with same descriptor
	  return msgID;
      break;
    case 1:
      msgID = ((to_addr << 21)|(nodeAddress << 13))|(descriptor) | 0x80000000; //recognize as _extended frame
	  TxMsg.rx_id = ((nodeAddress << 21)|(to_addr << 13))|(descriptor) | 0x80000000;
	  return msgID;
      break;
    default:
      Serial.println("ERROR: _ext must be equal to 0 or 1. Message not sent!");
      return 0;    
  }  
  
}

/*********************************************************************************************************
** Function name:           buildReceiveID
** Descriptions:            Public function to create ID for messages to be received 
** Arguments:               address to receive from, data descriptor
** Returns:                 created message ID
*********************************************************************************************************/
INT32U TTUCAN::buildReceiveID(INT32U from_addr, INT32U descriptor){
  if(_ext &&(from_addr > 255 || from_addr < 0)){
    Serial.println("ERROR: Adresses must be between 0-255 for _extended configuration.");
    return 0;
  }
  else if(!_ext &&(from_addr > 15 || from_addr < 0)){
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
      msgID = (((nodeAddress << 27)|(from_addr << 23))|(descriptor << 20))>>20;
	  return msgID;
      break;
    case 1:
      msgID = ((nodeAddress << 21)|(from_addr << 13))|(descriptor) | 0x80000000; //recognize as _extended frame
	  return msgID;
      break;
    default:
      Serial.println("ERROR: _ext must be equal to 0 or 1. Message not sent!");
      return 0;    
  }  
  
}

/*********************************************************************************************************
** Function name:           send_Msg
** Descriptions:            Public function to send CAN messages 
** Arguments:               CAN ID, remote frame (0 or 1), data array, data length in bytes
** Returns:                 1 if sent successfully, 0 if error
*********************************************************************************************************/
int TTUCAN::send_Msg(INT32U msgID, INT8U rtr, INT8U *data, INT32U len){
  if(rtr != 0 && rtr != 1){
    Serial.println("ERROR: rtr must be equal to 0 or 1. Message not sent!");
    return 0;     
  }
   
  if(rtr){
    msgID |= 0x40000000; //write as remote request
  }
#ifdef ISOTP_COMM
  TxMsg.len=sizeof(data);
  TxMsg.Buffer=(uint8_t *)calloc(MAX_MSGBUF,sizeof(uint8_t));
  TxMsg.tx_id=msgID;
  memcpy(TxMsg.Buffer,data,sizeof(data));
  Serial.println(F("Send..."));
  send(&TxMsg); 
#else
  if(len > 8){
	Serial.println("Error: Data array must be 8 bytes or less! Define ISOTP_COMM in TTUCAN.h to send larger arrays.");
	return 0;
  }
  byte sndStat = sendMsgBuf(msgID, len, data);
  if(sndStat == CAN_OK){
	Serial.println("Message Sent Successfully!");
    return 1;
  } else {
	Serial.println("Error Sending Message...");
	return 0;
  }
#endif
}

/*********************************************************************************************************
** Function name:           receive_Msg
** Descriptions:            Public function to receive CAN messages 
** Arguments:               address of ID variable (&rxId), address of data length variable (&len), data array
** Returns:                 1 if received successfully, 0 if error
*********************************************************************************************************/
int TTUCAN::receive_Msg(INT32U *id, INT8U *len, INT8U *buf){
	int result;
#ifdef ISOTP_COMM
	RxMsg.tx_id=can_id; //must be configured to send back to node that sent FF or CF - needs development
	RxMsg.Buffer=(uint8_t *)calloc(MAX_MSGBUF,sizeof(uint8_t));
	receive(&RxMsg);
	if(RxMsg.rx_id == pingID){
		byte data[] = {0xFF};
		byte sndStat = sendMsgBuf(pingResponse, 1, data); //send msg, len = 1
		if(sndStat == CAN_OK){
			Serial.println("Ping Response Sent Successfully!");
			return 1;
		} else {
			Serial.println("Error Sending Message...");
			return 0;
		}
	}
	result = 1;
#else
	memset(rxBuffer,0,sizeof(rxBuffer));
	result = readMsgBuf(&rxId, &rxLen, rxBuffer);	
	*id = rxId;
	*len = rxLen;
	for(int i = 0; i<rxLen; i++){
        buf[i] = rxBuffer[i];
	}
	
	if(rxId == pingID){
		byte data[] = {0xFF};
		byte sndStat = sendMsgBuf(pingResponse, 1, data); //send msg, len = 1
		if(sndStat == CAN_OK){
			Serial.println("Ping Response Sent Successfully!");
			return 1;
		} else {
			Serial.println("Error Sending Message...");
			return 0;
		}
	}
#endif
	return result; //status of receive function
}

/*********************************************************************************************************
** Function name:           checkReceive
** Descriptions:            Public function, Checks for received data.  (Used if not using the interrupt output)
*********************************************************************************************************/
INT8U TTUCAN::check_Receive(void)
{
    INT8U res;
    res = checkReceive();
    return res;
}

/*********************************************************************************************************
** Function name:           checkError
** Descriptions:            Public function, Returns error register data.
*********************************************************************************************************/
INT8U TTUCAN::check_Error(void)
{
    INT8U eflg = checkError();
    return eflg;
}


//************************************************ Home Node functions *************************************************//


/*********************************************************************************************************
** Function name:           homeMenu
** Descriptions:            Public function to display home node menu
*********************************************************************************************************/
void TTUCAN::homeMenu(void){
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
  Serial.println("** Input command at the top of the Serial Monitor. Make sure line ending is set to Newline. **");
  Serial.println();
  Serial.println();
}

/*********************************************************************************************************
** Function name:           displayActivity
** Descriptions:            Public function to display all messages sent on the bus
*********************************************************************************************************/
void TTUCAN::displayActivity(void){
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
	
    // If _mcp_int pin is low, read receive buffer
    if(!digitalRead(_mcp_int))   //can_receive()// (PINE & 0x10) == 0                   
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

/*********************************************************************************************************
** Function name:           networkStatus
** Descriptions:            Public function to send lowest priority message to check if there is a functioning network
** Returns:                 1 if sent successfully, 0 if error
*********************************************************************************************************/
int TTUCAN::networkStatus(void){ 
	if(nodeAddress != homeAddress){
		Serial.println("This function is reserved for the HOME Node only!");
		return 0;
	}
  Serial.println("Checking network status.");
  byte data[] = {0xFF};
  int len = sizeof(data);
  byte sndStat = sendMsgBuf(networkStatusID, len, data); //only waits for ack bit, this message is never processed
  if(sndStat == CAN_OK){
    Serial.println("Network is functioning!");
    return 1;
  } else {
    Serial.println("Network Error: check connections...");
    return 0;
  }  
}

/*********************************************************************************************************
** Function name:           checkNodes
** Descriptions:            Public function to ping all nodes to get a list of functioning nodes
** Returns:                 1 if completed successfully, 0 if error
*********************************************************************************************************/
int TTUCAN::checkNodes(void){
  if(nodeAddress != homeAddress){
	Serial.println("This function is reserved for the HOME Node only!");
	return 0;
  }
  Serial.println("Checking which nodes are functioning.  Type \"s\" to end function.");
  delay(1000);
  int currentTime, sentTime;
  INT32U ping_address = 0; //start with node 0
  INT32U ID, return_ID; 
  byte data[] = {0xFF};
  int TimeOut = 500;

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
	  ID = (((ping_address << 21)|(ping_address << 13))|0x00001FFF) | 0x80000000;
	  return_ID = (((homeAddress << 21)|(ping_address << 13))|0x00001FFF) | 0x80000000;
	}
    ID |= 0x40000000; //send as remote request
#ifdef ISOTP_COMM
  TxMsg.len=sizeof(data);
  TxMsg.Buffer=(uint8_t *)calloc(MAX_MSGBUF,sizeof(uint8_t));
  TxMsg.tx_id=ID;
  TxMsg.rx_id=return_ID;;
  memcpy(TxMsg.Buffer,data,sizeof(data));
  send(&TxMsg);
#else
	sendMsgBuf(ID, 1, data); //send msg 
#endif
    sentTime = millis();   
    //wait for response - need to decide timeout
    while(1){
	  if(!digitalRead(_mcp_int)){
        readMsgBuf(&rxId, &rxLen, rxBuffer);
	  }
      if(rxId == return_ID){
        sprintf(msgString, "** Node %d is functioning. **", ping_address);
        Serial.println(msgString);
        break; //end while loop
      }
      else{
        currentTime = millis() - sentTime; //check timeout
        if(currentTime >= TimeOut){ //5 second timeout to wait for return message
          sprintf(msgString, "Node %d is not functioning.", ping_address);
          Serial.println(msgString);
          break; //end while loop
        }
      }
    } //end while loop
    ping_address++; //move on to n_ext address
  } //end for loop

  return 1;
}

/*********************************************************************************************************
** Function name:           pingNode
** Descriptions:            ping one node directly to check if it is functioning
** Arguments:               node address to ping
** Returns:                 1 if sent successfully, 0 if error
*********************************************************************************************************/
int TTUCAN::pingNode(INT32U ping_address){
  if(nodeAddress != homeAddress){
	Serial.println("This function is reserved for the HOME Node only!");
	return 0;
  }
  sprintf(msgString, "Ping node: %d", ping_address);
  Serial.println(msgString);
	
  int currentTime, sentTime;
  INT32U ID, return_ID; 
  byte data[] = {0xFF};
  int TimeOut = 500;

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

  if(!_ext){ //standard message
    ID = ((((ping_address << 27)|(ping_address << 23))|0x00700000)>>20);
    return_ID = ((((homeAddress << 27)|(ping_address << 23))|0x00700000)>>20);
  }
  else{ //_extended message
    ID = (((ping_address << 21)|(ping_address << 13))|0x00001FFF) | 0x80000000;
    return_ID = (((homeAddress << 21)|(ping_address << 13))|0x00001FFF) | 0x80000000;
  }
  ID |= 0x40000000; //send as remote request
#ifdef ISOTP_COMM
  TxMsg.len=sizeof(data);
  TxMsg.Buffer=(uint8_t *)calloc(MAX_MSGBUF,sizeof(uint8_t));
  TxMsg.tx_id=ID;
  TxMsg.rx_id=return_ID;
  memcpy(TxMsg.Buffer,data,sizeof(data));
  send(&TxMsg);
#else
  sendMsgBuf(ID, 1, data); //send msg 
#endif
  sentTime = millis();   
  //wait for response - need to decide timeout
  while(1){
    if(!digitalRead(_mcp_int)){
      readMsgBuf(&rxId, &rxLen, rxBuffer);
	}
    if(rxId == return_ID){
      sprintf(msgString, "** Node %d is functioning. **", ping_address);
      Serial.println(msgString);
      return 1; //end while loop
    }
    else{
      currentTime = millis() - sentTime; //check timeout
      if(currentTime >= TimeOut){ //5 second timeout to wait for return message
        sprintf(msgString, "Node %d is not functioning.", ping_address);
        Serial.println(msgString);
        break; //end while loop
      }
    }
  } //end while loop
  return 0;
}
