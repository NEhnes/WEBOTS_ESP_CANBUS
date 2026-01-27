#include <SPI.h>
#include <mcp2515.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLDE_SDA 21
#define OLDE_SCL 22
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum class Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  NEUTRAL
};

byte upArrow[8] = {
  0b00011000,
  0b00111100,
  0b01111110,
  0b11111111,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000
};

byte downArrow[8] = {
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000
};

byte leftArrow[8] = {
  0b00001000,
  0b00001100,
  0b00001110,
  0b11111111,
  0b11111111,
  0b00001110,
  0b00001100,
  0b00001000
};

byte rightArrow[8] = {
  0b10000000,
  0b11000000,
  0b11100000,
  0b11111111,
  0b11111111,
  0b11100000,
  0b11000000,
  0b10000000
};

struct can_frame canMsg;
MCP2515 mcp2515(5); // CS pin is GPIO 5

#define CAN_ACK_ID 0x037  // CAN ID for acknowledgment

void draw(Direction _dir);
Direction getInput(int VRX, int VRY);

void setup()
{
  Serial.begin(115200);

  Serial.println("Setup begin - RECEIVER");

  Serial.println("Initializing OLED...");
  Wire.begin(OLED_SDA, OLED_SCL);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { //0x3C is standard address
    Serial.println(F("SSD1306 allocation failed"));     // init OLED
    for(;;); // Loop forever if OLED fails
  } else {
    Serial.println("OLED initialized successfully");
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  Serial.println("Initializing MCP2515...");

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

      // Send acknowledgment
      canMsg.can_id  = CAN_ACK_ID;  // Use ACK ID
      canMsg.can_dlc = 0;           // No data needed for ACK
      mcp2515.sendMessage(&canMsg);
      Serial.println("ACK sent, ID: 0x037");
    }
  }

  Direction dir = getInput(VRX, VRY);

  draw(dir);

  delay(200);
}



void draw(Direction _dir){
  display.clearDisplay();

  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("ESP32 CAN BUS RECEIVER");

  switch (Direction)
  {
  case Direction::UP:
    display.drawBitmap(56, 16, upArrow, 16, 16, SSD1306_WHITE);
    break;
  case Direction::DOWN:
    display.drawBitmap(56, 32, downArrow, 16, 16, SSD1306_WHITE);
    break;
  case Direction::LEFT:
    display.drawBitmap(48, 24, leftArrow, 16, 16, SSD1306_WHITE);
    break;
  case Direction::RIGHT:
    display.drawBitmap(64, 24, rightArrow, 16, 16, SSD1306_WHITE);
    break;
  case Direction::NEUTRAL:
    display.drawCircle(64, 24, 8, SSD1306_WHITE);
  default:
    break;
  }

  display.display();
}

Direction getInput(int VRX, int VRY){

    // deadzone threshold
    const int DEADZONE = 200;
    const int CENTER = 512;
    
    int x = VRX - CENTER;
    int y = VRY - CENTER;
    
    // apply deadzone
    if (abs(x) < DEADZONE && abs(y) < DEADZONE) {
        return Direction::Neutral;
    }
    
    // Determine which axis has more movement
    if (abs(x) > abs(y)) {
        // Horizontal movement is dominant
        return (x > 0) ? Direction::Right : Direction::Left;
    } else {
        // Vertical movement is dominant
        return (y > 0) ? Direction::Up : Direction::Down;
    }
}