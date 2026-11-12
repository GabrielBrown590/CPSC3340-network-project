#include <esp_now.h>
#include <WiFi.h> 

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

msg_t curr_message;

#define REC_BUFF_SIZE 10
#define MAX_NODES 20


typedef struct Node{
    struct *Node next;
    struct *Node prev;

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

bool mac_equal(uint8_t mac1[6], uint8_t mac2[6]){
    for (int i = 0; i<6; i++){
        if (mac1[i] != mac2[i]) return false;
    }
    return true;
}



bool should_prop(msg_t message, Linked_List list){
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
                }
                // TODO: Need to handle updating within last received list
                // TODO: Need to protect against integer overflow
            }
            // No
            else{
                // TODO: Need to handle resetting the entry for the new random key (ESP32 has rebooted)
            }


        }
        curr = curr->next;
    }
    // Haven't seen mac address before
    // TODO: Need to add to front of list and update size and possilby pop last if overflow

}





void OnDataRecv(const esp_now_rec_info *recv_info, const uint_8 *incomingData, int len){

    memcpy(&curr_message, incomingData, sizeof(curr_message));

}

