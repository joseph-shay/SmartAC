#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR-PASS";

ESP8266WebServer server(80);

// Relay pins
#define RELAY1 D5
#define RELAY2 D6

bool ac1Status = false;
bool ac2Status = false;

unsigned long timerStartMillis = 0;
unsigned long timerDurationMillis = 0;
bool timerRunning = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected. IP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/toggle1", toggleAC1);
  server.on("/toggle2", toggleAC2);
  server.on("/setTimer", setTimer);

  server.begin();
  Serial.println("Server started.");
}

void loop() {
  server.handleClient();

  if (timerRunning && millis() - timerStartMillis >= timerDurationMillis) {
    ac1Status = false;
    ac2Status = false;
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY2, LOW);
    timerRunning = false;
    Serial.println("Timer expired. ACs turned off.");
  }
}

void handleRoot() {
  unsigned long remaining = timerRunning ? (timerDurationMillis - (millis() - timerStartMillis)) / 1000 : 0;
  int hrs = remaining / 3600;
  int mins = (remaining % 3600) / 60;
  int secs = remaining % 60;

  String remainingStr = String(hrs) + "h " + String(mins) + "m " + String(secs) + "s";

  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>AC Control</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body { font-family: Arial; text-align: center; padding: 20px; }
        .switch { position: relative; display: inline-block; width: 60px; height: 34px; }
        .switch input { display: none; }
        .slider { position: absolute; cursor: pointer; top: 0; left: 0;
                  right: 0; bottom: 0; background-color: #ccc;
                  transition: .4s; border-radius: 34px; }
        .slider:before { position: absolute; content: ""; height: 26px; width: 26px;
                         left: 4px; bottom: 4px; background-color: white;
                         transition: .4s; border-radius: 50%; }
        input:checked + .slider { background-color: #2196F3; }
        input:checked + .slider:before { transform: translateX(26px); }
      </style>
    </head>
    <body>
      <h2>AC Control Panel</h2>

      <p>Downstairs AC</p>
      <label class="switch">
        <input type="checkbox" onchange="toggleAC(1)" %CHECK1%>
        <span class="slider"></span>
      </label>

      <p>Upstairs AC</p>
      <label class="switch">
        <input type="checkbox" onchange="toggleAC(2)" %CHECK2%>
        <span class="slider"></span>
      </label>

      <h3>Timer (minutes)</h3>
      <input type="number" id="minutes" min="1" max="180">
      <button onclick="setTimer()">Start Timer</button>

      <p id="countdown">Remaining: %REMAINING%</p>

      <script>
        function toggleAC(ac) {
          fetch("/toggle" + ac);
        }
        function setTimer() {
          const minutes = document.getElementById("minutes").value;
          if (minutes > 0) {
            fetch("/setTimer?m=" + minutes);
          }
        }
        setInterval(() => location.reload(), 5000);
      </script>
    </body>
    </html>
  )rawliteral";

  html.replace("%CHECK1%", ac1Status ? "checked" : "");
  html.replace("%CHECK2%", ac2Status ? "checked" : "");
  html.replace("%REMAINING%", remainingStr);

  server.send(200, "text/html", html);
}

void toggleAC1() {
  ac1Status = !ac1Status;
  digitalWrite(RELAY1, ac1Status ? HIGH : LOW);
  Serial.println("Toggled AC1: " + String(ac1Status ? "ON" : "OFF"));
  server.send(200, "text/plain", "Toggled AC1");
}

void toggleAC2() {
  ac2Status = !ac2Status;
  digitalWrite(RELAY2, ac2Status ? HIGH : LOW);
  Serial.println("Toggled AC2: " + String(ac2Status ? "ON" : "OFF"));
  server.send(200, "text/plain", "Toggled AC2");
}

void setTimer() {
  if (server.hasArg("m")) {
    int minutes = server.arg("m").toInt();
    if ((ac1Status || ac2Status) && minutes > 0) {
      timerDurationMillis = (unsigned long)minutes * 60000;
      timerStartMillis = millis();
      timerRunning = true;
      Serial.println("Timer started for " + String(minutes) + " minutes.");
      server.send(200, "text/plain", "Timer started");
    } else {
      server.send(200, "text/plain", "No AC is on or invalid time");
    }
  }
}
