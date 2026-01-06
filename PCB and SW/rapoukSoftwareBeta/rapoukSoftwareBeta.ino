//Suk Industries Software Development
//Tato verze je verze z CD k DMP
//Jedná se o beta verzi základní funkce projektu Rapouk a proto všechny funkce nefungují správně
//Základní princtip DMP je funkční

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_VL6180X.h>

const char* ssid = "softAP-ESP32";
const char* password = "123456789";

Servo SF0, SF1, SF2, SF3, S0, S1, S2, S3;

#define LED 2
#define maxConnection 1
#define ADC 34
#define stopDelay 10

int lastStationCount=-1;
int defaultAngle=80;
int add=30;
int beginAngle0=0;
int beginAngle1=180-beginAngle0;
int stopAngle0=65;
int stopAngle1=180-stopAngle0;
int rightAngle0=120;
int rightAngle1=180-rightAngle0;

uint8_t setSpeed;
uint16_t highValue = 0;
uint16_t lowValue = 4095;
uint32_t sum = 0;
uint8_t percent = 0;
unsigned int i = -1;

bool tempomat;
bool setupCore;
bool moving;

String IP;

AsyncWebServer server(80);
Adafruit_VL6180X sensor = Adafruit_VL6180X();

unsigned long previousMillis = 0;

enum Animation {HOME, FORWARD, BACKWARD, TURNLEFT, TURNRIGHT, ROTATELEFT, ROTATERIGHT, HELLO, PUSHUP, WAVE, ANIMATION0, ANIMATION1, ANIMATION2, LIE, OFF};
Animation currentAnimation = LIE;
int lastAnimation;

const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="cz">
<head>
    <meta charset="UTF-8">
    <title>Ovladání Rapouka</title>
    <style>
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 80vh;
            width: 95vw;
            background-color: #303134;
            font-family: 'Helvetica', 'Arial', sans-serif;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            -webkit-tap-highlight-color: transparent;
            touch-action: none;
        }

        .container {
            display: flex;
            justify-content: space-between;
            width: 100%;
            padding: 0 20px;
        }

        .controller, .animations {
            display: grid;
            grid-gap: 10px;
            padding: 20px;
            background-color: #4c4c4c;
            border-radius: 10px;
            box-shadow: 0 0 10px #00000033;
        }

        .controller {
            grid-template-areas: 
                "rotateLeft moveForward rotateRight"
                "turnLeft home turnRight"
                ". moveBackward .";
            justify-items: center;
            align-items: center;
        }

        .animations {
            grid-template-areas: 
                "animationHello animationPushup animationWave"
                "animationCustom0 animationCustom1 animationCustom2";
            justify-items: center;
            align-items: center;
        }

        .settings {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            text-align: center;
            padding: 15px;
            background-color: #4c4c4c;
            border-radius: 10px;
            box-shadow: 0 0 10px #00000033;
        }

        .settings .setting-item {
            display: flex;
            align-items: center;
            justify-content: center;
            margin: 20px 0;
            font-size: x-large;
            accent-color: #af0808;
        }

        .settings .setting-item p {
            margin: 0;
            margin-right: 10px;
        }
        
        button {
            width: 100px;
            height: 100px;
            font-size: xx-large;
            cursor: pointer;
            border: none;
            border-radius: 10px;
            box-shadow: 0 2px 5px #00000033;
            transition: background-color 0.3s ease;
            color: white;
        }

        button:active {
            transform: scale(0.95);
        }

        .moveForward {grid-area: moveForward; background-color: #7C0A02;}
        .turnLeft {grid-area: turnLeft; background-color: #7C0A02;}
        .home {grid-area: home; background-color: #7C0A02;}
        .turnRight {grid-area: turnRight; background-color: #7C0A02;}
        .moveBackward {grid-area: moveBackward; background-color: #7C0A02;}

        .rotateLeft {grid-area: rotateLeft; background-color: #af0808;}
        .rotateRight {grid-area: rotateRight; background-color: #af0808;}

        .animationHello {background-color: #7C0A02; font-size: 275%;}
        .animationPushup {background-color: #7C0A02; font-size: 275%;}
        .animationWave {background-color: #7C0A02; font-size: 275%;}
        .animationCustom0 {background-color: #7C0A02; font-size: larger;}
        .animationCustom1 {background-color: #7C0A02; font-size: larger;}
        .animationCustom2 {background-color: #7C0A02; font-size: larger;}

        #content {
            display: none;
        }

        #warning {
            display: none;
            margin: 10%;
            text-align: center;
            font-size: 50px;
            color: #f70d1a;
        }

        @media (orientation: landscape) {
            #content {
                display: block;
            }

            #warning {
                display: none;
            }
        }

        @media (orientation: portrait) {
            #content {
                display: none;
            }

            #warning {
                display: block;
            }
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }

        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #303134;
            -webkit-transition: .4s;
            transition: .4s;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: #af0808;
            -webkit-transition: .4s;
            transition: .4s;
        }

        input:checked + .slider {
            background-color: #7C0A02;
        }

        input:focus + .slider {
            box-shadow: 0 0 3px #00000033;
        }

        input:checked + .slider:before {
            -webkit-transform: translateX(26px);
            -ms-transform: translateX(26px);
            transform: translateX(26px);
        }

        .slider.round {
            border-radius: 34px;
        }

        .slider.round:before {
            border-radius: 50%;
        }
    </style>
</head>
<body>
    <div id="content">
        <div class="container">
            <div class="controller">
                <button class="rotateLeft" data-command="rotateLeft">↺</button>
                <button class="moveForward" data-command="moveForward">↑</button>
                <button class="rotateRight" data-command="rotateRight">↻</button>
                <button class="turnLeft" data-command="turnLeft">←</button>
                <button class="home" data-command="home">■</button>
                <button class="turnRight" data-command="turnRight">→</button>
                <button class="moveBackward" data-command="moveBackward">↓</button>
            </div>
            <div class="settings">
                <div class="setting-item">
                    <p class="IP">IP adresa<br><span id="IPAdress">unknown</span></p>
                </div>
                <div class="setting-item">
                    <p>Aktuální % baterie<br><span id="batteryStatus">unknown</span></p>
                </div>
                <div class="setting-item">
                    <p>Tempomat</p>
                    <label class="switch">
                        <input type="checkbox" data-command="autopilot">
                        <span class="slider round"></span>
                    </label>
                </div>
                <div>
                    <p class="setting-item">Rychlost</p>
                    <input class="setting-item" style="width: 180px;" type="range" min="0" max="50" value="10" data-command="setSpeed">
                </div>
            </div>
            <div class="animations">
                <button class="animationHello" data-command="animationHello">👋</button>
                <button class="animationPushup" data-command="animationPushup">🖱️</button>
                <button class="animationWave" data-command="animationWave">🌊</button>
                <button class="animationCustom0" data-command="animationCustom0">Custom 0</button>
                <button class="animationCustom1" data-command="animationCustom1">Custom 1</button>
                <button class="animationCustom2" data-command="animationCustom2">Custom 2</button>
            </div>
        </div>
    </div>
    <div id="warning">
        <p>Otoč si zařízení na šířku!</p>
        <p>Tohle není Instagram 😆</p>
    </div>
    <script>
        let isBusy = false;
        let longPressTimer = null;
        let lastOffCommand = null;
        let autopilotEnabled = false;

        async function sendRequest(url) {
            try {
                const response = await fetch(url);
                if (!response.ok) throw new Error("Loading error: " + response.status);
                return await response.text();
            } catch (error) {}
        }

        async function toggleCheckbox(command, value) {
            if (isBusy && command !== "off") return;
            if (autopilotEnabled && !["batteryStatus", "autopilotOn", "setSpeed"].includes(command)) return;

            let url = `/${command}${value !== undefined ? `?value=${value}` : ""}`;

            if (command === "off" && lastOffCommand === url) return;
            lastOffCommand = command === "off" ? url : null;

            isBusy = true;
            await sendRequest(url);
            isBusy = false;
        }

        function handleLongPress(command, value) {
            longPressTimer = setInterval(() => {
                if (!isBusy) toggleCheckbox(command, value);
            }, 1000);
        }

        function cancelLongPress() {
            clearInterval(longPressTimer);
            longPressTimer = null;
        }

        function setupButtons() {
            document.querySelectorAll("[data-command]").forEach(button => {
                const command = button.getAttribute("data-command");
                const value = button.getAttribute("data-value");

                if (button.type === "range") {
                    button.addEventListener("input", () => toggleCheckbox(command, button.value));

                    if (command === "setSpeed") {
                        toggleCheckbox(command, button.value);
                    }

                } else {
                    const pressHandler = () => {
                        cancelLongPress();
                        toggleCheckbox(command, value);
                        handleLongPress(command, value);
                    };
                    const releaseHandler = () => {
                        cancelLongPress();
                        if (command !== "setSpeed") toggleCheckbox("off");
                    };

                    button.addEventListener("touchstart", pressHandler);
                    button.addEventListener("touchend", releaseHandler);
                    button.addEventListener("mousedown", pressHandler);
                    button.addEventListener("mouseup", releaseHandler);
                }
            });
        }

        async function updateElementContent(url, elementId, suffix = "") {
            const response = await sendRequest(url);
            if (response !== undefined) document.getElementById(elementId).innerText = response + suffix;
        }

        async function updateBatteryStatus() {
            const response = await sendRequest("/batteryStatus");
            if (response !== undefined) {
                const batteryStatus = parseInt(response);
                const batteryElement = document.getElementById("batteryStatus");
                batteryElement.innerText = batteryStatus + "%";
                batteryElement.style.color = batteryStatus <= 20 ? "red" : "black";
            }
        }

        function lockOrientation() {
            if (screen.orientation) {
                screen.orientation.lock("landscape").catch(() => {});
            }
        }

        function setupAutopilotSwitch() {
            const autopilotSwitch = document.querySelector("[data-command='autopilot']");
            if (autopilotSwitch) {
                autopilotSwitch.addEventListener("change", () => {
                    autopilotEnabled = autopilotSwitch.checked;
                    toggleCheckbox(autopilotEnabled ? "autopilotOn" : "autopilotOff");
                });
            }
        }

        window.addEventListener("load", () => {
            lockOrientation();
            updateElementContent("/IPAdress", "IPAdress");
            updateBatteryStatus();
            setInterval(updateBatteryStatus, 6000);
            setupAutopilotSwitch();
            setupButtons();
        });
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.printf("Configuring Setup on core: %d\n", xPortGetCoreID());
  setupCore = xPortGetCoreID();
  Serial.println("Configuring AP");
  WiFi.mode(WIFI_AP);
  IPAddress localIP(192, 168, 4, 1);
  IPAddress gatewayIP(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  if (!WiFi.softAPConfig(localIP, gatewayIP, subnet) || !WiFi.softAP(ssid, password, 1, 0, maxConnection)) {
    Serial.println("AP Config Failed");
    return;
  }
  IP = WiFi.softAPIP().toString().c_str();
  Serial.printf("AP IP address: %s\n", IP);
  pinMode(LED, OUTPUT);
  Serial.print("VL6180X Test - ");
  if (!sensor.begin()) {
    digitalWrite(LED, HIGH);
    Serial.println("Failed");
    return;
  }
  digitalWrite(LED, LOW);
  Serial.println("Passed");
  Serial.print("Configuring animations on core: ");
  xTaskCreatePinnedToCore(animations, "animations", 2048, NULL, 0, NULL, 0);

  SF0.attach(23);
  S0.attach(16);
  SF1.attach(27);
  S1.attach(17);
  SF2.attach(26);
  S2.attach(18);
  SF3.attach(25);
  S3.attach(19);

  server.on("/", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(204);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
  });
  server.on("/setSpeed", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      setSpeed = value.toInt();
      Serial.printf("setSpeed value recieved: %d\n", setSpeed);
      }
    request->send(200, "text/plain", "ok");
  });
  server.on("/IPAdress", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("IPAdress received");
    request->send(200, "text/plain", IP);
  });
  server.on("/batteryStatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (i == -1) {
      for (i = 0; i <= 9; i++) {
      voltmeterMeasuring();
      vTaskDelay(25 / portTICK_PERIOD_MS);
      }
      voltmeterCalc();
    } else if (i <= 9) {
      voltmeterMeasuring();
      i++;
    } else {
      voltmeterCalc();
    }
    request->send(200, "text/plain", String(percent));
  });
  server.on("/autopilotOn", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("Tempomat On");
    tempomat = true;
    request->send(200, "text/plain", "ok");
  });
  server.on("/autopilotOff", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("Tempomat Off");
    tempomat = false;
    request->send(200, "text/plain", "ok");
  });
  server.on("/home", HTTP_GET, [] (AsyncWebServerRequest *request) {
     Serial.println("home received");
    currentAnimation = HOME;
    request->send(200, "text/plain", "ok");
  });
  server.on("/moveForward", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("moveForward received");
    currentAnimation = FORWARD;
    request->send(200, "text/plain", "ok");
  });
  server.on("/moveBackward", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("moveBackward received");
    currentAnimation = BACKWARD;
    request->send(200, "text/plain", "ok");
  });
  server.on("/turnLeft", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("turnLeft received");
    currentAnimation = TURNLEFT;
    request->send(200, "text/plain", "ok");
  });
  server.on("/turnRight", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("turnRight received");
    currentAnimation = TURNRIGHT;
    request->send(200, "text/plain", "ok");
  });
  server.on("/rotateLeft", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("rotateLeft received");
    currentAnimation = ROTATELEFT;
    request->send(200, "text/plain", "ok");
  });
  server.on("/rotateRight", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("rotateRight received");
    currentAnimation = ROTATERIGHT;
    request->send(200, "text/plain", "ok");
  });
  server.on("/animationHello", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("animationHello received");
    currentAnimation = HELLO;
    request->send(200, "text/plain", "ok");
  });
  server.on("/animationPushup", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("animationPushup received");
    currentAnimation = PUSHUP;
    request->send(200, "text/plain", "ok");
  });
  server.on("/animationWave", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("animationWave received");
    currentAnimation = WAVE;
    request->send(200, "text/plain", "ok");
  });
  server.on("/animationCustom0", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("animationCustom0 received");
    currentAnimation = ANIMATION0;
    request->send(200, "text/plain", "ok");
  });
  server.on("/animationCustom1", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("animationCustom1 received");
    currentAnimation = ANIMATION1;
    request->send(200, "text/plain", "ok");
  });
  server.on("/animationCustom2", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("animationCustom2 received");
    currentAnimation = ANIMATION2;
    request->send(200, "text/plain", "ok");
  });
  server.on("/off", HTTP_GET, [] (AsyncWebServerRequest *request) {
    currentAnimation = OFF;
    Serial.println("off received");
    request->send(200, "text/plain", "ok");
  });
  server.onNotFound([](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(404, "text/html", "<html><body><h1>Error 404 - Not Found</h1></body></html>");
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
  });
  server.begin();
  Serial.println("Server started");
}

void voltmeterMeasuring() {
  uint16_t measurements[i];
  measurements[i] = analogRead(ADC);
  if (measurements[i] >= highValue) {
    highValue = measurements[i];
  } else if (measurements[i] <= lowValue) {
    lowValue = measurements[i];
  }
  sum += measurements[i];
  Serial.printf("Measurement number %d: %d\n", i+1, measurements[i]);
}

void voltmeterCalc() {
  Serial.printf("Lowest value: %d\n", lowValue);
  Serial.printf("Highest value: %d\n", highValue);
  Serial.printf("Suma: %d\n", sum);
  uint16_t sensorValues = (sum - (lowValue + highValue)) / (10 - 2);
  float voltage = sensorValues * (6.95 * (3.3 / 4095.0));
  Serial.printf("Average: %d\n", sensorValues);
  Serial.printf("Voltage: %.1f V\n", voltage);
  if (voltage < 0.1) {
    percent = 0;
    Serial.println("Voltmeter error");
    digitalWrite(LED, HIGH);
  } else if (voltage >= 0.2 && voltage < 6.8) {
    percent = 0;
  } else if (voltage >= 6.8 && voltage < 7.2) {
    percent = map(voltage*1000, 7000, 7199, 2, 3);
  } else if (voltage >= 7.2 && voltage < 7.4) {
    percent = map(voltage*1000, 7200, 7399, 4, 12);
  } else if (voltage >= 7.4 && voltage < 7.5) {
    percent = map(voltage*1000, 7400, 7499, 13, 19);
  } else if (voltage >= 7.5 && voltage < 7.6) {
    percent = map(voltage*1000, 7500, 7599, 20, 34);
  } else if (voltage >= 7.6 && voltage < 8.2) {
    percent = map(voltage*1000, 7600, 8199, 35, 89);
  } else if (voltage >= 8.2 && voltage < 8.4) {
    percent = map(voltage*1000, 8200, 8400, 90, 100);
  } else {
    percent = 100;
  }
  Serial.printf("Percent: %d%%\n", percent);
  sum = 0;
  i = 0;
  highValue = 0;
  lowValue = 4095;
}

void animations(void * pvParameters) {
  Serial.println(xPortGetCoreID());
  while(xPortGetCoreID()==setupCore) {
    digitalWrite(LED, HIGH);
    Serial.println();
    Serial.println("Core initialization failed - set animations on second core");
    vTaskDelay(portMAX_DELAY);
  }
  Serial.println("Core initialization - Passed");
  Serial.print("Starting animation: ");
  while(!moving && currentAnimation != lastAnimation) {
    moving = true;
    switch (currentAnimation) {
      case HOME: home(); Serial.print("home"); break;
      case FORWARD: moveForward(); Serial.print("moveForward"); break;
      case BACKWARD: moveBackward(); Serial.print("moveBackward"); break;
      case TURNLEFT: turnLeft(); Serial.print("turnLeft"); break;
      case TURNRIGHT: turnRight(); Serial.print("turnRight"); break;
      case ROTATELEFT: rotateLeft(); Serial.print("rotateLeft"); break;
      case ROTATERIGHT: rotateRight(); Serial.print("rotateRight"); break;
      case HELLO: animationHello(); Serial.print("animationHello"); break;
      case PUSHUP: animationPushup(); Serial.print("animationPushup"); break;
      case WAVE: animationWave(); Serial.print("animationWave"); break;
      case ANIMATION0: animationCustom0(); Serial.print("animationCustom0"); break;
      case ANIMATION1: animationCustom1(); Serial.print("animationCustom1"); break;
      case ANIMATION2: animationCustom2(); Serial.print("animationCustom2"); break;
      case LIE: lie(); Serial.print("lie"); break;
      case OFF: home(); Serial.print("off"); break;
      default: home(); digitalWrite(LED, HIGH); Serial.println("Animation error"); break;
    }
    Serial.println(" done");
    lastAnimation = currentAnimation;
    Serial.println(lastAnimation);
    moving = false;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void loop() {
  if (WiFi.softAPgetStationNum()!=lastStationCount) {
    if (WiFi.softAPgetStationNum()==maxConnection) {
      Serial.println("Server full");
      //currentAnimation = HOME;
    } else {
      Serial.println("Server open");
      currentAnimation = LIE;
    }
    lastStationCount = WiFi.softAPgetStationNum();
  }
  if (tempomat) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 100) {
      uint8_t range = sensor.readRange();
      uint8_t status = sensor.readRangeStatus();
      Serial.print("Tempomat - ");
      switch (status) {
        case VL6180X_ERROR_NONE:
          digitalWrite(LED, LOW);
          if (range <= 50) {
            currentAnimation = HOME;
            Serial.print("found an obstacle - ");
          } else {
            currentAnimation = FORWARD;
            Serial.print("measuring - ");
          }
          Serial.printf("Distance: %d mm\n", range);
          break;
        case VL6180X_ERROR_ECEFAIL:
          currentAnimation = HOME;
          digitalWrite(LED, HIGH);
          Serial.println("ECE failure");
          break;
        case VL6180X_ERROR_NOCONVERGE:
          currentAnimation = HOME;
          digitalWrite(LED, HIGH);
          Serial.println("no convergence");
          break;
        case VL6180X_ERROR_RANGEIGNORE:
          currentAnimation = HOME;
          digitalWrite(LED, HIGH);
          Serial.println("ignoring range");
          break;
        case VL6180X_ERROR_SNR:
          currentAnimation = HOME;
          digitalWrite(LED, HIGH);
          Serial.println("signal/noise error");
          break;
        case VL6180X_ERROR_RAWUFLOW:
          currentAnimation = HOME;
          digitalWrite(LED, HIGH);
          Serial.println("raw reading underflow");
          break;
        case VL6180X_ERROR_RAWOFLOW:
          currentAnimation = HOME;
          digitalWrite(LED, HIGH);
          Serial.println("raw reading overflow");
          break;
        case VL6180X_ERROR_RANGEUFLOW:
          currentAnimation = HOME;
          digitalWrite(LED, HIGH);
          Serial.println("range reading underflow");
          break;
        case VL6180X_ERROR_RANGEOFLOW:
          currentAnimation = HOME;
          digitalWrite(LED, HIGH);
          Serial.println("range reading overflow");
          break;
        default:
          currentAnimation = LIE;
          digitalWrite(LED, HIGH);
          Serial.println("unkown system error");
          break;
        }
      previousMillis = currentMillis;
    }
  }
}

void home() {
  SF0.write(defaultAngle);
  SF1.write(defaultAngle);
  SF2.write(defaultAngle);
  SF3.write(defaultAngle);
  S0.write(stopAngle1);
  S1.write(stopAngle0);
  S2.write(stopAngle1);
  S3.write(stopAngle0);
}

void lie() {
  SF0.write(180);
  SF1.write(180);
  SF2.write(180);
  SF3.write(180);
  S0.write(180);
  S1.write(0);
  S2.write(180);
  S3.write(0);
}

void moveForward() {
  S1.write(0);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  S1.write(30);
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  S1.write(60);
  vTaskDelay(1500 / portTICK_PERIOD_MS);
  S1.write(90);
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  /*SF1.write(defaultAngle+add);
  SF3.write(defaultAngle+add);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  S1.write(stopAngle0-20);
  S3.write(stopAngle0-20);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  SF1.write(defaultAngle);
  SF3.write(defaultAngle);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  SF0.write(defaultAngle+add);
  SF2.write(defaultAngle+add);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  S0.write(stopAngle1+35);
  S2.write(90);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  S1.write(90);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  SF0.write(defaultAngle);
  SF2.write(defaultAngle);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  SF1.write(defaultAngle+add+add);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  S1.write(stopAngle0);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  SF3.write(defaultAngle+10);
  S0.write(90);
  S2.write(stopAngle1);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  SF1.write(defaultAngle);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  SF3.write(defaultAngle);
  /*delay(stopDelay);

  SF0.write(defaultAngle+add);
  SF2.write(defaultAngle+add);
  delay(setSpeed);
  S0.write(stopAngle1+20);
  S2.write(stopAngle1+20);
  delay(setSpeed);
  SF0.write(defaultAngle);
  SF2.write(defaultAngle);
  delay(setSpeed);
  SF1.write(defaultAngle+add);
  SF3.write(defaultAngle+add);
  delay(setSpeed);
  S1.write(stopAngle0-35);
  S3.write(90);
  delay(setSpeed);
  S0.write(90);
  delay(setSpeed);
  SF1.write(defaultAngle);
  SF3.write(defaultAngle);
  delay(setSpeed);
  SF0.write(defaultAngle+add+add);
  delay(setSpeed);
  S0.write(stopAngle1);
  delay(setSpeed);
  SF2.write(defaultAngle+10);
  S1.write(90);
  S3.write(stopAngle0);
  delay(setSpeed);
  SF0.write(defaultAngle);
  delay(setSpeed);
  SF2.write(defaultAngle);
  delay(stopDelay);*/
}

void moveBackward() {
  SF0.write(defaultAngle+add);
  SF2.write(defaultAngle+add);
  delay(1000);
  S0.write(stopAngle1+20);
  S2.write(stopAngle1+20);
  delay(1000);
  SF0.write(defaultAngle);
  SF2.write(defaultAngle);
  delay(1000);
  SF1.write(defaultAngle+add);
  SF3.write(defaultAngle+add);
  delay(1000);
  S3.write(stopAngle0-35);
  S1.write(90);
  delay(1000);
  S2.write(90);
  delay(1000);
  SF1.write(defaultAngle);
  SF3.write(defaultAngle);
  delay(1000);
  SF2.write(defaultAngle+add+add);
  delay(1000);
  S2.write(stopAngle1);
  delay(1000);
  SF0.write(defaultAngle+10);
  S3.write(90);
  S1.write(stopAngle0);
  delay(1000);
  SF0.write(defaultAngle);
  delay(1000);
  SF2.write(defaultAngle);
  /*delay(stopDelay);

  SF1.write(defaultAngle+add);
  SF3.write(defaultAngle+add);
  delay(setSpeed);
  S1.write(stopAngle0-20);
  S3.write(stopAngle0-20);
  delay(setSpeed);
  SF1.write(defaultAngle);
  SF3.write(defaultAngle);
  delay(setSpeed);
  SF0.write(defaultAngle+add);
  SF2.write(defaultAngle+add);
  delay(setSpeed);
  S2.write(stopAngle1+35);
  S0.write(90);
  delay(setSpeed);
  S3.write(90);
  delay(setSpeed);
  SF0.write(defaultAngle);
  SF2.write(defaultAngle);
  delay(setSpeed);
  SF3.write(defaultAngle+add+add);
  delay(setSpeed);
  S3.write(stopAngle0);
  delay(setSpeed);
  SF1.write(defaultAngle+10);
  S2.write(90);
  S0.write(stopAngle1);
  delay(setSpeed);
  SF1.write(defaultAngle);
  delay(setSpeed);
  SF3.write(defaultAngle);
  delay(stopDelay);*/
}

void turnLeft() {
  SF1.write(defaultAngle+add);
  SF3.write(defaultAngle+add);
  delay(setSpeed);
  S1.write(stopAngle0-20);
  S3.write(stopAngle0-20);
  delay(setSpeed);
  SF1.write(defaultAngle);
  SF3.write(defaultAngle);
  delay(setSpeed);
  SF2.write(defaultAngle+add);
  SF0.write(defaultAngle+add);
  delay(setSpeed);
  S0.write(stopAngle1+35);
  S2.write(90);
  delay(setSpeed);
  S1.write(90);
  delay(setSpeed);
  SF2.write(defaultAngle);
  SF0.write(defaultAngle);
  delay(setSpeed);
  SF1.write(defaultAngle+add+add);
  delay(setSpeed);
  S1.write(stopAngle0);
  delay(setSpeed);
  SF3.write(defaultAngle+10);
  S2.write(150);
  S0.write(stopAngle1);
  delay(setSpeed);
  SF1.write(defaultAngle);
  delay(setSpeed);
  SF3.write(defaultAngle);
  delay(stopDelay);

  SF0.write(defaultAngle+add);
  SF2.write(defaultAngle+add);
  delay(setSpeed);
  S0.write(stopAngle1+20);
  S2.write(stopAngle1+20);
  delay(setSpeed);
  SF0.write(defaultAngle);
  SF2.write(defaultAngle);
  delay(setSpeed);
  SF1.write(defaultAngle+add);
  SF3.write(defaultAngle+add);
  delay(setSpeed);
  S3.write(stopAngle0-35);
  S1.write(90);
  delay(setSpeed);
  S2.write(90);
  delay(setSpeed);
  SF1.write(defaultAngle);
  SF3.write(defaultAngle);
  delay(setSpeed);
  SF2.write(defaultAngle+add+add);
  delay(setSpeed);
  S2.write(stopAngle1);
  delay(setSpeed);
  SF0.write(defaultAngle+10);
  S1.write(30);
  S3.write(stopAngle0);
  delay(setSpeed);
  SF2.write(defaultAngle);
  delay(setSpeed);
  SF0.write(defaultAngle);
  delay(stopDelay);
}

void turnRight() {
  SF1.write(defaultAngle+add);
  SF3.write(defaultAngle+add);
  delay(setSpeed);
  S1.write(stopAngle0-20);
  S3.write(stopAngle0-20);
  delay(setSpeed);
  SF1.write(defaultAngle);
  SF3.write(defaultAngle);
  delay(setSpeed);
  SF0.write(defaultAngle+add);
  SF2.write(defaultAngle+add);
  delay(setSpeed);
  S2.write(stopAngle1+35);
  S0.write(90);
  delay(setSpeed);
  S3.write(90);
  delay(setSpeed);
  SF0.write(defaultAngle);
  SF2.write(defaultAngle);
  delay(setSpeed);
  SF3.write(defaultAngle+add+add);
  delay(setSpeed);
  S3.write(stopAngle0);
  delay(setSpeed);
  SF1.write(defaultAngle+10);
  S0.write(150);
  S2.write(stopAngle1);
  delay(setSpeed);
  SF1.write(defaultAngle);
  delay(setSpeed);
  SF3.write(defaultAngle);
  delay(stopDelay);

  SF0.write(defaultAngle+add);
  SF2.write(defaultAngle+add);
  delay(setSpeed);
  S0.write(stopAngle1+20);
  S2.write(stopAngle1+20);
  delay(setSpeed);
  SF0.write(defaultAngle);
  SF2.write(defaultAngle);
  delay(setSpeed);
  SF1.write(defaultAngle+add);
  SF3.write(defaultAngle+add);
  delay(setSpeed);
  S1.write(stopAngle0-35);
  S3.write(90);
  delay(setSpeed);
  S0.write(90);
  delay(setSpeed);
  SF1.write(defaultAngle);
  SF3.write(defaultAngle);
  delay(setSpeed);
  SF0.write(defaultAngle+add+add);
  delay(setSpeed);
  S0.write(stopAngle1);
  delay(setSpeed);
  SF2.write(defaultAngle+10);
  S3.write(30);
  S1.write(stopAngle0);
  delay(setSpeed);
  SF0.write(defaultAngle);
  delay(setSpeed);
  SF2.write(defaultAngle);
  delay(stopDelay);
}

void rotateRight() {
  SF0.write(defaultAngle+add);
  S0.write(rightAngle1);
  delay(setSpeed);
  SF0.write(defaultAngle);
  delay(setSpeed);
  
  SF1.write(defaultAngle+add);
  S1.write(beginAngle0);
  delay(setSpeed);
  SF1.write(defaultAngle);
  delay(setSpeed);

  SF2.write(defaultAngle+add);
  S2.write(rightAngle1);
  delay(setSpeed);
  SF2.write(defaultAngle);
  delay(setSpeed);
  
  SF3.write(defaultAngle+add);
  S3.write(beginAngle0);
  delay(setSpeed);
  SF3.write(defaultAngle);
  delay(setSpeed);
  
  S0.write(stopAngle1);
  S1.write(stopAngle0);
  S2.write(stopAngle1);
  S3.write(stopAngle0);
  delay(stopDelay);
}

void rotateLeft() {
  SF0.write(defaultAngle+add);
  S0.write(beginAngle1);
  delay(setSpeed);
  SF0.write(defaultAngle);
  delay(setSpeed);
  
  SF1.write(defaultAngle+add);
  S1.write(rightAngle0);
  delay(setSpeed);
  SF1.write(defaultAngle);
  delay(setSpeed);

  SF2.write(defaultAngle+add);
  S2.write(beginAngle1);
  delay(setSpeed);
  SF2.write(defaultAngle);
  delay(setSpeed);
  
  SF3.write(defaultAngle+add);
  S3.write(rightAngle0);
  delay(setSpeed);
  SF3.write(defaultAngle);
  delay(setSpeed);
  S0.write(stopAngle1);
  S1.write(stopAngle0);
  S2.write(stopAngle1);
  S3.write(stopAngle0);
  delay(stopDelay);
}

void animationHello() {

}

void animationPushup() {

}

void animationWave() {

}

void animationCustom0() {

}

void animationCustom1() {
  
}

void animationCustom2() {
  
}

