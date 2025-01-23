#include "Mouse.h"

// Pin definitions (xiao rp2350)
#define CLK_PIN 27  // D1
#define DATA_PIN 28 // D2
#define MOT_PIN 5   // D3

// Write 1 byte using 2MHz SPI, mode 2, bit-banging
void writeSPI(uint8_t address, uint8_t data) {
  address |= 0x80;

  // Send address
  for (int i = 7; i >= 0; i--) {
    digitalWrite(DATA_PIN, (address >> i) & 0x01); // Send from MSB
    digitalWrite(CLK_PIN, LOW);                    // Clock Low
    delayMicroseconds(0.25);     // 1MHz clock (T=1µs → half period 0.5µs)
    digitalWrite(CLK_PIN, HIGH); // Clock High
    delayMicroseconds(0.25);
  }

  // Send data
  for (int i = 7; i >= 0; i--) {
    digitalWrite(DATA_PIN, (data >> i) & 0x01); // Send from MSB
    digitalWrite(CLK_PIN, LOW);                 // Clock Low
    delayMicroseconds(0.25);
    digitalWrite(CLK_PIN, HIGH); // Clock High
    delayMicroseconds(0.25);
  }

  digitalWrite(DATA_PIN, HIGH);
}

// Read 1 byte using 2MHz SPI, mode 2, bit-banging
uint8_t readSPI(uint8_t address) {
  address &= 0x7F;
  uint8_t data = 0;

  // Send address
  for (int i = 7; i >= 0; i--) {
    digitalWrite(DATA_PIN, (address >> i) & 0x01); // Send from MSB
    digitalWrite(CLK_PIN, LOW);                    // Clock Low
    delayMicroseconds(0.25);
    digitalWrite(CLK_PIN, HIGH); // Clock High
    delayMicroseconds(0.25);
  }

  // Receive data
  pinMode(DATA_PIN, INPUT); // Switch data pin to input mode
  delayMicroseconds(1);

  for (int i = 7; i >= 0; i--) {
    digitalWrite(CLK_PIN, LOW); // Clock Low
    delayMicroseconds(0.25);
    digitalWrite(CLK_PIN, HIGH);          // Clock High
    data |= (digitalRead(DATA_PIN) << i); // Receive from MSB
    delayMicroseconds(0.25);
  }
  pinMode(DATA_PIN, OUTPUT); // Switch data pin back to output mode
  digitalWrite(DATA_PIN, HIGH);

  return data;
}

void setup() {
  // Set pin modes
  pinMode(CLK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(MOT_PIN, INPUT);

  digitalWrite(CLK_PIN, HIGH); // Initial clock state (High when idle)

  // initialize mouse control:
  Mouse.begin();

  uint8_t pid0 = readSPI(0x00);
  uint8_t pid1 = readSPI(0x01);

  // Assert reset
  writeSPI(0x06, 0xD1);

  delay(35);

  // Enable 12bit mode
  //  writeSPI(0x09, 0x5A);
  //  writeSPI(0x19, 0x04);
  //  writeSPI(0x09, 0x00);

  // Wakeup from sleep
  writeSPI(0x06, 0x11);

  delay(35);

  // Deassert reset
  writeSPI(0x05, 0xB9);

  delay(35);

  // Read some register
  readSPI(0x02);
  readSPI(0x03);
  readSPI(0x04);
  readSPI(0x12);

  // Reset bus state
  digitalWrite(CLK_PIN, LOW);
  delay(1);
  digitalWrite(CLK_PIN, HIGH);
  delay(2);
}

void loop() {
  static unsigned long last_send_time;
  static unsigned long last_read_time;

  // if (digitalRead(MOT_PIN) == 0) {
  unsigned long current_time = millis();
  int8_t buf[4];
  buf[0] = readSPI(0x02);
  buf[1] = readSPI(0x03);
  buf[2] = readSPI(0x04);

  // // Read upper 4bit of XY data
  // buf[3] = readSPI(0x12);

  // Check motion flag
  if (buf[0] & 0x80) {
    Serial.printf("%4d, %4d, %d\n", buf[1], buf[2],
                  current_time - last_read_time);
    last_send_time = current_time;

    Mouse.move(buf[1], buf[2], 0);
  }

  last_read_time = current_time;
  // }
}