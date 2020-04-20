// CAN Home Node Example
//

#include <mcp_can.h>
#include <SPI.h>
#include <TTUCAN.h>

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];                        // Array to store serial string

#define CAN0_INT 2                              // Set INT to pin 2
TTUCAN CAN0(10, CAN0_INT, 15, 0);                               // Set CS to pin 10

void setup()
{
  Serial.begin(115200);
  pinMode(CAN0_INT, INPUT);                            // Configuring pin for /INT input
  CAN0.TTU_begin();
  CAN0.homeMenu();
}

byte data[8] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
String command = "";
char rxByte = 0;

void loop()
{ 
  if(Serial.available() > 0){
    rxByte = Serial.read(); //get one character
    if(rxByte != '\n'){
      command += rxByte;
    }
    else{
      //Serial.println(command);
      if(command == "display"){
        Serial.println("Displaying bus activity.  Type \"s\" to end function.");
        delay(1000);
        CAN0.displayActivity();
        command = "";
        CAN0.homeMenu();
      }
      else if(command == "network"){
        Serial.println("Checking network status.");
        CAN0.networkStatus(0);
        command = "";
        CAN0.homeMenu();
      }
      else if(command == "nodes"){
        Serial.println("Checking which nodes are functioning.  Type \"s\" to end function.");
        delay(1000);
        CAN0.checkNodes(0);
        command = "";
        CAN0.homeMenu();
      }
      
      else{
        INT32U address = command.toInt();
        if(command == "0" && address == 0){ //check that 0 is the intended node
          sprintf(msgString, "Ping node: %d", address);
          Serial.println(msgString);
          CAN0.pingNode(address, 0);
        }
        else if(address == 0){ //command.toInt() will output zero if there's an error
          Serial.println("Invalid input!");
        }
        else{
          sprintf(msgString, "Ping node: %d", address);
          Serial.println(msgString);
          CAN0.pingNode(address, 0);
        }
        command = "";
        CAN0.homeMenu();
      }
    }
  }
}
