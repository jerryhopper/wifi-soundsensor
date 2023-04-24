#include <Arduino.h>

#include "config.h"
#include "boards.h"

#include "soundsensor.h"
#include "measurement.h"
#include "base64.h"

#include <HTTPClient.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <WiFiClientSecure.h>

#include "config_portal.h"



// Variables will change:
int webserverloopCounter = 0;
bool configuredStatus = false;
int HoursOfflineUnconfigured = 1; // after 24hrs nonconnected, set unconfigured status.

String ssid; // The SSID to connect with.
String pass; // The password for wifi
String email;// The mailadress associated with this device.
String email64;
int reboots; // each wificonnect attempt before reboot takes ~3minutes.






WiFiClientSecure client;
HTTPClient http;


static char deveui[32];
String deveui64;
static int cycleTime = CYCLETIME;
static bool networkOk = false;

// WiFi connect timeout per AP. Increase when connecting takes longer.
const uint32_t connectTimeoutMs = 10000;

// Task 1 is the default ESP core 1, this one handles the sending of the messages
// Task 0 is the added ESP core 0, this one handles the audio, (read MEMS, FFT process and compose message)
TaskHandle_t Task0;


// task semaphores
bool audioRequest = false;
bool audioReady = false;

// payloadbuffer, filled by core 0, read by core 1
char payload[80];
int payloadLength = 0;

// create soundsensor
static SoundSensor soundSensor;










// UrlDecode function
String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

// UrlEncode function
String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}


// h2int (needed for urlEncode, urlDecode)
unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}


//Task0code: handle sound measurements
void Task0code( void * pvParameters ){
  Serial.print("Task0 running on core ");
  Serial.println(xPortGetCoreID());

  //#if defined(ARDUINO_TTGO_LoRa32_V1)
  //  oled.begin(deveui);
  //#endif

  // Weighting lists
  static float aweighting[] = A_WEIGHTING;
  static float cweighting[] = C_WEIGHTING;
  static float zweighting[] = Z_WEIGHTING;

  // measurement buffers
  static Measurement aMeasurement( aweighting);
  static Measurement cMeasurement( cweighting);
  static Measurement zMeasurement( zweighting);

  soundSensor.begin();

  // main loop task 0
  while( true){
    // read chunk form MEMS and perform FFT, and sum energy in octave bins
    float* energy = soundSensor.readSamples();

    // update
    aMeasurement.update( energy);
    cMeasurement.update( energy);
    zMeasurement.update( energy);
 
    // calculate average and compose message
    if( audioRequest) {
       audioRequest = false;

      aMeasurement.calculate();
      cMeasurement.calculate();
      zMeasurement.calculate();

      // debug info, should be comment out
      //aMeasurement.print();
      //cMeasurement.print();
      //zMeasurement.print();

  //#if defined(HAS_OLED)
  //    oled.showValues( aMeasurement, cMeasurement, zMeasurement, networkOk);
  //#endif

      composeMessage( aMeasurement, cMeasurement, zMeasurement);

      // reset counters etc.
      aMeasurement.reset();
      cMeasurement.reset();
      zMeasurement.reset();
      //printf("end compose message core=%d\n", xPortGetCoreID());
      audioReady = true;    // signal worker task that audio report is ready
    }
  }
}
// compose payload message
static bool composeMessage( Measurement& la, Measurement& lc, Measurement& lz) {
  
  // find max value to compress values [0 .. max] in an unsigned byte from [0 .. 255]
  float max = ( la.max > lc.max) ? la.max : lc.max;
  max = ( lz.max > max) ? lz.max : max;
  float c = 255.0 / max;
  int i=0;
  payload[ i++] = round(max);   // save this constant as first byte in the message
  
  payload[ i++] = round(c * la.min);
  payload[ i++] = round(c * la.max);
  payload[ i++] = round(c * la.avg);

  payload[ i++] = round(c * lc.min);
  payload[ i++] = round(c * lc.max);
  payload[ i++] = round(c * lc.avg);
 
  payload[ i++] = round(c * lz.min);
  payload[ i++] = round(c * lz.max);
  payload[ i++] = round(c * lz.avg);
  
  for ( int j = 0; j < OCTAVES; j++) {
    payload[ i++] = round(c * lz.spectrum[j]);
  }

  payloadLength = i;
  
  if ( payloadLength > 51)   // max message length
    printf( "message to big length=%d\n", payloadLength);  
}


void initwifi(){
    // We start by connecting to a WiFi network
    Serial.print("Connecting to network: ");
    //WiFi.begin(ssid, pass);


  Serial.println(ssid);
  //Serial.println(pass);
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(deveui);

  WiFi.begin(ssid.c_str(),pass.c_str());
  Serial.print("Connecting to WiFi ..");


    int teller = 1;
    while (WiFi.status()  != WL_CONNECTED  && teller < 13) {
        delay(1000);
        Serial.print(".");
        teller++;
    }

    if (WiFi.status()  == WL_CONNECTED){
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());  
      //digitalWrite(LEDX_PIN,LOW);
    }else{
      Serial.println("No wifi connected.");
      delay(1000);
    }
    
}

void wifiSendData(){
  if(WiFi.status()  == WL_CONNECTED){
      digitalWrite(LED_BUILTIN, LOW);  


      // Disable verification of certificate.
      client.setInsecure();

      // Your Domain name with URL path or IP address with path
      http.begin(client, SENSORENDPOINT );
      
      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      http.addHeader("Device-Id", deveui64 );
      http.addHeader("Owner", email64 );
      // Data to send with HTTP POST
      String httpRequestData = "deveui="+deveui64+"&payload="+urlencode(base64::encode(payload));

      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
      
            
      if (httpResponseCode>0) {
        Serial.print(" HTTP Response code: ");
        Serial.println(httpResponseCode);
        networkOk = true;
        // Returnstring:  ok
        // Returnstring:  cmd:rst
        // Returnstring:  cmd:wfc:5
      
        String responsepayload = http.getString();
        Serial.print("Response data: ");
        Serial.println(responsepayload);
        //Serial.print("Response length: ");
        //Serial.println(responsepayload.length());

        //Serial.print("substring data: ");
        //Serial.println(responsepayload.substring(4,7));
        
        if(responsepayload.substring(0,2) == "ok"){
          Serial.println("OK");
          
        }else if(responsepayload.substring(0,3) == "cmd"){
          //Serial.println("COMMAND");
          if(responsepayload.substring(4,7) == "wfc"){
            Serial.print("WIFICYCLE: ");
            cycleTime = responsepayload.substring(8,responsepayload.length()).toInt();
            Serial.println(cycleTime);
          }
          if(responsepayload.substring(4,7) == "rst"){
            Serial.println("RESTART!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1");
            esp_restart();
          }
          if(responsepayload.substring(4,7) == "clr"){
            Serial.println("CLEAR RESTART!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1");
            ResetPrefs();
            esp_restart();
          }
        }


      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      
      // Free resources
      http.end();
      
    }
    else {
      Serial.println("WiFi Disconnected");
      networkOk = false;
      initwifi();
    }
}







void soundmeterloop() { 
  // signal audiotask to compose an audio report
  audioRequest = true;  
  // wait for Task 0 to be ready
  while( !audioReady)
    delay(50);    
  audioReady = false;

  wifiSendData();
  Serial.print("DELAY: ");
  Serial.print(cycleTime);
  delay(cycleTime); 
}


bool scanNetworks(){
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("Wifi scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
    Serial.print(n);
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i)==ssid.c_str() ){
        Serial.println(" network found");
        return true;
        //Serial.print(WiFi.SSID(i)); // NAME!
      }
    }
  }
  Serial.println("");

  return false;
}


void setup() {

  Serial.begin(115200); 
  delay(1000);

  if(PREFSRESET==1){
    ResetPrefs();
  }
    

  //WiFiMulti wifiMulti;
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)

  pinMode(BUTTON_PIN, INPUT);
  //pinMode(LEDX_PIN,OUTPUT);

  //digitalWrite(LEDX_PIN,HIGH);
  //delay(15000);
  //digitalWrite(LEDX_PIN,HIGH);
  //delay(15000);
  //digitalWrite(LEDX_PIN,LOW);
  //delay(15000);

  prefsBegin();

  

  uint64_t chipid = ESP.getEfuseMac();   //The chip ID is essentially its MAC address(length: 6 bytes).
  sprintf(deveui, "WFSNDMTR_%08X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  deveui64 = base64::encode(deveui);
  printf("deveui=%s\n", deveui);

    

  Serial.println("end setup"); 


  
  
  delay(2000);


  // Read preferences.
  ssid = getWifiSSID(); //preferences.getString("ssid1", ""); 
  pass = getWifiPass(); //preferences.getString("pass1", ""); 
  email = getOwner();
  email64 = base64::encode(email);

  reboots = getReboots();
  
  Serial.print("Read SSID from prefs: '");
  Serial.print(ssid); 
  Serial.println("'");
  delay(2000);

  if( ssid == "" ){
    Serial.println("Preferences not set");
    configuredStatus = false;

  }else{
    configuredStatus = true;
    Serial.print("Read SSID from prefs: '");
    Serial.print(ssid); 
    Serial.println("'"); 
    Serial.print("Reboot count: " );
    Serial.print(reboots);
    Serial.print(" (");
    Serial.print(reboots*3);
    Serial.println(" minutes offline)");
    
    //20 reboots = 1 hour.
    if( reboots > (20 * HoursOfflineUnconfigured) ){
      // Reset to no config.
      Serial.println("Device will fallback in unconfigured status.");
      ResetPrefs();
      esp_restart();
    }

    delay(1000); 
  }
  

  if ( configuredStatus ){
    initwifi();
    //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
    xTaskCreatePinnedToCore(
                    Task0code,   /* Task function. */
                    "Task0",     /* name of task. */
                    40000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task0,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */    
  } else {
    setupCaptivePortal();
  }


  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(2,HIGH);
  delay(1000);
  digitalWrite(2,LOW);
  delay(1000);
  digitalWrite(2,HIGH);
  


  pinMode(25, OUTPUT);
  digitalWrite(25,HIGH);
  delay(2000);
  digitalWrite(25,LOW);
  delay(2000);
  digitalWrite(25,HIGH);
  delay(2000);
  digitalWrite(25,LOW);

}



void loop() {
  // main loop 
  //printf("send message len=%d core=%d\n", payloadLength, xPortGetCoreID());

  if(WiFi.status() == WL_CONNECTED){
    //digitalWrite(LEDX_PIN,LOW);
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    soundmeterloop();
    webserverloopCounter = 0;
    
    setReboots(0);


  }else{


    if(configuredStatus){
      // Device is configured, but not online
      initwifi();
      webserverloopCounter= webserverloopCounter + 1;

      if (webserverloopCounter>11){
        

        setReboots(reboots+1);
        digitalWrite(2,LOW);
        delay(1000);
        esp_restart();

      }

    }else{
      // Device needs configuration (Captive Portal)
      webserverloopCounter= webserverloopCounter + 1;

      // Wait for config, if wait too long, restart.
      if (webserverloopCounter<1000){
        digitalWrite(2,HIGH);
        webserverloop();  
        digitalWrite(2,LOW);
      }else{
        Serial.println("Restarting!");
        digitalWrite(2,LOW);
        delay(1000);      
        esp_restart();
      }
    } 

  }
}
