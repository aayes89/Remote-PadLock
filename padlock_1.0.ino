/* Servo controller with WiFi AP CaptivePortal and Authentication service for Arduino Wemos D1 module by Slam
   E-mail: aayes89@gmail.com
   (21/09/2022)
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <Stepper.h>

#ifndef STASSID
#define STASSID "HomePADLock"
#define STAPSK "1234567890"
#define USR "administrator"
#define PASS "Admin1str@t0r."
#define REALM "Login"
#define AERROR "<html><head><title>My Locker</title><link rel='stylesheet' href='/styles.css'></head><body><div class='container'><h1>Incorrect login</h1></div></body></html>"
#define WPORT 80
#define DPORT 53
#define PIN_LED 15
#endif

// Web Auth
const char* www_username = USR;
const char* www_password = PASS;

// allows you to set the realm of authentication Default:"Login Required"
const char* www_realm = REALM;
// the content of the HTML response in case of unauthorized access Default: empty
String authFailResponse = AERROR;

// One time reset flag
boolean isReset = false;
// PIN LED
const int led = PIN_LED;
// SSID
const char* ssid = STASSID;
// AP Pass
const char* password = STAPSK;

// DNS server
const byte DNS_PORT = DPORT;
DNSServer dnsServer;

// Web server on port 80
ESP8266WebServer server(WPORT);

// Variables for Stepper Drivers
const int stepsPerRevolution = 200;  // number of steps per revolution
const int retraso = 1000;
Stepper myStepper(stepsPerRevolution, D4, D5, D6, D7);

// Send data to Arduino to control the stepper motor
void handleMotor(int state) {
  switch (state) {
    case 1: // left orientation
      Serial.println("counterclockwise");
      myStepper.step(stepsPerRevolution);
      delay(retraso);
      break;
    case 2: // right orientation
      Serial.println("clockwise");
      myStepper.step(-stepsPerRevolution);
      delay(retraso);
      break;
    default: // stop the motor (do nothing)
      Serial.print("stop");
      Serial.println("");
      break;
  }
}


// Manage server behavior
void handleRoot() {
  digitalWrite(led, HIGH);
  String index = "<!DOCTYPE html>\n";
  index += "<html>\n";
  index += "<head>\n";
  index += " <meta charset='utf-8'>\n";
  index += " <meta http-equiv='X-UA-Compatible' content='IE=edge'>\n";
  index += " <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
  index += " <title>My Locker</title>\n";
  index += " <script src='/scripts.js'></script>\n";
  index += " <link rel='stylesheet' href='/styles.css'>\n";
  index += " <link rel='icon' href='/favicon.ico' type='image/png' />\n";
  index += "</head>\n";
  index += "<body>\n";
  index += " <div class='container'>\n";
  index += "  <div id='div_title'> <h1>Swipe to change lock state</h1> </div>\n";
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
  scripts += "  document.getElementById('td_status').innerHTML=\"<h1>The PADLOCK is now: </h1><div><img id='status_img' src='data:image/png;base64,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' style='width:50%'/></div>\";\n";
  scripts += " }else {\n";
  scripts += "  data.set('state', false);\n";
  scripts += "  document.getElementById('td_status').innerHTML=\"<h1>The PADLOCK is now: </h1><div><img id='status_img' src='data:image/png;base64,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' style='width:50%'/></div>\"; }\n";
  scripts += "  document.getElementById('td_switch_btn').style.marginLeft='50px';\n";
  scripts += "  request.open('POST', '/changeState',true);\n";
  scripts += "  request.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');\n";
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

void handlePOST() {
  if (server.method() != HTTP_POST) {
    digitalWrite(led, HIGH);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, LOW);
  } else {
    digitalWrite(led, HIGH);
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      // Move the servo or stepper motor
      if (server.argName(i).indexOf("true") >= 0 || server.arg(i).indexOf("true") >= 0) {
        message += "rotate servo 180°\n";
        message += "argName: " + server.argName(i) + "\n";
        message += "arg: " + server.arg(i) + "EOF\n";
        handleMotor(2); // send right orientation
		break;
      } else if (server.argName(i).indexOf("false") >= 0 || server.arg(i).indexOf("false") >= 0) {
        message += "rotate servo -180°\n";
        message += "argName: " + server.argName(i) + "\n";
        message += "arg: " + server.arg(i) + "EOF\n";
        handleMotor(1); // send left orientation
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

void handleOneTimeReset() {
  if (!isReset) {
    ESP.reset();
    delay(1000);
    isReset = true;
  }
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
  digitalWrite(led, HIGH);
  Serial.begin(115200);  // put serial speed transmission

  // set the speed at 60 rpm:
  myStepper.setSpeed(60);

  // Configuring Wifi AccessPoint
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAP(ssid, password);  // SSID and Password
  Serial.println();
  IPAddress myIP = WiFi.softAPIP();  // Get local IP (192.168.4.1)
  Serial.print("Web server address: http://");
  Serial.println(myIP);

  // Setup the DNS server redirecting all the domains to the AccessPoint IP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", myIP);

  // Setup MDNS server
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
    MDNS.addService("http", "tcp", 80);  // add service to MDNS-SD
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  // Handle Auth Realm
  ArduinoOTA.begin();

  // The web page structure
  server.on("/", []() {  // Serves the root with auth
    if (!server.authenticate(www_username, www_password)) {
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
    }
  });
  server.on("/styles.css", handleStyles);
  server.on("/scripts.js", handleScripts);  // Serves the javascript file
  server.on("/changeState", handlePOST);    // Handle POST request
  // Serves the SVG
  //server.on("/locked.png", drawCLocked);      // paint locked svg
  //server.on("/unlocked.png", drawCUnlocked);  // paint unlocked svg
  server.on("/favicon.ico", drawLocked);      // paint locked svg
  server.onNotFound(handleNotFound);          // Handle 404-Error
  server.on("/reset", handleOneTimeReset);
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
    Serial.println("Connection established...\nUpdating DNS entries");
    MDNS.update();
  } else if (wifi_status == WL_NO_SSID_AVAIL) {
    Serial.println("Disconnecting from the network!");
    WiFi.disconnect();
  }
  // DNS
  dnsServer.processNextRequest();
  // HTTP
  server.handleClient();
}

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
