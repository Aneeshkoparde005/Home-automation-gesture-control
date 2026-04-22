#include <WiFi.h>
#include <esp_now.h>

// Pin assignments
const int ledPins[4] = {2, 4, 5, 18};        // LEDs for device selection
const int devicePins[4] = {12, 13, 14, 27};  // Fan, Microwave, Bulb, Machine

int deviceIndex = 0;
bool deviceStates[4] = {false, false, false, false};
int fanSpeed = 0;            // Range: 0 to 3 (mapped to PWM)
int bulbBrightness = 0;      // Range: 0 to 100%

const uint32_t pwmFreq = 5000;
const uint8_t pwmRes = 8; // 0–255 range

// Update outputs based on current states
void updateOutputs() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], i == deviceIndex ? HIGH : LOW);

    if (i == 0) { // Fan speed with PWM
      int fanPWM = map(fanSpeed, 0, 3, 0, 255);
      ledcWrite(devicePins[i], deviceStates[i] ? fanPWM : 0);
    }
    else if (i == 2) { // Bulb brightness with PWM
      int bulbPWM = map(bulbBrightness, 0, 100, 0, 255);
      ledcWrite(devicePins[i], deviceStates[i] ? bulbPWM : 0);
    }
    else {
      digitalWrite(devicePins[i], deviceStates[i] ? HIGH : LOW);
    }
  }

  Serial.printf("Selected: %d | Fan Speed: %d | Bulb Brightness: %d\n", deviceIndex, fanSpeed, bulbBrightness);
}

// Gesture handler
void handleGesture(String gesture) {
  if (gesture == "LEFT") {
    deviceIndex = (deviceIndex + 3) % 4;
  } else if (gesture == "RIGHT") {
    deviceIndex = (deviceIndex + 1) % 4;
  } else if (gesture == "UP") {
    deviceStates[deviceIndex] = true;
  } else if (gesture == "DOWN") {
    deviceStates[deviceIndex] = false;
  } else if (gesture == "CW") {
    if (deviceIndex == 0) fanSpeed = min(fanSpeed + 1, 3);
    if (deviceIndex == 2) bulbBrightness = min(bulbBrightness + 10, 100);
  } else if (gesture == "ACW") {
    if (deviceIndex == 0) fanSpeed = max(fanSpeed - 1, 0);
    if (deviceIndex == 2) bulbBrightness = max(bulbBrightness - 10, 0);
  }

  updateOutputs();
}

// ESP-NOW callback
void onReceiveData(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
  String gesture = "";
  for (int i = 0; i < len; i++) gesture += (char)incomingData[i];

  Serial.print("From: ");
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           recvInfo->src_addr[0], recvInfo->src_addr[1], recvInfo->src_addr[2],
           recvInfo->src_addr[3], recvInfo->src_addr[4], recvInfo->src_addr[5]);
  Serial.println(macStr);

  Serial.println("Gesture Received: " + gesture);
  handleGesture(gesture);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  esp_now_register_recv_cb(onReceiveData);

  // Set pin modes
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(devicePins[i], OUTPUT);
  }

  // Attach PWM to fan (GPIO12) and bulb (GPIO14)
  if (!ledcAttach(devicePins[0], pwmFreq, pwmRes)) Serial.println("PWM failed for Fan");
  if (!ledcAttach(devicePins[2], pwmFreq, pwmRes)) Serial.println("PWM failed for Bulb");

  // Initialize PWM output to 0
  ledcWrite(devicePins[0], 0); // Fan
  ledcWrite(devicePins[2], 0); // Bulb
}

void loop() {
  // Nothing to do
}
