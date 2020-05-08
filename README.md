# TTUCAN

## CAN bus protocol for TTU Vehicle Simulator.

This Library is compatible for the Arduino Mega 2560

This implementation requires MCP_CAN_lib and TTU_IsoTp:

MCP_CAN_lib: https://github.com/coryjfowler/MCP_CAN_lib

TTU_IsoTp: https://github.com/saidm5797/TTU_IsoTp

## General Guidelines:

To create a CAN object use the class constructor: 

 - TTUCAN(INT8U _CS, uint8_t mcp_int, INT32U _address, INT8U _ext) 

The arguments are the chip select pin (recommended pin 10), the interrupt pin (recommended pin 2), the address of the node you are initializing (0-15 for standard; 0-255 for extended), and the extension status (0 for standard; 1 for extended). It is important to make sure that the node's address and extension status match. It is also important that all nodes use the same extension status or else they will filter out all messages. 

 

Use the TTU_begin() function to initialize the node: 

- TTU_begin(INT8U idmodeset = MCP_STDEXT, INT8U speedset = CAN_500KBPS, INT8U clockset = MCP_16MHZ) 

This function works using default arguments, but other arguments can be used if desired. Look in the MCP_CAN_lib for options of other arguments. The begin function will set up all of the necessary filters automatically. To initialize other filters use the addFilter() function: 

 - addFilter(INT32U filterAddress, INT8U filterRegister) 

The arguments are the node address whose messages to receive, and the filter register to initialize (each node can only change filters 3, 4, and 5) 

 

It is up to the user to decide what message IDs to use for each type of information based on the guidelines described in the CAN Plan file. It is necessary to know what messages a node will be sending and receiving before runtime so that the data being transmitted can be processed accordingly. Use the **display_IDs** example to see a list of all message IDs that a node can send and receive from other nodes. This will help in planning communication between nodes. 

 

Other functions are available for getting message IDs. 

 - buildTransmitID(INT32U to_addr, INT32U descriptor) 

 - buildReceiveID(INT32U from_addr, INT32U descriptor) 

These functions take the node to send a message to or receive a message from, and the descriptor of the ID. They return the message IDs to be passed to the send_Msg() function or to be used in the preProcess() and postProcess() functions. 

 

After planning out the messages to be sent and received, the user should then modify the preProcess() and postProcess() functions.  

 - preProcess(byte *data, INT32U to_addr, INT32U descriptor) 

 - postProcess(byte *data, INT32U receiveID) 

The preProcess() function takes a data array to be filled, the node address to send to, and the data descriptor to be used. 

The postProcess() functions takes a data array to be filled, and the ID of the received message. 

These functions look at the IDs of messages to be sent or that have been received and then construct variables into a data array or deconstruct the received data into variables. It is important to use global variables as these functions do not take any variables as arguments. Use the **pre_post_process** and **three_node_comm** examples to see how these functions should be structured. 

  

To send a message use the send_Msg() function: 

 - send_Msg(INT32U msgID, INT8U rtr, INT8U *data, INT32U len) 

This function takes the message ID to use, the remote status (1 for remote message, 0 for non-remote message), the data array to send, and the length of the data in bytes (8 bytes in most cases). 

Before using the send_Msg() function, use the buildTransmitID() function or the preProcess() functions. These functions return a message ID that can be passed to the send_Msg() function. The preProcess() function will also fill the data array based on the node you plan to send to and the data descriptor used. 

 

To receive a message use the receive_Msg() function: 

 - receive_Msg(INT32U *id, INT8U *len, INT8U *buf) 

This function takes pointers to variables that hold the received message's ID and length, as well as a data array to fill with received data. This function should always be used within an if statement as shown here: 

 

>If(!digitalread(CAN0_int)){ 

>receive_Msg(&rxId, &len, rxBuffer) 

>} 

 

This makes use of the interrupt pin (named CAN0_int in this case) to make sure the controller only looks for messages when it has received one. Look at the **Send_and_Receive** example to see how this is done. 

 

After receiving a message, use the postProcess() function to update the variables in your program so that they can be used as normal.
