/* Rele controller with WiFi Client 
 *  for Arduino Wemos D1 or ESP8266
   by Allan Ayes Ramírez (Slam)
   E-mail: aayes89@gmail.com
   (03/05/2025)
*/

/*
   Copyright (c) 2022, Allan Ayes Ramírez (Slam)
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Allan Ayes Ramírez (Slam) nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "iHOME2"
#define STAPSK "Ayesperdi.2"
//#define USR "admin"
//#define PASS "admin123"
#define REALM "Login a Sistema Ventilador"
#define AERROR "<html><head><title>Ventilador</title><link rel='stylesheet' href='/styles.css'></head><body><div class='container'><h1>Login incorrecto</h1></div></body></html>"
#define WPORT 80
#define DPORT 53
#define RGPIO D8
#endif

// Web Auth
//const char* www_username = USR;
//const char* www_password = PASS;

// allows you to set the realm of authentication Default:"Login Required"
const char* www_realm = REALM;
// the Content of the HTML response in case of Unautherized Access Default:empty
String authFailResponse = AERROR;

// PIN LED
const int led = LED_BUILTIN;
// PIN Rele
const int rele = RGPIO;
// SSID
const char* ssid = STASSID;
// AP Pass
const char* password = STAPSK;

// DNS server
const byte DNS_PORT = DPORT;
DNSServer dnsServer;

// Web server on port 80
ESP8266WebServer server(WPORT);

// Manage server behavior
void handleRoot() {
  digitalWrite(led, HIGH);
  String index = "<!DOCTYPE html>\n";
  index += "<html>\n";
  index += "<head>\n";
  index += " <meta charset='utf-8'>\n";
  index += " <meta http-equiv='X-UA-Compatible' content='IE=edge'>\n";
  index += " <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
  index += " <title>Ventilador</title>\n";
  index += " <script src='/scripts.js'></script>\n";
  index += " <link rel='stylesheet' href='/styles.css'>\n";
  index += " <link rel='icon' href='/favicon.ico' type='image/png' />\n";
  index += "</head>\n";
  index += "<body>\n";
  index += " <div class='container'>\n";
  index += "  <div id='div_title'> <h1>Deslizar para cambiar estado del ventilador</h1> </div>\n";
  index += "  <div id='td_switch_btn' class='switch-button'>\n";
  index += "   <input type='checkbox' name='switch-button' id='switch-label' class='switch-button__checkbox' onclick='getState()'>\n";
  index += "   <label for='switch-label' class='switch-button__label' style='animation: glow 1s ease-in-out infinite alternate;'></label>\n";
  index += "  </div>\n";
  index += " <div id='td_status'></div>\n";
  index += "</div>\n";
  index += "</body>\n";
  index += "</html>";
  server.send(200, "text/html", index);
  digitalWrite(led, LOW);
}

void handleScripts() {
  String scripts = "function getState(){\n";
  scripts += " var data = new FormData();\n";
  scripts += " var request = new XMLHttpRequest();\n";
  scripts += " var obj = document.getElementById('switch-label');\n";
  scripts += " var label = document.querySelector('.switch-button__label');\n";
  scripts += " label.style.setProperty('animation','');\n";
  scripts += " if(obj.checked==true){\n";
  scripts += "  data.set('state', true);\n";

  scripts += "  document.getElementById('td_status').innerHTML=\"<h1>El Ventilador ahora está: </h1><div><img id='status_img' src='data:image/jpeg;base64,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' style='width:50%'/></div>\";\n";
  scripts += " }else {\n";
  scripts += " data.set('state', false);\n";
  scripts += " document.getElementById('td_status').innerHTML=\"<h1>El Ventilador ahora está: </h1><div><img id='status_img' src='data:image/jpeg;base64,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' style='width:50%'/></div>\"; }\n";
  scripts += " document.getElementById('td_switch_btn').style.marginLeft='50px';\n";
  scripts += " request.open('POST', '/changeState',true);\n";
  scripts += " request.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');\n";
  scripts += " request.send(data);\n}";
  server.send(200, "text/javascript", scripts);
}

void handleStyles() {
  String styles = "body { font-family: Arial, Helvetica, Sans-Serif; color: #fff; background-color: var(--color-black); }\n";
  styles += ".container {margin: auto; padding: 1em 0; border-radius: 1em; width: fit-content; background: #ffffff1c; text-align: center; box-sizing: border-box;}\n";
  styles += ":root {--color-green: #00a878;--color-red: #fe5e41;--color-button: #fdffff;--color-black: #000;}\n";
  styles += ".switch-button {display: inline-block; }\n";
  styles += ".switch-button .switch-button__checkbox {display: none;}\n";
  styles += ".switch-button .switch-button__label {background-color: var(--color-red);  width: 15rem;  height: 3rem;  border-radius: 3rem;  display: inline-block;  position: relative;}\n";
  styles += ".switch-button .switch-button__label:before {transition: .2s;  display: block;  position: absolute;  width: 3rem;  height: 3rem;  background-color: var(  --color-button);  content: '';  border-radius: 50%;    box-shadow: inset 1px 0px 10px 1px var(--color-black); }\n";
  styles += ".switch-button .switch-button__checkbox:checked + .switch-button__label {background-color: var(--color-green);}\n";
  styles += ".switch-button .switch-button__checkbox:checked + .switch-button__label:before { transform: translateX(12rem); }\n";
  styles += "#div_title {margin-left:20px; margin-right:20px}\n";
  styles += "@keyframes glow { from { background-color: var(--color-red); } to {background-color: var(--color-green);} }";
  server.send(200, "text/css", styles);
}
void handleSolenoid(boolean state){
  switch(state){
    case true:  // TRUE state turns ON solenoid
    digitalWrite(rele, LOW);
    break;
    case false: // FALSE state turns OFF solenoid
    digitalWrite(rele, HIGH);
    break;
  }  
}

void handlePOST() {
  if (server.method() != HTTP_POST) {
    digitalWrite(led, HIGH);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, LOW);
  } else {
    digitalWrite(led, HIGH);
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      // Change the state of the solenoid
      if (server.argName(i).indexOf("true") >= 0 || server.arg(i).indexOf("true") >= 0) {        
        message += "arg: " + server.arg(i) + "EOF\n";
        handleSolenoid(true); // turns ON solenoid
        break;
      } else if (server.argName(i).indexOf("false") >= 0 || server.arg(i).indexOf("false") >= 0) {        
        message += "arg: " + server.arg(i) + "EOF\n";
        handleSolenoid(false); // turns OFF solenoid
        break;
      }
    }
    server.send(200, "text/plain", message);
    digitalWrite(led, LOW);
  }
}

void handleNotFound() {
  digitalWrite(led, HIGH);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, LOW);
}

// Some graphics improvements for the UI

// Paint Functions for SVG images
void drawLocked() {
  String img = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"512\" height=\"512\">\n";
  img += "<g transform=\"translate(0.000000,512.000000) scale(0.100000,-0.100000)\" fill=\"#000000\" stroke=\"none\">\n";
  img += "<path d=\"M2400 4840 c-328 -46 -603 -182 -823 -409 -185 -191 -302 -412 -359 -681 -16 -77 -21 -148 -25 -376 l-5 -282 -117 -5 c-64 -3 -133 -11 -153 -17\n";
  img += "-68 -24 -130 -80 -168 -154 l-35 -69 0 -1206 0 -1206 26 -55 c37 -80 72 -115 147 -152 l67 -33 1605 0 1605 0 67 33 c75 37 110 72 147 152 l26 55 0 1206 0\n";
  img += "1206 -35 69 c-38 74 -100 130 -168 154 -20 6 -89 14 -153 17 l-117 5 -5 282 c-4 234 -9 297 -26 377 -61 281 -184 510 -378 701 -198 194 -441 322 -713 373\n";
  img += "-103 20 -320 27 -410 15z m375 -684 c199 -63 368 -222 445 -416 40 -101 50 -187 50 -426 l0 -224 -710 0 -710 0 0 224 c0 300 19 393 108 541 45 75 160\n";
  img += "189 235 234 67 39 168 78 241 91 89 16 251 5 341 -24z m1443 -1149 c23 -15 54 -46 69 -69 l28 -42 3 -1195 c3 -1321 6 -1242 -61 -1311 -17 -18 -50 -41 -72\n";
  img += "-51 -38 -18 -111 -19 -1625 -19 -1516 0 -1587 1 -1625 19 -51 23 -93 65 -116 116 -18 38 -19 97 -19 1228 0 974 2 1193 13 1220 21 48 62 93 110 116 l42 21\n";
  img += "1606 -2 1605 -3 42 -28z\"/>\n";
  img += "</g>\n</svg>";
  server.send(200, "image/svg+xml", img);
}

// Initializing server
void setup() {
  pinMode(led, OUTPUT);  // put led to output state
  pinMode(rele, OUTPUT); // rele is on GPIO 0
  //digitalWrite(rele, LOW);
  digitalWrite(led, HIGH);
  Serial.begin(115200);  // put serial speed transmission

  // Configuring Wifi AccessPoint
  Serial.println();
  Serial.print("Configuring client mode...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //WiFi.softAP(ssid, password);  // SSID and Password
  Serial.println();
  //IPAddress myIP = WiFi.softAPIP();  // Get local IP (192.168.4.1)
  //Serial.print("Web server address: http://");
  //Serial.println(myIP);
  Serial.println("Conectado a la WiFi...");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado con IP: ");
  Serial.println(WiFi.localIP());

  // Setup the DNS server redirecting all the domains to the AccessPoint IP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", WiFi.localIP());

  // Setup MDNS server
  // http://fan.local
  if (MDNS.begin("fan")) {
    Serial.println("MDNS iniciado: http://fan.local");
    MDNS.addService("http", "tcp", 80);  // add service to MDNS-SD
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  // Handle Auth Realm
  ArduinoOTA.begin();

  // The web page structure
  server.on("/", []() {  // Serves the root with auth
    handleRoot();
    /*if (!server.authenticate(www_username, www_password)) {
      //Basic Auth Method with Custom realm and Failure Response
      //return server.requestAuthentication(BASIC_AUTH, www_realm, authFailResponse);
      //Digest Auth Method with realm="Login Required" and empty Failure Response
      //return server.requestAuthentication(DIGEST_AUTH);
      //Digest Auth Method with Custom realm and empty Failure Response
      //return server.requestAuthentication(DIGEST_AUTH, www_realm);
      //Digest Auth Method with Custom realm and Failure Response
      return server.requestAuthentication(DIGEST_AUTH, www_realm, authFailResponse);
    } else {
      handleRoot();
    }*/
  });
  server.on("/styles.css", handleStyles);
  server.on("/scripts.js", handleScripts);  // Serves the javascript file
  server.on("/changeState", handlePOST);    // Handle POST request
  // Serves the SVG
  //server.on("/locked.png", drawCLocked);      // paint locked svg
  //server.on("/unlocked.png", drawCUnlocked);  // paint unlocked svg
  server.on("/favicon.ico", drawLocked);      // paint locked svg
  server.onNotFound(handleNotFound);          // Handle 404-Error
  server.begin();                             // Start webserver
  Serial.println("HTTP server started");
  digitalWrite(led, LOW);
}

// Lookpup for changes on server
void loop() {
  ArduinoOTA.handle();
  // MDNS
  unsigned int wifi_status = WiFi.status();
  //Serial.print("Network status: ");
  //Serial.println(wifi_status);
  if (wifi_status == WL_CONNECTED) {
    //Serial.println("Conexion establecida...\nActualizando entradas en el DNS");
    MDNS.update();
  } else if (wifi_status == WL_NO_SSID_AVAIL) {
    Serial.println("Desconectando de la red!");
    WiFi.disconnect();
  }
  // DNS
  dnsServer.processNextRequest();
  // HTTP
  server.handleClient();  
}
