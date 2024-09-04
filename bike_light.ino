#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#define SIGNAL_LED D4
#define STOP_LED D6
#define RIGHT_TURN_LED D5
#define LEFT_TURN_LED D0
#define SW_BTN D7

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

int leds_pause_s = false;
int leds_pause_r = false;
int leds_pause_l = false;

int currentSignalMode = 0;
bool rightTurnSignal = false;
bool leftTurnSignal = false;
bool stopSignalActive = false;

int currentSignalMode_save = 0;
int rightTurnSignal_save = false;
int leftTurnSignal_save = false;

unsigned long lastRightTurnToggle = 0;
unsigned long lastLeftTurnToggle = 0;
unsigned long lastSignalUpdate = 0;
unsigned long stopSignalEndTime = 0;
unsigned long rightTurnStartTime = 0;
unsigned long leftTurnStartTime = 0;
unsigned long lastActivityTime = 0;  // Время последней активности

const unsigned long inactivityThreshold = 5 * 60 * 1000;     // 5 минут в миллисекундах
const unsigned long deepSleepDuration = 2 * 60 * 60 * 1000;  // 2 часа в миллисекундах


const char *ssid = "";
const char *password = "";


const char *server_api = ".....";
const int port = 80;

bool server_started = 0;

// Функция для добавления CORS-заголовков ко всем ответам
void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Credentials", "true");
    server.sendHeader("Access-Control-Allow-Headers", "Origin, Content-Type, Accept, Authorization, X-Request-With");
  server.sendHeader("Cache-Control", "");
  server.sendHeader("X-Request-With", "");

}

void sendAnswer(String status = "OK") {
  String json = "{";
  json += "\"status\":" + String(status) + ",";
  json += "\"leds_pause_s\":" + String(leds_pause_s) + ",";
  json += "\"leds_pause_r\":" + String(leds_pause_r) + ",";
  json += "\"leds_pause_l\":" + String(leds_pause_l) + ",";
  json += "\"currentSignalMode\":" + String(currentSignalMode) + ",";
  json += "\"rightTurnSignal\":" + String(rightTurnSignal) + ",";
  json += "\"leftTurnSignal\":" + String(leftTurnSignal) + ",";
  json += "\"stopSignalActive\":" + String(stopSignalActive) + ",";
  json += "\"currentSignalMode_save\":" + String(currentSignalMode_save) + ",";
  json += "\"rightTurnSignal_save\":" + String(rightTurnSignal_save) + ",";
  json += "\"leftTurnSignal_save\":" + String(leftTurnSignal_save) + ",";
  json += "\"lastRightTurnToggle\":" + String(lastRightTurnToggle) + ",";
  json += "\"lastLeftTurnToggle\":" + String(lastLeftTurnToggle) + ",";
  json += "\"lastSignalUpdate\":" + String(lastSignalUpdate) + ",";
  json += "\"stopSignalEndTime\":" + String(stopSignalEndTime) + ",";
  json += "\"rightTurnStartTime\":" + String(rightTurnStartTime) + ",";
  json += "\"leftTurnStartTime\":" + String(leftTurnStartTime) + ",";
  json += "\"lastActivityTime\":" + String(lastActivityTime) + ",";
  json += "\"inactivityThreshold\":" + String(inactivityThreshold) + ",";
  json += "\"deepSleepDuration\":" + String(deepSleepDuration);
  json += "}";

  server.send(200, "text/html", json);
}

// Function to handle OPTIONS requests
void handleOptions() {
  addCorsHeaders();
  server.send(204);  // No Content
}

void goToDeepSleep() {
  WiFi.disconnect();
  delay(100);
  digitalWrite(SIGNAL_LED, HIGH);
  digitalWrite(RIGHT_TURN_LED, HIGH);
  digitalWrite(LEFT_TURN_LED, HIGH);
  digitalWrite(STOP_LED, HIGH);
  delay(100);
  ESP.deepSleep(deepSleepDuration * 1000);
}

void goToLightSleep() {
  delay(100);

  digitalWrite(SIGNAL_LED, HIGH);
  digitalWrite(RIGHT_TURN_LED, HIGH);
  digitalWrite(LEFT_TURN_LED, HIGH);
  digitalWrite(STOP_LED, HIGH);

  delay(100);
  wifi_fpm_do_sleep(1000000 * 60 * 60);
}

void setup() {

  pinMode(SIGNAL_LED, OUTPUT);
  pinMode(STOP_LED, OUTPUT);
  pinMode(RIGHT_TURN_LED, OUTPUT);
  pinMode(LEFT_TURN_LED, OUTPUT);
  pinMode(SW_BTN, INPUT_PULLUP);

  analogWrite(SIGNAL_LED, HIGH);
  digitalWrite(STOP_LED, HIGH);
  digitalWrite(RIGHT_TURN_LED, HIGH);
  digitalWrite(LEFT_TURN_LED, HIGH);

  lastActivityTime = millis();

  wifi_set_sleep_type(MODEM_SLEEP_T);
}

void sendDataToServer() {
  lastActivityTime = millis();


  WiFiClient client;
  bool success = false;

  if (!client.connect(server_api, port)) {
    return;
  }

  String ip = WiFi.localIP().toString();
  String ssid = WiFi.SSID();
  int signalStrength = WiFi.RSSI();                      // Уровень сигнала Wi-Fi
  String firmwareVersion = String(ESP.getSketchSize());  // Пример версии прошивки
  String freeHeap = String(ESP.getFreeHeap());           // Доступная память

  // Собираем данные о плате и соединении
  String data = "ip=" + ip + "&";
  data += "ssid=" + ssid + "&";
  data += "signal_strength=" + String(signalStrength) + "&";
  data += "firmware_version=" + firmwareVersion + "&";
  data += "free_heap=" + freeHeap + "&";
  data += "chip_id=" + String(ESP.getChipId()) + "&";
  data += "sdk_version=" + String(ESP.getSdkVersion()) + "&";
  data += "cpu_freq=" + String(ESP.getCpuFreqMHz()) + "&";
  data += "flash_size=" + String(ESP.getFlashChipSize()) + "&";
  data += "flash_real_size=" + String(ESP.getFlashChipRealSize()) + "&";
  data += "flash_speed=" + String(ESP.getFlashChipSpeed()) + "&";
  data += "status=" + String(WiFi.status());

  String url = "/esp_ip.php?" + data;

  String request = String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + server_api + "\r\n" + "Connection: close\r\n\r\n";
  client.print(request);
  client.stop();
}

void StartServer() {
  server_started = 1;
  unsigned long wifiStartAttempt = millis();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {

    if (millis() - wifiStartAttempt >= 2 * 60 * 1000) {
      goToDeepSleep();
    }
    delay(1000);
  }

  sendDataToServer();

  digitalWrite(RIGHT_TURN_LED, LOW);
  delay(100);
  digitalWrite(RIGHT_TURN_LED, HIGH);

  server.on("/", HTTP_OPTIONS, handleOptions);
  server.on("/setSignalMode", HTTP_OPTIONS, handleOptions);
  server.on("/rightTurn", HTTP_OPTIONS, handleOptions);
  server.on("/leftTurn", HTTP_OPTIONS, handleOptions);
  server.on("/stopSignal", HTTP_OPTIONS, handleOptions);
  server.on("/startUpdate", HTTP_OPTIONS, handleOptions);
  server.on("/reboot", HTTP_OPTIONS, handleOptions);

  server.on("/", handleRoot);
  server.on("/setSignalMode", handleSetSignalMode);
  server.on("/rightTurn", handleRightTurn);
  server.on("/leftTurn", handleLeftTurn);
  server.on("/stopSignal", handleStopSignal);
  server.on("/startUpdate", handleStartUpdate);
  server.on("/reboot", handleReboot);

  server.onNotFound(handleNotFound);

  server.begin();
}

void loop() {

  if (server_started) {
    server.handleClient();
  }

  handleRightTurnSignal();
  handleLeftTurnSignal();
  handleSignalMode();
  handleStopSignalState();

  if (millis() - lastActivityTime > inactivityThreshold && currentSignalMode == 0) {
    goToLightSleep();
  }

  // Проверка состояния кнопки
  if (digitalRead(SW_BTN) == LOW) {
    delay(50);

    unsigned long pressTime = millis();

    while (digitalRead(SW_BTN) == LOW) {
      delay(10);
      if (millis() - pressTime >= 3000 && millis() - pressTime <= 5000) {
        lastSignalUpdate = millis();
        if (currentSignalMode != 19) {
          currentSignalMode = 19;
          currentSignalMode_save = 19;
        } else {
          currentSignalMode = 0;
          leftTurnSignal = false;
          rightTurnSignal = false;
        }

      } else if (millis() - pressTime >= 5000 && millis() - pressTime <= 10000) {
        digitalWrite(SIGNAL_LED, LOW);
        delay(100);
        digitalWrite(SIGNAL_LED, HIGH);
        StartServer();

      } else if (millis() - pressTime >= 10000) {
        digitalWrite(SIGNAL_LED, LOW);
        delay(100);
        digitalWrite(SIGNAL_LED, HIGH);
        delay(100);
        digitalWrite(SIGNAL_LED, LOW);
        delay(100);
        digitalWrite(SIGNAL_LED, HIGH);
        delay(100);
        digitalWrite(SIGNAL_LED, LOW);
        delay(100);
        digitalWrite(SIGNAL_LED, HIGH);
        delay(100);

        goToDeepSleep();
      }
    }

    // Если кнопка была нажата и отпущена до 5 секунд
    if (millis() - pressTime < 3000) {
      currentSignalMode = (currentSignalMode + 1) % 20;
    }
  }
}


////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

void LedsPause(bool S = true, bool L = true, bool R = true) {

  if (S) {
    leds_pause_s = true;
    currentSignalMode_save = currentSignalMode;

    currentSignalMode = 0;
    digitalWrite(SIGNAL_LED, HIGH);
  }

  if (R) {
    leds_pause_r = true;
    rightTurnSignal_save = rightTurnSignal;

    rightTurnSignal = 0;
    digitalWrite(RIGHT_TURN_LED, HIGH);
  }

  if (L) {
    leds_pause_l = true;
    leftTurnSignal_save = leftTurnSignal;

    leftTurnSignal = 0;
    digitalWrite(LEFT_TURN_LED, HIGH);
  }
}

void LedsBack(bool S = true, bool L = true, bool R = true) {

  if (S) {
    leds_pause_s = false;
    currentSignalMode = currentSignalMode_save;
  }
  if (R) {
    leds_pause_r = false;
    rightTurnSignal = rightTurnSignal_save;
  }
  if (L) {
    leds_pause_l = false;
    leftTurnSignal = leftTurnSignal_save;
  }
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

void handleRoot() {
  addCorsHeaders();
  lastActivityTime = millis();

  String html = R"=====(
  <html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Bike Light Control</title>
  <script>
    function sendRequest(url) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", url, true);
        xhr.send();
    }

  function updateFirmware() {
        sendRequest('/startUpdate');
        window.setTimeout(function(){window.location.href = "/update";}, 2000);
    }
  </script>
  </head>
  <body>
        <button class="button" onclick="updateFirmware()">Update Firmware</button>
        <button class="button" onclick="sendRequest('/reboot')">reboot</button>
  </body>
  </html>
  )=====";
  server.send(200, "text/html", html);
}

void handleStartUpdate() {
  addCorsHeaders();
  lastActivityTime = millis();

  sendAnswer();
  httpUpdater.setup(&server);
}

void handleReboot() {
  lastActivityTime = millis();

  sendAnswer();
  delay(500);
  ESP.restart();
}


void handleSetSignalMode() {
  addCorsHeaders();
  lastActivityTime = millis();


  if (server.hasArg("mode")) {
    currentSignalMode = server.arg("mode").toInt();
  }
  sendAnswer();
}

void handleRightTurn() {
  addCorsHeaders();
  lastActivityTime = millis();


  rightTurnSignal = !rightTurnSignal;
  if (rightTurnSignal) {
    rightTurnStartTime = millis();  // Запоминаем время включения
    LedsPause(true, false, false);  //блокируем сигнал
  } else {
    LedsBack(true, false, false);  //блокируем сигнал
  }

  sendAnswer();
}

void handleLeftTurn() {
  addCorsHeaders();
  lastActivityTime = millis();

  leftTurnSignal = !leftTurnSignal;

  if (leftTurnSignal) {
    leftTurnStartTime = millis();   // Запоминаем время включения
    LedsPause(true, false, false);  //блокируем сигнал
  } else {
    LedsBack(true, false, false);  //блокируем сигнал и правый повопрот
  }

  sendAnswer();
}


void handleStopSignal() {
  addCorsHeaders();
  lastActivityTime = millis();

  stopSignalActive = true;
  stopSignalEndTime = millis() + 5000;

  sendAnswer();
}

void handleNotFound() {
  addCorsHeaders();
  lastActivityTime = millis();

  server.send(404, "text/plain", "Not Found");
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

int lastSignalMode = 0;

void handleSignalMode() {

  if (leds_pause_s) return;                      // Если сигналы запрещены выходим
  if (millis() - lastSignalUpdate < 50) return;  // Интервал между циклами
  lastSignalUpdate = millis();

  digitalWrite(SIGNAL_LED, HIGH);

  if (currentSignalMode != 0) {
    lastActivityTime = millis();
  }

  if (lastSignalMode == 3 && leds_pause_s) {
    leftTurnSignal = false;
    rightTurnSignal = false;
  }

  lastSignalMode = currentSignalMode;


  switch (currentSignalMode) {
    case 0:
      digitalWrite(SIGNAL_LED, HIGH);
      break;
    case 1:
      digitalWrite(SIGNAL_LED, (millis() / 500) % 2 == 0 ? LOW : HIGH);
      break;
    case 2:
      {
        static uint32_t morseStartTime = millis();  // Время начала текущего сигнала
        static int morseIndex = 0;                  // Индекс текущего символа
        static bool ledState = LOW;                 // Текущее состояние LED (LOW = включено)

        // Определяем длительность мигания в зависимости от текущего символа
        uint32_t morseDurations[] = { 500, 100, 100, 100, 500, 100, 100, 500, 500, 100, 100, 500, 500, 100, 100, 500, 100, 500, 100, 100, 100, 500, 100, 100, 500, 500, 500 };
        // Это последовательность сигналов для ".--. .-. .. .-- . -" (привет)

        if (millis() - morseStartTime >= morseDurations[morseIndex]) {
          morseStartTime = millis();  // Обновляем время начала текущего сигнала
          morseIndex++;               // Переходим к следующему символу

          // Переключаем состояние LED
          if (morseIndex % 2 == 0) {
            ledState = LOW;  // Включаем свет
          } else {
            ledState = HIGH;  // Выключаем свет
          }

          // Если достигли конца последовательности, начинаем заново
          if (morseIndex >= sizeof(morseDurations) / sizeof(morseDurations[0])) {
            morseIndex = 0;
          }
        }

        digitalWrite(SIGNAL_LED, ledState);  // Устанавливаем состояние LED
        break;
      }
    case 3:
      if ((millis() / 400) % 3 == 0) analogWrite(SIGNAL_LED, 128);
      else if ((millis() / 400) % 3 == 1) analogWrite(SIGNAL_LED, 255);
      else analogWrite(SIGNAL_LED, 0);
      break;
    case 4:
      {
        static uint32_t kalinkaStartTime = millis();  // Время начала текущего мигания
        static int kalinkaIndex = 0;                  // Индекс текущего мигания
        static bool ledState = LOW;                   // Текущее состояние LED (LOW = включено)

        // Последовательность миганий для полной версии "Калинка"
        uint32_t kalinkaDurations[] = {
          300, 300, 600, 300, 300, 600,    // "Ка-лин-ка, Ка-лин-ка"
          300, 300, 600, 300, 300, 600,    // "Ка-лин-ка моя"
          500, 500, 1000, 500, 500, 1000,  // "В саду ягода малинка, малинка моя"

          300, 300, 600, 300, 300, 600,    // "Ка-лин-ка, Ка-лин-ка"
          300, 300, 600, 300, 300, 600,    // "Ка-лин-ка моя"
          500, 500, 1000, 500, 500, 1000,  // "В саду ягода малинка, малинка моя"

          300, 300, 600, 300, 300, 600,    // "Под сосной, под зелёною"
          300, 300, 600, 300, 300, 600,    // "Спать положите вы меня"
          500, 500, 1000, 500, 500, 1000,  // "Ай-люли, люли, ай-люли, люли"

          300, 300, 600, 300, 300, 600,    // "Ка-лин-ка, Ка-лин-ка"
          300, 300, 600, 300, 300, 600,    // "Ка-лин-ка моя"
          500, 500, 1000, 500, 500, 1000,  // "В саду ягода малинка, малинка моя"
        };

        // Если прошло достаточно времени с момента последнего изменения состояния
        if (millis() - kalinkaStartTime >= kalinkaDurations[kalinkaIndex]) {
          kalinkaStartTime = millis();  // Обновляем время начала текущего мигания
          kalinkaIndex++;               // Переходим к следующему миганию

          // Переключаем состояние LED
          ledState = !ledState;  // Переключаем состояние светодиода

          // Если достигли конца последовательности, начинаем заново
          if (kalinkaIndex >= sizeof(kalinkaDurations) / sizeof(kalinkaDurations[0])) {
            kalinkaIndex = 0;
          }
        }

        digitalWrite(SIGNAL_LED, ledState);  // Устанавливаем состояние LED
        break;
      }
      break;
    case 5:
      analogWrite(SIGNAL_LED, (millis() % 200 < 100) ? 255 : 0);
      break;
    case 6:
      analogWrite(SIGNAL_LED, (millis() % 500) / 2);
      break;
    case 7:
      analogWrite(SIGNAL_LED, (millis() % 2550) / 10);
      break;
    case 8:
      digitalWrite(SIGNAL_LED, (millis() / 600) % 2 == 0 ? LOW : HIGH);
      break;
    case 9:
      analogWrite(SIGNAL_LED, (millis() % 600 < 300) ? 255 : 0);
      break;
    case 10:
      digitalWrite(SIGNAL_LED, (millis() / 150) % 6 < 3 ? LOW : HIGH);
      break;
    case 11:
      digitalWrite(SIGNAL_LED, (millis() / 100) % 6 < 3 ? LOW : HIGH);
      break;
    case 12:
      analogWrite(SIGNAL_LED, (millis() / 2000) % 2 == 0 ? 128 : 0);
      break;
    case 13:
      analogWrite(SIGNAL_LED, (millis() % 3060) < 1530 ? (millis() % 1530) / 6 : (3060 - (millis() % 3060)) / 6);
      break;
    case 14:
      if ((millis() / 1500) % 3 == 0) analogWrite(SIGNAL_LED, 255);
      else if ((millis() / 1500) % 3 == 1) analogWrite(SIGNAL_LED, 128);
      else analogWrite(SIGNAL_LED, 0);
      break;
    case 15:
      if ((millis() / 250) % 4 == 0) analogWrite(SIGNAL_LED, 255);
      else if ((millis() / 250) % 4 == 1) analogWrite(SIGNAL_LED, 128);
      else if ((millis() / 250) % 4 == 2) analogWrite(SIGNAL_LED, 64);
      else analogWrite(SIGNAL_LED, 0);
      break;
    case 16:
      if ((millis() / 400) % 2 == 0) analogWrite(SIGNAL_LED, 255);
      else analogWrite(SIGNAL_LED, 128);
      break;
    case 17:
      if ((millis() / 1000) % 2 == 0) analogWrite(SIGNAL_LED, 255);
      else analogWrite(SIGNAL_LED, 0);
      break;
    case 18:
      if ((millis() / 2000) % 2 == 0) analogWrite(SIGNAL_LED, 0);
      else analogWrite(SIGNAL_LED, 255);
      break;
    case 19:

      if (millis() - leftTurnStartTime > 6000) {
        leftTurnSignal = !leftTurnSignal;
        rightTurnSignal = !rightTurnSignal;
        if (leftTurnSignal) {
          leftTurnStartTime = millis();  // Запоминаем время включения
        }

        if (rightTurnSignal) {
          rightTurnStartTime = millis();  // Запоминаем время включения
        }
      }
      break;
  }
}

void handleRightTurnSignal() {

  if (rightTurnSignal) {
    lastActivityTime = millis();

    if (millis() - rightTurnStartTime > 15000) {
      rightTurnSignal = false;
      digitalWrite(RIGHT_TURN_LED, HIGH);
      LedsBack(true, false, false);
    } else {
      if (millis() - lastRightTurnToggle > 500) {
        digitalWrite(RIGHT_TURN_LED, !digitalRead(RIGHT_TURN_LED));
        lastRightTurnToggle = millis();
      }
    }
  } else {
    digitalWrite(RIGHT_TURN_LED, HIGH);
  }
}

void handleLeftTurnSignal() {

  if (leftTurnSignal) {
    lastActivityTime = millis();

    if (millis() - leftTurnStartTime > 15000) {
      leftTurnSignal = false;
      digitalWrite(LEFT_TURN_LED, HIGH);
      LedsBack(true, false, false);
    } else {
      if (millis() - lastLeftTurnToggle > 500) {
        digitalWrite(LEFT_TURN_LED, !digitalRead(LEFT_TURN_LED));
        lastLeftTurnToggle = millis();
      }
    }
  } else {
    digitalWrite(LEFT_TURN_LED, HIGH);
  }
}



void handleStopSignalState() {

  if (stopSignalActive) {
    lastActivityTime = millis();

    if (millis() > stopSignalEndTime) {
      digitalWrite(STOP_LED, HIGH);
      stopSignalActive = false;
    } else {
      digitalWrite(STOP_LED, LOW);
    }
  }
}
