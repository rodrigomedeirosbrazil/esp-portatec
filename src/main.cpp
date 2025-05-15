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

void handleRoot();
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='0;url=http://" + myIP.toString() + "'>";
  html += "<title>Portal de Autenticação</title></head>";
  html += "<body><h1>Bem-vindo ao ESP-PORTATEC</h1>";
  html += "<p>Por favor, autentique-se para acessar a internet</p>";
  html += "<form method='POST' action='/login'>";
  html += "<input type='text' name='username' placeholder='Usuário'><br>";
  html += "<input type='password' name='password' placeholder='Senha'><br>";
  html += "<button type='submit'>Conectar</button></form></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println();
  Serial.print("Configuring access point...");

  WiFi.softAP(ssid, password);

  myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  dnsServer.start(53, "*", myIP);
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
}
