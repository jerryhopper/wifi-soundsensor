#ifndef _BOARDCONFIG_h /* Prevent loading library twice */
#define _BOARDCONFIG_h


// pin config MEMS microphone
#if defined( ARDUINO_ESP32_DEVKIT_V1 )
  // define IO pins TTGO LoRa32 V1 for I2S for MEMS microphone
  #define i2s_pin_bck_io_num 4
  #define i2s_pin_ws_io_num 2
  #define i2s_pin_data_out_num -1 
  #define i2s_pin_ata_in_num 15
  // Channel selection 
  #define i2s_config_channel_format I2S_CHANNEL_FMT_ONLY_LEFT

  #define BUTTON_PIN 21
  #define TOUCH_PIN 21

#elif defined( ARDUINO_TTGO_LoRa32_V1 )
  // define ARDUINO_ESP32_DEV for I2S for MEMS microphone
  #define i2s_pin_bck_io_num 13
  #define i2s_pin_ws_io_num 12
  #define i2s_pin_data_out_num -1 
  #define i2s_pin_ata_in_num 35 //changed to 35, (21 is used by i2c)
  // Channel selection 
  #define i2s_config_channel_format I2S_CHANNEL_FMT_ONLY_LEFT

  #define BUTTON_PIN 21
  #define TOUCH_PIN 21
#else
  #error Unsupported board selection.
#endif







#endif