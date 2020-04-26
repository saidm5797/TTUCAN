// Display CAN IDs Example

#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <TTU_IsoTp.h>
#include <SPI.h>
#include <TTUCAN.h>

long unsigned int rxId, txId;
char msgString[128];

#define CAN0_INT 2                              // Set INT to pin 2
TTUCAN CAN0(10, CAN0_INT, 15, 0);   //(CS to pin 10, interrupt pin, address, ext)

void setup()
{
  Serial.begin(115200);
  pinMode(CAN0_INT, INPUT);                            // Configuring pin for /INT input
  CAN0.TTU_begin();
  
  Serial.println();
  Serial.println("****************Message IDs sent by this node.****************");
  // all possible txId
  for(int to_addr=0; to_addr <= 15; to_addr++){
    sprintf(msgString, "Messages sent to node %1d:", to_addr);
    Serial.println(msgString);
    for(int descriptor=0; descriptor <= 7; descriptor++){
      txId = CAN0.buildTransmitID(to_addr, descriptor);
      sprintf(msgString, "Message sent with descriptor %1d: 0x%X", descriptor, txId);
      Serial.println(msgString);
    }
    Serial.println();
  }
  
  Serial.println();
  Serial.println("****************Message IDs received by this node.****************");
  // all possible rxId
  for(int from_addr=0; from_addr <= 15; from_addr++){
    sprintf(msgString, "Messages received from node %1d:", from_addr);
    Serial.println(msgString);
    for(int descriptor=0; descriptor <= 7; descriptor++){
      rxId = CAN0.buildReceiveID(from_addr, descriptor);
      sprintf(msgString, "Message received with descriptor %1d: 0x%X", descriptor, rxId);
      Serial.println(msgString);
    }
    Serial.println();
  }  
}

void loop()
{

}
