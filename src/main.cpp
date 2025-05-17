#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <DNSServer.h>

#ifndef APSSID
  #define APSSID "ESP-PORTATEC"
  #define APPSK "123456789"
#endif

const char *ssid = APSSID;
const char *password = APPSK;
IPAddress myIP;

DNSServer dnsServer;
ESP8266WebServer server(80);

void handleNotFound();
void handleNotFound() {
  server.sendHeader("Location", "http://" + myIP.toString(), true);
  server.send(302, "text/plain", "");
}

void handleGPIO() {
  if (!server.hasArg("gpio")) {
    server.send(400, "text/plain", "GPIO parameter required");
    return;
  }

  int gpio = server.arg("gpio").toInt();
  if (gpio < 0 || gpio > 3) {
    server.send(400, "text/plain", "Invalid GPIO number");
    return;
  }

  Serial.print("GPIO");
  Serial.print(gpio);
  Serial.println(" HIGH");
  digitalWrite(gpio, HIGH);
  delay(1000);
  Serial.print("GPIO");
  Serial.print(gpio);
  Serial.println(" LOW");
  digitalWrite(gpio, LOW);
  server.send(200, "text/plain", "GPIO" + String(gpio) + " toggled");
}

void handleRoot();
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP-PORTATEC Control</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }";
  html += "button { padding: 10px 20px; font-size: 16px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }";
  html += "button:hover { background-color: #45a049; }";
  html += "button:disabled { background-color: #cccccc; cursor: not-allowed; }";
  html += ".processing { background-color: #ff9800 !important; }";
  html += ".button-container { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; margin: 20px 0; }";
  html += "</style></head>";
  html += "<body>";
  html += "<h1>ESP-PORTATEC Control</h1>";
  html += "<p>Dispositivo: " + String(APSSID) + "</p>";
  html += "<div class='button-container'>";
  html += "<button id='gpio0Button' onclick='toggleGPIO(0)'>Toggle GPIO0</button>";
  html += "<button id='gpio1Button' onclick='toggleGPIO(1)'>Toggle GPIO1</button>";
  html += "<button id='gpio2Button' onclick='toggleGPIO(2)'>Toggle GPIO2</button>";
  html += "<button id='gpio3Button' onclick='toggleGPIO(3)'>Toggle GPIO3</button>";
  html += "</div>";
  html += "<script>";
  html += "function toggleGPIO(gpio) {";
  html += "  const button = document.getElementById('gpio' + gpio + 'Button');";
  html += "  button.disabled = true;";
  html += "  button.classList.add('processing');";
  html += "  button.textContent = 'Processando...';";
  html += "  fetch('/gpio?gpio=' + gpio)";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      console.log(data);";
  html += "      setTimeout(() => {";
  html += "        button.disabled = false;";
  html += "        button.classList.remove('processing');";
  html += "        button.textContent = 'Toggle GPIO' + gpio;";
  html += "      }, 1000);";
  html += "    });";
  html += "}";
  html += "</script></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.print("Configuring access point...");

  delay(1000);

  WiFi.softAP(ssid, password);

  myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Configure all GPIOs as outputs
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);

  // Initialize all GPIOs to LOW
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);

  server.on("/", handleRoot);
  server.on("/gpio", handleGPIO);
  server.onNotFound(handleNotFound);
  server.begin();
  dnsServer.start(53, "*", myIP);
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
}
