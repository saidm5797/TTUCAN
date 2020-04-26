# TTUCAN

CAN bus protocol for TTU Vehicle Simulator. This Library is compatible for the Arduino Mega 2560

This implementation requires MCP_CAN_lib and TTU_IsoTp:
MCP_CAN_lib: https://github.com/coryjfowler/MCP_CAN_lib
TTU_IsoTp: https://github.com/saidm5797/TTU_IsoTp

Main Functions:

TTUCAN(INT8U _CS, uint8_t mcp_int, INT32U _address, INT8U _ext)
//Class Constructor 
//takes chip select pin, interrupt pin, assigned node address (0-15 for standard, 0-255 for extended), extension (0 for standard, 1 for //extended

int TTU_begin(INT8U idmodeset = MCP_STDEXT, INT8U speedset = CAN_500KBPS, INT8U clockset = MCP_16MHZ)
//Initialize CAN object
//default arguments recommended

int addFilter(INT32U filterAddress, INT8U filterRegister)
//Allow a node to read messages addressed to another node
//takes address of node address whose messages to listen for, filter register to use (3, 4, or 5)

INT32U buildTransmitID(INT32U to_addr, INT32U descriptor)
//Build CAN ID of transmit message
//takes address to send message to, message data descriptor (0-7 for standard messages, 0-8192 for extended)

int send_Msg(INT32U msgID, INT8U rtr, INT8U *data, INT32U len)
//Send a message over CAN
//takes message ID, rtr (1 for remote frame, 0 for non-remote), data array, message length

int receive_Msg(INT32U *id, INT8U *len, INT8U *buf)
//Receive a message
//takes pointers to Id variable, message length variable, and data array
