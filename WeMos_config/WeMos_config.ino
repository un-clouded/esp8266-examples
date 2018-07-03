/*

Configure WiFi network settings via serial.

*/

#include <EEPROM.h>
#include <ESP8266WiFi.h>



#define  send  Serial.print



class Settings
{
 public:

  char  hostname [10+1];  // `+1` for terminating NUL
  char  network_name [30+1];
  char  passphrase [30+1];

  static const uint16_t  address_for_hostname = 0;
  static const uint16_t  address_for_network_name = sizeof (hostname);
  static const uint16_t  address_for_passphrase = address_for_network_name + sizeof (network_name);


  void  load ()
  {
    load (address_for_hostname,     sizeof (hostname),     hostname);
    load (address_for_network_name, sizeof (network_name), network_name);
    load (address_for_passphrase,   sizeof (passphrase),   passphrase);
  }


  void  load (uint16_t address, uint16_t length, char *value)
  {
    for (int  a = address;  a < address + length;  a += 1)
    {
      *value = EEPROM.read (a);
      value += 1;
    }
  }


  void  save ()
  {
    save (hostname,     sizeof (hostname),     address_for_hostname);
    save (network_name, sizeof (network_name), address_for_network_name);
    save (passphrase,   sizeof (passphrase),   address_for_passphrase);
  }


  void  save (char *value, uint16_t length, uint16_t address)
  {
    for (int  a = address;  a < address + length;  a += 1)
    {
      EEPROM.write (a, *value);
      value += 1;
    }
    EEPROM.commit ();
  }

}
settings;



static void  show_help ()
{
  send ("?       Help\r\n");
  send ("N       Network scan\r\n");
  send ("s       Show current settings\r\n");
  send ("h name  Set hostname\r\n");
  send ("n name  Set network name\r\n");
  send ("p pass  Set passphrase\r\n");
  send ("c       Connect\r\n");
  send ("r       Show RSSI\r\n");
  send ("S       Save settings");
}



static void  network_scan ()
{
  int8_t  networks = WiFi.scanNetworks ();

  for (uint8_t  nwid = 0;  nwid < networks;  nwid += 1)
  {
    send ("SSID: ");  send (WiFi.SSID (nwid));
    send ("  RSSI: ");  send (WiFi.RSSI (nwid));
    send ("  Chan: ");  send (WiFi.channel (nwid));
    send ("\r\n");
  }

  WiFi.scanDelete ();
}



static void  show_settings ()
{
  send ("hostname: \"");  send (settings.hostname);
  send ("\"\r\nnetwork_name: \"");  send (settings.network_name);
  send ("\"\r\npassphrase: \"");  send (settings.passphrase);  send ('"');
}



static void  set_hostname (char *hostname)
{
  strncpy (settings.hostname, hostname, sizeof (settings.hostname));
}



static void  set_network_name (char *network_name)
{
  strncpy (settings.network_name, network_name, sizeof (settings.network_name));
}



static void  set_passphrase (char *passphrase)
{
  strncpy (settings.passphrase, passphrase, sizeof (settings.passphrase));
}



static void  connect ()
{
  static const uint8_t  timeout = 10; // seconds

  send ("Connected: ");  send (WiFi.isConnected ());  send ("\r\n");
  // Times out with `status` `6` if try to connect while already connected
  if (WiFi.isConnected ())
    WiFi.disconnect ();

  send ("Setting hostname to \"");  send (settings.hostname);  send ("\"\r\n");
  WiFi.hostname (settings.hostname);

  send ("Connecting to \"");  send (settings.network_name);
  send ("\" with passphrase \"");  send (settings.passphrase);  send ("\"\r\n");
  WiFi.begin (settings.network_name, settings.passphrase);

  uint8_t  t;
  for (t = 0;  t < timeout;  t += 1)
  {
    if (WiFi.status () == WL_CONNECTED) break;
    send (".");
    delay (1000);
  }
  if (t < timeout)
  {
    send ("\r\nIP address: ");  send (WiFi.localIP ());
  }
  else {
    send ("\r\nTimed out.  Not connected.  Status: ");  send (WiFi.status ());
  }
}



static void  show_RSSI ()
{
  send (WiFi.RSSI ());
}



class Console
{
  char  request [2 + sizeof(settings.passphrase) + 1];
  int   cursor;


  static void  prompt ()
  {
    // A line terminator should be sent before the prompt so that commands need
    // not finish with a newline
    send ("\r\n> ");
  }


  static void  erase ()
  {
    send ("\b \b");
  }


  static void  bell ()
  {
    send ('\a');
  }


  void  handle_request ()
  {
    char *parameter = &request[2];
    char  command = request[0];
    switch (command)
    {
    case '?': show_help (); break;
    case 'N': network_scan (); break;
    case 's': show_settings (); break;
    case 'h': set_hostname (parameter); break;
    case 'n': set_network_name (parameter); break;
    case 'p': set_passphrase (parameter); break;
    case 'c': connect (); break;
    case 'r': show_RSSI (); break;
    case 'S': settings.save (); break;
    default:
      send ("Unrecognised command.  ? for help");
    }
  }


 public:

  void  setup ()
  {
    Serial.begin (115200);
    cursor = 0;
  }


  void  poll ()
  {
    while (Serial.available ())
    {
      char  char_received = (char) Serial.read();

      switch (char_received)
      {
      case '\r':
        // The request should be NUL terminated
        request[ cursor] = '\0';
        // Any output sent by the command should be on a new line
        send ("\r\n");
        handle_request ();
        prompt ();
        cursor = 0;
        break;

      case 127: // backspace
        if (0 < cursor)
        {
          erase ();
          cursor -= 1;
        }
        else
          bell ();
        break;

      default:
        if (cursor < sizeof (request) - 1) // "- 1" to allow space for NUL
        {
          request [cursor] = char_received;
          cursor += 1;
          // The character should be echoed so that the user can see what
          // they're typing
          send (char_received);
        }
        else
          bell ();
      }
    }
  }

}
console;



void  setup()
{
  EEPROM.begin(512);
  settings.load ();
  console.setup ();
}



void  loop ()
{
  console.poll ();
}

