#include <esp_now.h>
#include <WiFi.h> 
#include "esp_mac.h"
#include <queue>

#define TTLHOPS 5
#define CHANNEL 1
#define TIMETILLOUTOFNET 3 * 1000
const uint8_t allRecieverMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t broadcastPeer;
msg_t recievedMessage;

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

typedef struct __attribute__((packed)) {
  uint8_t  ver;       // protocol version; reject mismatches
  uint8_t  type;      // 0 = HEARTBEAT, 1 = BUTTON, 2 = DELIVERY REPORT (optional)
  uint8_t  origin[6]; // MAC of the pod that pressed the button (not the last relay)
  uint8_t  original_sender[6];
  uint32_t  rand_key;
  uint16_t seq;       // per-origin sequence number
  uint8_t  ttl;       // remaining hops
  uint8_t  r, g, b;   // origin's color
} msg_t;

void heartbeat()
{
  msg_t heartBeat = {};
  heartBeat.type = 0;
  WiFi.macAddress(heartBeat.origin);
  heartBeat.ttl = 0;

  esp_now_send(allRecieverMac, (uint8_t *) &heartBeat, sizeof(heartBeat));
  timeSentLastHeartBeat = millis();

}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
  if (len != sizeof(msg_t)) return; //throw out garbage packets
  if(info->rx_ctrl->rssi < -75) return;

  memcpy(&recievedMessage, incomingData, sizeof(recievedMessage));
  queue.push(recievedMessage);
}

void popQueue()
{
  msg_t q_msg;
  q_msg = queue.pop()
  if(q_msg.type == 0)
  {
    inNetwork = true;
    timeRecievedLastHeartBeat = millis();
    currState = INRANGE;
  } else if (q_msg.type == 1) {
    currState = MESSAGE;
    messageRed = q_msg.r;
    messageGreen = q_msg.g;
    messageBlue = q_msg.b;
  }
}

void declareBroadcastPeer()
{
  memcpy(broadcastPeer.peer_addr, allRecieverMac, 6);
  broadcastPeer.ifidx=WIFI_IF_STA;
  broadcastPeer.channel = CHANNEL;
  broadcastPeer.encrypt = false;
  if (esp_now_add_peer(&broadcastPeer) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}


msg_t curr_message;


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
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  if (esp_now_init() != ESP_OK) 
    {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
  esp_now_register_recv_cb(OnDataRecv); //tell esp which function to run on packet recieve.

  std::queue<msg_t> queue;

  timeRecievedLastHeartBeat = millis();
  timeSentLastHeartBeat = millis();
  
  declareBroadcastPeer();
  inNetwork = false;

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
