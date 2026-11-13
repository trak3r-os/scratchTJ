#include <CapacitiveSensor.h>

#define faderPin A5
#define sendPin 10
#define sensePin 12

#define NUM_BUTTONS 4
const int buttonPins[NUM_BUTTONS] = {A0, A1, A2, A3};

CapacitiveSensor capSensor = CapacitiveSensor(sendPin, sensePin);

// Packet: [0xAA] [fHi] [fLo] [cHi] [cLo] [buttons] [checksum]
// 7 bytes. Checksum = XOR of bytes 1-5.
#define SYNC_BYTE 0xAA
#define HANDSHAKE_MAGIC 0x53  // 'S' for ScratchTJ

bool handshakeCompleted = false;

void setup() {
  for (int i = 0; i < NUM_BUTTONS; i++)
    pinMode(buttonPins[i], INPUT_PULLUP);

  Serial.begin(500000);
  capSensor.set_CS_AutocaL_Millis(0xFFFFFFFF);
}

unsigned long previousMillis = 0;
const unsigned long interval = 5;  // 200Hz

void loop() {
  if (Serial.available() > 0) {
    byte incoming = Serial.read();
    if (incoming == HANDSHAKE_MAGIC) {
      Serial.write('T');
      handshakeCompleted = true;
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    int faderValue = analogRead(faderPin);

    long rawCap = capSensor.capacitiveSensor(30);
    unsigned int capValue = (rawCap > 65535) ? 65535 : (rawCap < 0 ? 0 : (unsigned int)rawCap);

    // Buttons: bit 0=A0 .. bit 3=A3, 1=pressed (active low inverted)
    byte buttons = 0;
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (digitalRead(buttonPins[i]) == LOW)
        buttons |= (1 << i);
    }

    byte packet[7];
    packet[0] = SYNC_BYTE;
    packet[1] = (faderValue >> 8) & 0xFF;
    packet[2] = faderValue & 0xFF;
    packet[3] = (capValue >> 8) & 0xFF;
    packet[4] = capValue & 0xFF;
    packet[5] = buttons;
    packet[6] = packet[1] ^ packet[2] ^ packet[3] ^ packet[4] ^ packet[5];

    Serial.write(packet, 7);
  }
}
