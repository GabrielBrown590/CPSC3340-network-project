// Hardware bring-up test for a pod: RGB status LED + push button.
//
// Wiring
//   Button: one leg to BUTTON_PIN, other leg to GND. No external resistor;
//           the internal pull-up holds the pin HIGH, so the button is ACTIVE LOW
//           (released = HIGH, pressed = LOW).
//   LED:    each color pin -> current-limiting resistor -> LED leg.
//           Common cathode: shared leg to GND (LED_COMMON_ANODE = false).
//           Common anode:   shared leg to 3V3 (LED_COMMON_ANODE = true).
//
// Behavior
//   Solid red when not connected, solid green when connected.
//   Each press prints "PRESSED" once and flashes white for FLASH_MS,
//   without blocking the loop.

const int RED_PIN = 21;
const int GREEN_PIN = 48;
const int BLUE_PIN = 47;
const int BUTTON_PIN = 2;

const int BUTTON_ACTIVE = LOW;        // active low (INPUT_PULLUP)
const bool LED_COMMON_ANODE = false;  // flip if the LED's shared leg goes to 3V3

const unsigned long DEBOUNCE_MS = 30;
const unsigned long FLASH_MS = 1000;

bool connected = false;

// Button debounce state
int lastReading = HIGH;          // raw reading from the previous loop
int stableState = HIGH;          // debounced state (HIGH = released)
unsigned long lastChangeMs = 0;  // when the raw reading last changed

// White flash state
bool flashing = false;
unsigned long flashStartMs = 0;

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  if (LED_COMMON_ANODE) {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

// Returns true exactly once per press (on the debounced HIGH -> LOW edge).
bool buttonJustPressed() {
  int reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  if (reading != lastReading) {
    lastReading = reading;
    lastChangeMs = now;
  }

  if (now - lastChangeMs >= DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    return stableState == BUTTON_ACTIVE;
  }
  return false;
}

void setup() {
  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  lastReading = stableState = digitalRead(BUTTON_PIN);
  setColor(255, 0, 0);  // start red (not connected)
}

void loop() {
  unsigned long now = millis();

  if (buttonJustPressed()) {
    Serial.println("PRESSED");
    flashing = true;
    flashStartMs = now;
  }

  if (flashing && now - flashStartMs >= FLASH_MS) {
    flashing = false;
  }

  if (flashing) {
    setColor(255, 255, 255);  // white
  } else if (connected) {
    setColor(0, 255, 0);      // green
  } else {
    setColor(255, 0, 0);      // red
  }
}
