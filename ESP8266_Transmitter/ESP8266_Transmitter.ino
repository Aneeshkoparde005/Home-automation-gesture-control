#include <WiFi.h>
#include <esp_now.h>

const int ledPin = 14;  // GPIO pin for LED

// PWM variables
const int pwmFreq = 5000;      // PWM frequency (5kHz)
const int pwmResolution = 8;   // 8-bit resolution (0-255)
const int pwmChannel = 0;      // Channel number (0 for simplicity)
int pwmValue = 0;              // PWM duty cycle value (0-255)

int deviceIndex = 0;
bool deviceStates[4] = {false, false, false, false};
int fanSpeed = 0;
int bulbBrightness = 0;

// Register addresses and values for PWM configuration
#define LEDC_TIMER 0
#define LEDC_CHANNEL 0
#define LEDC_RESOLUTION LEDC_TIMER_8_BIT
#define LEDC_SPEED LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_MAX 255  // Max duty cycle for 8-bit resolution (0-255)

// Set up the GPIO pin for PWM output manually
void manualLedSetup() {
  // Configure GPIO pin as output for PWM
  pinMode(ledPin, OUTPUT);

  // Set up PWM timer
  ledcSetup(pwmChannel, pwmFreq, pwmResolution); // Set frequency and resolution
  ledcAttachPin(ledPin, pwmChannel); // Attach PWM channel to GPIO pin
}

// Update PWM duty cycle manually
void manualLedUpdate(int dutyCycle) {
  // Make sure the duty cycle is within the allowable range
  dutyCycle = constrain(dutyCycle, 0, LEDC_DUTY_MAX);

  // Set PWM duty cycle manually
  ledcWrite(pwmChannel, dutyCycle);
}

// Update outputs based on gestures and states
void updateOutputs() {
  for (int i = 0; i < 4; i++) {
    if (i == 2) { // Bulb PWM brightness on GPIO14
      int pwmValue = map(bulbBrightness, 0, 100, 0, 255);
      manualLedUpdate(deviceStates[i] ? pwmValue : 0);  // Use PWM control for bulb
    } else {
      digitalWrite(i, deviceStates[i] ? HIGH : LOW);
    }
  }

  Serial.printf("Selected: %d | Fan Speed: %d | Bulb Brightness: %d\n", deviceIndex, fanSpeed, bulbBrightness);
}

// Handle gesture logic
void handleGesture(String gesture) {
  if (gesture == "LEFT") {
    deviceIndex = (deviceIndex + 3) % 4;
  } else if (gesture == "RIGHT") {
    deviceIndex = (deviceIndex + 1) % 4;
  } else if (gesture == "UP") {
    deviceStates[deviceIndex] = true;
  } else if (gesture == "DOWN") {
    deviceStates[deviceIndex] = false;
  } else if (gesture == "CW" && deviceIndex == 0) {
    fanSpeed = min(fanSpeed + 1, 3);
  } else if (gesture == "ACW" && deviceIndex == 0) {
    fanSpeed = max(fanSpeed - 1, 0);
  } else if (gesture == "CW" && deviceIndex == 2) {
    bulbBrightness = min(bulbBrightness + 10, 100);
  } else if (gesture == "ACW" && deviceIndex == 2) {
    bulbBrightness = max(bulbBrightness - 10, 0);
  }

  updateOutputs();
}

// ESP-NOW data receive handler
void onReceiveData(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
  String gesture = "";
  for (int i = 0; i < len; i++) {
    gesture += (char)incomingData[i];
  }

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

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  esp_now_register_recv_cb(onReceiveData);

  // Set up GPIO for devices and LED pins
  for (int i = 0; i < 4; i++) {
    pinMode(i, OUTPUT);
  }

  // Set up LED pin for PWM control
  manualLedSetup();
}

void loop() {
  // Nothing needed here
}
