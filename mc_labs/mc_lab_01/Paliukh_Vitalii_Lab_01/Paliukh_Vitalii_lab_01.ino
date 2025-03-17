#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

#define SSID "TP-LINK_4408"
#define PASSWORD "26470467"

#define BTN_PIN 14  
#define LED1 12     
#define LED2 0      
#define LED3 2      
#define holdTime 150
#define ledInterval 500
#define debounceTime 20

uint32_t lastLEDUpdate = 0;
uint32_t buttonPressStartTime = 0;
uint8_t lastLED = 0;
bool virtualButtonPressed = false;

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px;}
    .button {
      padding: 10px 20px;
      font-size: 24px;
      text-align: center;
      outline: none;
      color: #fff;
      background-color: #2f4468;
      border: none;
      border-radius: 5px;
      box-shadow: 0 6px #999;
      cursor: pointer;
    }
    .button:hover { background-color: #1f2e45; }
    .button:active {
      background-color: #1f2e45;
      box-shadow: 0 4px #666;
      transform: translateY(2px);
    }
  </style>
</head>
<body>
  <h1>ESP Algorithm Stop</h1>
  <button class="button"
  onmousedown="sendRequest('press');"
  ontouchstart="sendRequest('press');" 
  onmouseup="sendRequest('release');"
  ontouchend="sendRequest('release');">HOLD</button>
  <script>
    function sendRequest(action) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/" + action, true);
      xhr.send();
    }
  </script>
</body>
</html>
)rawliteral";

void handlePress() {
  virtualButtonPressed = true;
}


void handleRelease() {
  virtualButtonPressed = false;
}

void pinSetup(){
  pinMode(BTN_PIN, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
}

void setup() {
  pinSetup();
  wifiSetup();
  endpointSetup();
  Serial.begin(115200);
}

void wifiSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  uint32_t startAttemptTime = millis();
  uint32_t checkInterval = 500; 
  uint32_t lastCheckTime = 0;

  while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < 15000)) {
    if (millis() - lastCheckTime >= checkInterval) {
      lastCheckTime = millis();
      Serial.println("Connecting to Wi-Fi...");
    }
    yield(); 
  }

  if (WiFi.status() == WL_CONNECTED) {
    server.begin();
    Serial.print("IP ESP8266: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("cant connect to wifi.");
  }
}

void endpointSetup(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/press", HTTP_GET, [](AsyncWebServerRequest *request) {
    handlePress();
    request->send(200, "text/plain", "Button pressed");
  });
  server.on("/release", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleRelease();
    request->send(200, "text/plain", "Button released");
  });
}

void loop() {
  bool physicalButtonState = (digitalRead(BTN_PIN) == HIGH);
  static bool lastPhysicalButtonState = false;

  if (physicalButtonState != lastPhysicalButtonState && (millis() - buttonPressStartTime) > debounceTime) {
    lastPhysicalButtonState = physicalButtonState;
    buttonPressStartTime = millis();
  }

  bool effectiveButtonPressed = virtualButtonPressed || physicalButtonState;

  if (effectiveButtonPressed && (millis() - buttonPressStartTime) >= holdTime) {
    while (virtualButtonPressed || physicalButtonState) {
      yield();
      physicalButtonState = (digitalRead(BTN_PIN) == HIGH);
      effectiveButtonPressed = virtualButtonPressed || physicalButtonState;
      Serial.println("presed");
    }
  }

  if (millis() - lastLEDUpdate >= ledInterval) {
    lastLEDUpdate = millis();

    switch (lastLED) {
      case 0:
        digitalWrite(LED1, HIGH);
        digitalWrite(LED3, LOW);
        lastLED = 1;
        break;
      case 1:
        digitalWrite(LED2, HIGH);
        digitalWrite(LED1, LOW);
        lastLED = 2;
        break;
      case 2:
        digitalWrite(LED3, HIGH);
        digitalWrite(LED2, LOW);
        lastLED = 0;
        break;
    }
  }
}
