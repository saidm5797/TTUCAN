// CAN Home Node Example
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <TTU_IsoTp.h>
#include <SPI.h>
#include <TTUCAN.h>

#define CAN0_INT 2                              // Set INT to pin 2
TTUCAN CAN0(10, CAN0_INT, 15, 0);                               // Set CS to pin 10

void setup()
{
  Serial.begin(115200);
  pinMode(CAN0_INT, INPUT);                            // Configuring pin for /INT input
  CAN0.TTU_begin();
  CAN0.homeMenu();
}

void loop()
{   
  char msgString[128];                        // Array to store serial string
  String command = "";
  char rxByte = 0;

  while(1){
    if(Serial.available() > 0){
      rxByte = Serial.read(); //get one character
      if(rxByte != '\n'){
        command += rxByte;
      }
      else{
        //Serial.println(command);
        if(command == "display"){
          CAN0.displayActivity();
          command = "";
          CAN0.homeMenu();
        }
        else if(command == "network"){
          CAN0.networkStatus();
          command = "";
          CAN0.homeMenu();
        }
        else if(command == "nodes"){
          CAN0.checkNodes();
          command = "";
          CAN0.homeMenu();
        }
        
        else{
          INT32U address = command.toInt();
          if(command == "0" && address == 0){ //check that 0 is the intended node
            CAN0.pingNode(address);
          }
          else if(address == 0){ //command.toInt() will output zero if there's an error
            Serial.println("Invalid input!");
          }
          else{
            CAN0.pingNode(address);
          }
          command = "";
          CAN0.homeMenu();
        }
      }
    }
  }
}
