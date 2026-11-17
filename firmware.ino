// This file will contain functions about what the color of its message LED is, the state machine, logging, etc.
#include <WiFi.h>
#include "esp_mac.h"

typedef struct __attribute__((packed)) {
  uint8_t  ver;       // protocol version; reject mismatches
  uint8_t  type;      // 0 = HEARTBEAT, 1 = BUTTON, 2 = DELIVERY REPORT (optional)
  uint8_t  origin[6]; // MAC of the pod that pressed the button (not the last relay)
  uint16_t seq;       // per-origin sequence number
  uint8_t  ttl;       // remaining hops
  uint8_t  r, g, b;   // origin's color
} msg_t;

// the rgb values of this pod as an originator
int originRed = 0;
int originGreen = 0;
int originBlue = 0;

// the rgb values of message
int messageRed = 0;
int messageGreen = 0;
int messageBlue = 0;

const int redPin = 16;
const int greenPin = 17;
const int bluePin = 18;

const int buttonPin = 15;

enum LED_STATE {INRANGE, OUTOFRANGE, MESSAGE};

LED_STATE currState = INRANGE;



void rgbFromMAC() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  originRed = (int)mac[3];
  originGreen = (int)mac[4];
  originBlue = (int)mac[5];
  Serial.println(originRed);
  Serial.println(originGreen);
  Serial.println(originBlue);
}

void setColor(int r, int g, int b) {
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
}


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  rgbFromMAC();

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  pinMode(buttonPin, INPUT_PULLUP); 
}

void loop() {

  int buttonState = digitalRead(buttonPin);
  Serial.println(buttonState);
  if (buttonState == LOW) {
    // send message here
  }
  if (currState == INRANGE) {
    setColor(0, 255, 0);
    // look for incoming messages from queue here (if nonempty). Otherwise, become OUTOFRANGE.
    currState = OUTOFRANGE;
  } else if (currState == OUTOFRANGE) {
    setColor(255, 0, 0);
    // look for heartbeat here
    currState = INRANGE;
    
  } else if (currState == MESSAGE) {
    // determine color of incoming message rgb (blue for now)

    setColor(0, 0, 255);
    delay(1000);
    currState = INRANGE;
  }
}
