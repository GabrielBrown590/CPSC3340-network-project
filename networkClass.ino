#include <WiFi.h>
#include <esp_now.h>
#include <vector>

#define TTLHOPS 50
#define CHANNEL 1
#define TIMETILLOUTOFNET 3 * 1000
const uint8_t allRecieverMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t broadcastPeer;


typedef struct __attribute__((packed)) {
  uint8_t  ver;       // protocol version; reject mismatches
  uint8_t  type;      // 0 = HEARTBEAT, 1 = BUTTON, 2 = DELIVERY REPORT (optional)
  uint8_t  origin[6]; // MAC of the pod that pressed the button (not the last relay)
  uint8_t  originalSender[6];
  uint8_t randKey;
  uint16_t seq;       // per-origin sequence number
  uint8_t  ttl;       // remaining hops
  uint8_t  r, g, b;   // origin's color
} msg_t;

msg_t recievedMessage;
int inNetwork;
uint timeRecievedLastHeartBeat;
uint timeSentLastHeartBeat;

void heartbeat()
{
  msg_t heartBeat = {};
  heartBeat.type = 0;
  WiFi.macAddress(heartBeat.origin);
  heartBeat.ttl = TTLHOPS;

  esp_now_send(allRecieverMac, (uint8_t *) &heartBeat, sizeof(heartBeat));
  timeSentLastHeartBeat = millis();

}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
  if (len != sizeof(msg_t)) return; //throw out garbage packets
  if(info->rx_ctrl->rssi < -75) return;

  memcpy(&recievedMessage, incomingData, sizeof(recievedMessage));
  if(recievedMessage.type == 0)
  {
    inNetwork = true;
    timeRecievedLastHeartBeat = millis();
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
  timeRecievedLastHeartBeat = millis();
  timeSentLastHeartBeat = millis();
  
  declareBroadcastPeer();
  inNetwork = false;
}

void loop() {
  //check if we're still in network
  if(millis()-timeRecievedLastHeartBeat > TIMETILLOUTOFNET)
  {
    inNetwork = false;
  }
  //check if we need to send heartbeat
  if(millis()-timeSentLastHeartBeat > 1000)
  {
    heartbeat();
  }

}
