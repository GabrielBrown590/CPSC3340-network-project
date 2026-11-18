#define RED 14
#define BLUE 13
#define GREEN 12
#define BUTTON_PIN 4

bool connected;

void colorLED(int r, int g, int b) {
    analogWrite(RED, r);
    analogWrite(GREEN, g);
    analogWrite(BLUE, b);
}

void setup() {
  Serial.begin(115200);
  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  connected = false;
  digitalWrite(RED, HIGH);
}

void loop() {

  if (connected) {
    colorLED(0, 255, 0);   // Green
  } else {
    colorLED(255, 0, 0);   // Red
  }

  int button_pressed = digitalRead(BUTTON_PIN);
  if (button_pressed) {
    Serial.println("PRESSED");
    colorLED(255, 255, 255);
    delay(1000);
  }
}

