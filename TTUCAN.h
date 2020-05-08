//TTUCAN.h
#ifndef TTUCAN_H
#define TTUCAN_H

#include <mcp_can.h>
#include <TTU_IsoTp.h>
#include <mcp_can_dfs.h>
#include "Arduino.h"

//#define ISOTP_COMM
//#define TTU_DEBUG

class TTUCAN : protected IsoTp
{
	private:
		//Attributes
		INT32U nodeAddress;  //address of initialized node
		INT32U homeAddress;  //address of home node
		INT32U pingID;       //ID of received ping message
		INT32U nodeFilter;   //filter for receiving messages directed to this node 
		INT32U pingFilter;   //filter for receiving pingResponse
		INT32U pingResponse; //ID sent by node after receiving ping from home node
		INT8U numNodes;      //number of possible nodes
		INT32U networkStatusID; //ID of network ping message
		char rxByte = 0;       
		char msgString[128];
		
	public:
		//Methods
		TTUCAN(INT8U _CS, uint8_t mcp_int, INT32U _address, INT8U _ext);
		int TTU_begin(INT8U idmodeset = MCP_STDEXT, INT8U speedset = CAN_500KBPS, INT8U clockset = MCP_16MHZ);
		int addFilter(INT32U filterAddress, INT8U filterRegister);
		INT32U buildTransmitID(INT32U to_addr, INT32U descriptor);
		INT32U buildReceiveID(INT32U from_addr, INT32U descriptor);
		int send_Msg(INT32U msgID, INT8U rtr, INT8U *data, INT32U len);
		int receive_Msg(INT32U *id, INT8U *len, INT8U *buf);
		INT8U check_Receive(void);
		INT8U check_Error(void);
		/// Home Node Methods ///
		void homeMenu(void);
		void displayActivity(void);
		int networkStatus(void);
		int checkNodes(void);
		int pingNode(INT32U ping_address);
		//Attributes
		struct Message_t TxMsg, RxMsg; //these are public so users can access the data buffers
};
#endif
