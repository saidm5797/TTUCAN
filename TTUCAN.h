//TTUCAN.h
//
#ifndef TTUCAN_H
#define TTUCAN_H

#include <mcp_can.h>
#include <iso-tp-derived.h>
#include "Arduino.h"

//#define ISOTP_COMMUNICATION


class TTUCAN : protected IsoTp
{
	private:
		//Attributes
		INT32U nodeAddress;
		INT8U _ext;
		INT32U homeAddress;
		INT32U pingID;
		INT32U pingResponse;
		INT8U numNodes;
		INT32U networkStatusID;
		struct Message_t TxMsg, RxMsg;
		char rxByte = 0;
		char msgString[128];
		
	public:
		//Methods
		TTUCAN(INT8U _CS, uint8_t mcp_int, INT32U _address, INT8U _ext)
		int TTU_begin();
		int addFilter(INT32U filterAddress, INT8U _ext, INT8U filterRegister);
		INT32U buildMsgID(INT32U to_addr, INT32U descriptor);
		int send_Msg(INT32U to_addr, INT8U rtr, INT32U descriptor, INT8U *data, INT32U len);
		int send_Msg(INT32U msgID, INT8U rtr, INT8U *data, INT32U len);
		int receive_Msg(INT32U *id, INT8U *len, INT8U *buf);
		INT8U check_Receive();
		INT8U check_Error();
		/// Home Node Methods ///
		void homeMenu();
		void displayActivity();
		int networkStatus();
		int checkNodes();
		int pingNode(INT32U ping_address);
		//Attributes
		
}