/** 
 * Define your configuration settings here
 * 
 */

#ifndef _CONFIG_h /* Prevent loading library twice */
  #define _CONFIG_h

  // resets prefs on boot (FOR TESTING ONLY)
  #define PREFSRESET 0

  // define in milliseconds, how often a message will be sent
  #define CYCLETIME   1500 //150  // use 150 for RIVM and sensor.community project

  // define wifi
  #define WIFISSID1 ""
  #define WIFIPASS1 ""

  // define if device has oled screen
  // #define HAS_OLED

  #define SENSORENDPOINT "https://ttn.devpoc.nl/update-http-sensor-test"

  // specify what board.
  //#define ARDUINO_ESP32_DEVKIT_V1 
  //#define ARDUINO_TTGO_LoRa32_V1

#endif
