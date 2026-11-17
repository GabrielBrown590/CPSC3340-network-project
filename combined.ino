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

#define REC_BUFF_SIZE 10
#define MAX_NODES 20


typedef struct Node{
    struct Node *next;
    struct Node *prev;

    uint16_t last_received; 
    bool has_received[REC_BUFF_SIZE];

    uint8_t  mac[6];
    uint32_t  rand_key;

}Node;

typedef struct Linked_List{
    Node *head;
    Node *tail;
    int size;
    
}List;


// Returns true if mac addresses are identical
bool mac_equal(uint8_t mac1[6], uint8_t mac2[6]){
    for (int i = 0; i<6; i++){
        if (mac1[i] != mac2[i]) return false;
    }
    return true;
}

// helper function for freeing nodes
void free_node{

}

void *create_node(msg_t msg, List *list){

    // Initializes all entries of has_received to false
    Node *new_node = calloc(sizeof(*new_node));

    // Check if malloc was successful
    if (!new_node){ 
        printf("malloc failed in create_node()\n");
        return NULL;
    }

    // remove tail node if list at capacity
    if (list->size == MAX_NODES){
        Node *temp = list->tail;
        list->tail = list->tail->prev;
        free(temp);
        list->size--;
    }

    // Assign node to specific ESP and boot cycle
    new_node->mac = msg.origin;
    new_node->rand_key = msg.rand_key;

    // Log message
    new_node->last_received = msg.seq;
    new_node->has_received[0] = true;

    // increment size of list
    list->size++;

    // Insert node at head of list
    new_node->next = list->head;
    list->head->prev = new_node;
    list_head = new_node;

    return;
}

// helper for moving node to front of list
void move_to_front(Node *curr, List *list){

    if (!curr || curr == list->head){
        return;
    }

    // update tail if necessary
    if (list->tail == curr){
        list->tail = curr->prev;
    }

    // remove curr from current position and fix gap
    curr->prev->next = curr->next;
    if (curr->next){
        curr->next->prev = curr->prev;
    }

    // Insert node at front of list
    curr->prev = NULL;
    curr->next = list->head;
    list->head->prev = curr->next;
    list->head = curr;
}

bool should_prop(msg_t message, Linked_List *list){
    //search for mac address in linked list
    Node *curr = list.head;
    
    while (curr){
        // Check if mac address already in list
        if (mac_equal(message.origin, curr->mac)){
            //check rand_key check received messages
            if (message.rand_key == curr->rand_key){
                // Node likely hasn't turned off since last received message
                if (message.seq>curr->last_received){
                    // Must shift last_received but definitely haven't received yet
                   int diff = message.seq-curr->last_received
                   if (diff >= REC_BUFF_SIZE){
                    curr->has_received[0] = true; 
                    for (int i = 1; i<REC_BUFF_SIZE; i++){
                        curr->has_received[i] = false;
                    }
                    // TODO: Need to move node to front of LRU list and return True
                   } // TODO: Need to handle else of actually shifting the has_received list

                                   // Move node to front of LRU
                    // Should be done whether or not rand_key of message origin matches key on record, right?
                    move_to_front(curr, list);

                   // Shift has_received list by message.seq-last_received (diff) positions
                   bool temp[REC_BUFF_SIZE] = {false};
                   int idx = 0; 

                   // Copy current bools in has_received into shifted positions
                   for (int i = diff; i<REC_BUFF_SIZE-1; i++){
                        temp[i] = curr->has_received[idx++];
                   } 
                   // Array was initialied to false, so after shift the only position requiring an update
                   temp[0] = true;

                   // This copying could also probably be implemented with memmove and memset, but maybe later

                   // Update list
                   curr->has_received = temp;

                   return true;
                }
                // TODO: Need to handle updating within last received list
                // TODO: Need to protect against integer overflow

                // What do we do if message.seq < last_received - REC_BUFF_SIZE?
                // Could we use that as an indication that message.seq has been reset to 0 (to avoid int overflow) and use it as last_received?
                // In code:
                if (message.seq < last_received - REC_BUFF_SIZE){
                //     curr->last_received = message.seq;
                //     memset(curr->has_received, 0, REC_BUFF_SIZE);  // reset list
                //     curr->has_received[0] = true;
                    return false;
                }
                // last_received - REC_BUFF_SIZE < message.seq < last_received (falls in range of has_received)
                // need to check has_received to see if message has already been received
                else {
                    int pos = curr->last_received - message.seq; 
                    if (curr->has_received[pos]) return false; // Message already received
                    // If not received, update has_received and return true
                    else{
                        curr->has_received[pos] = true;
                        return true;
                    }
                } 



            }
            //  message.seq < last_received - REC_BUFF_SIZE
            // else statement is for if origin esp has same random key as on record, indicating whether it's rebooted since it's last received message
            // If random key is different, rand_key needs to be updated, node needs to be moved to front, and has_received needs to be reset
            else {
                // TODO: Need to handle resetting the entry for the new random key (ESP32 has rebooted)
                curr->rand_key = message.rand_key; //udpate key

                // Move node to front (most recent received message)
                move_to_front(curr, list);

                // Update last_received and reset has_received[]
                curr->last_received = message.seq;
                memset(curr->has_received, 0, REC_BUFF_SIZE);  // reset list
                curr->has_received[0] = true;

            }


        }
        // Haven't seen mac address before
        // TODO: Need to add to front of list and update size and possilby pop last if overflow
        else {
            // Create node for newly discovered ESP using information from received message
            create_node(message, list);
        }
        curr = curr->next;
    }
}

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
