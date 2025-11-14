// libs
#include <SPI.h>
#include <Arduino.h>
#include <mcp2515.h>

struct can_frame canMsg; // struct to hold CAN message
MCP2515 mcp2515(5); // SPI CS pin is GPIO 5

#define MAX_RETRIES 3
#define CAN_ACK_ID 0x037  // CAN ID for acknowledgment

void setup() {
  Serial.begin(115200);
  Serial.println("Setup begin - SENDER");
  SPI.begin();
  mcp2515.reset();

  MCP2515::ERROR result = mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  if (result != MCP2515::ERROR_OK) {
    Serial.println("ERROR: setBitrate failed!");
    Serial.println(result);
    while(1); // Halt
  } else {
    Serial.println("Bitrate set to 500kbps");
  }

  result = mcp2515.setNormalMode();
  if (result != MCP2515::ERROR_OK) {
    Serial.println("ERROR: setNormalMode failed!");
    Serial.println(result);
    while(1); // Halt
  } else {
    Serial.println("MCP2515 in Normal Mode");
  }

  Serial.println("Setup complete");
}

void loop() {

  int value = 1234; // Your integer value to send

  // Prepare CAN message
  canMsg.can_id  = 0x036;  // CAN ID
  canMsg.can_dlc = 2;      // Data length code (number of bytes)
  canMsg.data[0] = (value >> 8) & 0xFF; // MSB of value
  canMsg.data[1] = value & 0xFF;        // LSB of value

  bool messageSent = false;
  int retries = 0;

  while (!messageSent && retries < MAX_RETRIES) {
    
    if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
      Serial.print("Value sent: ");
      Serial.println(value);

      // Wait for acknowledgment
      unsigned long startTime = millis();
      bool ackReceived = false;
      
      while (millis() - startTime < 500) { // Wait up to 500ms for an ACK
        if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
          if (canMsg.can_id == CAN_ACK_ID) {
            ackReceived = true;
            break;
          }
        }
      }

      if (ackReceived) {
        Serial.println("ACK received");
        messageSent = true;
      } else {
        Serial.println("ACK not received, retrying...");
        retries++;
      }
    } else {
      Serial.println("Error sending message, retrying...");
      retries++;
    }
  }

  if (!messageSent) {
    Serial.println("Failed to send message after retries");
  }

  delay(1000); // Send data every second
}