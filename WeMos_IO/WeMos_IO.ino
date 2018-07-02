/*

The ADC seems to be a bit off.  Meter reads 2.909 V and ADC reads 881, which
should indicate 2.839 V (3.3 * 881 / 1024), so use 3.381 instead:

  2.909 = 3.381 * 881 / 1024

The meter shows 3.310 V on the 3V3 line.


## Plan

  - Be able to configure the hostname, network name and passphrase via serial
    and have it stored in EEPROM

  - SPI

  - PWM

*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>



// No, `0` does not refer to D0
const char  DIGITAL_PIN[] = {D0, D1, D2, D3, D4, D5, D6, D7, D8};

const char*  network_name = "";
const char*  network_passphrase = "";


ESP8266WebServer  server (80);
#define  BAD_REQUEST  400



static void  handle_adc ()
{
  // analogRead returns 0..1023 representing 0V to 3V3
  uint16_t  reading = analogRead (A0);
  server.send (200, "text/plain", String(reading));
}



// Configure the pin as OUTPUT and drive it either HIGH or LOW:
//
//  ?pin=0&level=0
//
// Configure the pin as INPUT and sense the level at the pin:
//
//  ?pin=8
//
static void  handle_gpio ()
{
  String    pin_param_value = server.arg ("pin");
  uint8_t   pin = pin_param_value.toInt ();
  uint16_t  response_code = 200;
  String    response = "";

  // If the "pin" parameter is missing or not a natural number between 0 and 8
  // then the request should be rejected
  if (pin_param_value.length () == 0  ||  8 < pin)
  {
    response_code = BAD_REQUEST;
    response = "Bad pin";
    goto respond;
  }

  pin = DIGITAL_PIN[ pin];

  // If the "level" parameter is absent then it is a SENSE request
  if (! server.hasArg ("level"))
  {
    pinMode (pin, INPUT);
    uint8_t  level = digitalRead (pin);
    response = String (level);
  }
  else {
    // If the level parameter is present but neither "0" nor "1" then the
    // request should be rejected
    String  level_param_value = server.arg ("level");
    if (level_param_value != "0"  &&  level_param_value != "1")
    {
      response_code = BAD_REQUEST;
      response = "Bad level";
      goto respond;
    }

    pinMode (pin, OUTPUT);
    digitalWrite (pin, (level_param_value == "0") ? LOW : HIGH);
  }

 respond:
  server.send (response_code, "text/plain", response);
}



void  setup ()
{
  Serial.begin (115200);

  WiFi.hostname ("thing01");
  WiFi.begin (network_name, network_passphrase);
  while (WiFi.status () != WL_CONNECTED)
  {
    delay (500);
    Serial.print (".");
  }
  Serial.println ();
  Serial.print ("IP address: ");
  Serial.println (WiFi.localIP ());

  server.on ("/adc", handle_adc);
  server.on ("/gpio", handle_gpio);

  server.begin ();
  Serial.println ("HTTP server started");
}



void  loop ()
{
  server.handleClient ();
}

