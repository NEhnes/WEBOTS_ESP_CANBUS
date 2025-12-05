#include <SPI.h>
#include <mcp2515.h>
#include <Arduino.h>
// #include <Adafruit_SSD1306.h> // OLED display library

// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64
// #define OLED_RESET -1
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct can_frame canMsg;
MCP2515 mcp2515(5); // CS pin is GPIO 5

#define CAN_ACK_ID 0x037  // CAN ID for acknowledgment

void setup()
{
  Serial.begin(115200);
  Serial.println("Setup begin - RECEIVER");

  // display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // display.clearDisplay();
  // display.setTextColor(WHITE);

  SPI.begin();

  mcp2515.reset();

  Serial.println(mcp2515.checkError());

  MCP2515::ERROR result = mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  if (result != MCP2515::ERROR_OK) {
    Serial.print("ERROR: setBitrate failed! --- ");
    Serial.println(result);
    while(1); // Halt
  }

  result = mcp2515.setNormalMode();
  if (result != MCP2515::ERROR_OK) {
    Serial.println("ERROR: setNormalMode failed!");
    Serial.println(result);
    while(1); // Halt
  }
  
  Serial.println("Setup complete");
}


void loop()
{
  Serial.println("Waiting for CAN message...");
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK)
  {
    Serial.print("Message received with ID: 0x");
    Serial.println(canMsg.can_id, HEX);
    if (canMsg.can_id == 0x036)  // Check if the message is from the sender
    {
      // // legacy from original example code
      // int value = (canMsg.data[0] << 8) | canMsg.data[1]; // Combine MSB and LSB
      // Serial.print("Value Received: ");
      // Serial.println(value);

      // here's my new shi for joystick
      int VRX, VRY;
      memcpy(&VRX, &canMsg.data[0], sizeof(VRX)); // Copy data bytes 0-3 into VRX
      memcpy(&VRY, &canMsg.data[4], sizeof(VRY)); // Copy data bytes 4-7 into VRY

      Serial.print("VRX: ");
      Serial.print(VRX); 
      Serial.print(" | VRY: ");
      Serial.println(VRY);
      Serial.println("---------------------");

      // display.clearDisplay();

      // display.setTextSize(1);
      // display.setCursor(25, 10);
      // display.print("Received: ");

      // display.setTextSize(2);
      // display.setCursor(25, 30);
      // display.print(value);

      // display.display();

      // Send acknowledgment
      canMsg.can_id  = CAN_ACK_ID;  // Use ACK ID
      canMsg.can_dlc = 0;           // No data needed for ACK
      mcp2515.sendMessage(&canMsg);
      Serial.println("ACK sent, ID: 0x037");
    }
  }
  delay(200);
}