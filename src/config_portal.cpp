#include "config_portal.h"



Preferences preferences;

const byte DNS_PORT = 53;
IPAddress apIP(8,8,4,4); // The default android DNS
DNSServer dnsServer;
AsyncWebServer server(80);

const char* PARAM_INPUT_SSID1 = "wifissid1";
const char* PARAM_INPUT_PASS1 = "wifipass1";
const char* PARAM_INPUT_EMAILADD = "emailadd";


const char index_html[] PROGMEM  = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Device Setup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  body{color:white; background-color:#141414; font-family: Helvetica, Verdana, Arial, sans-serif}
  h1{text-align:center;}
  p{text-align:center;}
  div{margin: 5%; background-color:#242424; padding:10px; border-radius:8px;}
  br{display: block; margin: 10px 0; line-height:22px; content: " ";}
  label{text-align:left; padding:2%;}
  input{border-radius: 4px; border: 2px solid #0056a8; width:90%; padding:10px; display:block; margin-right:auto; margin-left:auto;}
  input[type="submit"]{font-size: 25px; background-color:#0056a8; color:white; border-radius:8px; height:50px; width:95%;}
  </style>
  </head><body>
  <div>
    <h1>Device Setup</h1>
    <p>Please enter the following details to setup your device:</p>
    <form action="/get">
      <label>Wi-Fi Details</label>
      <br>
      <input type="text" name="wifissid1" id="wifissid1" placeholder="Wi-Fi SSID">
      <br>
      <input type="password" name="wifipass1" id="wifipass1" placeholder="Wi-Fi Password">
      <br>

      <br>
      <label for="emailadd">Email Address</label>
      <br>
      <input type="email" name="emailadd" id="emailadd">

      <br>
      <input type="submit" value="Submit">
    </form><br>
  </div>
</body></html>)rawliteral";



String getWifiSSID(){
  return preferences.getString("ssid1", "");
}

String getWifiPass(){
  return preferences.getString("pass1", ""); 
}

String getOwner(){
  return preferences.getString("email", ""); 
}

int getReboots(){
  return preferences.getInt("reboots",0);
}

void prefsBegin(){
  preferences.begin("credentials", false);
}

void setReboots(int num){
  preferences.putInt("reboots", num); 
}

void ResetPrefs(){
  preferences.begin("credentials", false);
  preferences.clear();
  preferences.end();
}

void webserverloop() {
  digitalWrite(LED_BUILTIN, LOW); 
  delay(100);
  
  //Serial.println( webserverloopCounter );
  digitalWrite(LED_BUILTIN, HIGH); 
  dnsServer.processNextRequest();
  delay(200);
  
  
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}


void setupCaptivePortal(){
  Serial.println("Setting up Captive Portal ");


    WiFi.mode(WIFI_AP);
    WiFi.softAP("Wifi-SoundMeter");//("+String(deveui)+")");
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);


    // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.print("/get");
      String inputMessage;
      String inputParam;
      String ssid;
      String pass;
      String email;

      //List all parameters
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isFile()){ //p->isPost() is also true
          Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else if(p->isPost()){
          Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } else {
          Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }



      //preferences.begin("credentials", false);
      // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
      if (request->hasParam(PARAM_INPUT_SSID1)) {
        Serial.print("write to prefs:");
        inputMessage = request->getParam(PARAM_INPUT_SSID1)->value();
        preferences.putString("ssid1", inputMessage); 
        Serial.print("SSID:");
        Serial.println(inputMessage);
        ssid = inputMessage;
        inputParam = PARAM_INPUT_SSID1;
        //Serial.println("read from prefs");
        //Serial.println(preferences.getString("ssid1", "")); 
      }
      // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
      if (request->hasParam(PARAM_INPUT_PASS1)) {
        Serial.print("write to prefs:");
        inputMessage = request->getParam(PARAM_INPUT_PASS1)->value();
        preferences.putString("pass1", inputMessage); 
        Serial.print("PASS:");
        Serial.println(inputMessage);
        inputParam = PARAM_INPUT_PASS1;
        pass = inputMessage;
      }

      // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
      if (request->hasParam(PARAM_INPUT_EMAILADD)) {
        Serial.print("write to prefs:");
        inputMessage = request->getParam(PARAM_INPUT_EMAILADD)->value();
        preferences.putString("email", inputMessage); 
        Serial.print("EMAIL:");
        Serial.println(inputMessage);
        inputParam = PARAM_INPUT_EMAILADD;
        email = inputMessage;
      }




      



      //else {
      //  inputMessage = "No message sent";
      //  inputParam = "none";
      //}
      preferences.end();

      //Serial.println(ssid);
      //Serial.println(pass);
      //Serial.println(email);
      
      request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                      + inputParam + ") with value: " + inputMessage +
                                      "<br><a href=\"/\">Return to Home Page</a>");
      //delay(5000);
      esp_restart();
    });


    // Send web page with input fields to client
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
    });


    server.onNotFound(notFound);
    server.begin();
  
}
