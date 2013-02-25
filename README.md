Sim900
=====
This is an Arduino library for the Sim900 GPRS Shield.
It relies on the 2.8v pin from the Sim900 being plugged into one of
the analog pins so that the library can determine the power state
of the modem.

To work with the Arduino Mega2560 the software serial pins need to be 
remapped to pins that support change detect (PCINT). With some Arduino 
jumper leads, map pin 7 to pin 10 and pin 8 to pin 11.


Example usage:
```cxx
/*
  Sim900 is an Arduino library for working with the Sim900 GRPS Shield
  Copyright (C) 2012  Nigel Bajema

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <SoftwareSerial.h>
#include <Sim900.h>


CONN settings;
// pins 10, and 11 are serial rx and tx pins for the modem
// pin 9 is the power toggle pin
// analog pin 8 is the GRPS power status pin
Sim900 modem(new SoftwareSerial(10, 11), 19200, 9, 8);
char url[] = "www.example.com";

void setup()
{
  Serial.begin(19200);             // the Serial port of Arduino baud rate.
  delay(2000);
  settings.cid = 1;
  settings.contype = "GPRS";
  settings.apn = "internet";
  test_modem();
  
}


void loop()
{
}

void test_modem()
{
  int cid = 0, code = 0, error_code = 0;  
  uint32_t  length = 0;
  
  if(modem.powerUp())
  {
      Serial.println("Powered Up") ;
      Serial.println("=================================================");
      int strength = -1, error_rate = -1;      
      if(modem.getSignalQuality(strength, error_rate))
      {
        Serial.print("Strength: ");
        Serial.print(strength);
        Serial.print(" Error Rate: ");
        Serial.println(error_rate);        
      }
      GPRSHTTP* con = modem.createHTTPConnection(settings, url);
      if(con == NULL)
      {
          Serial.println(get_error_message(modem.get_error_condition()));
          return;
      }
      if(con->init() && con->post_init(14))
      {
        con->println("Hello World!");
        if(con->post(cid, code, length))
        {
          Serial.print("HTTP CODE: ");
          Serial.println(code, DEC);  
          Serial.print("Length: ");   
          Serial.println(length, DEC);     
          if(con->init_retrieve())
          {     
            length = length + 1;    //Trigger an error condition.
            char data[length + 1];  //Ensure that this is a NULL Terminated String....
            data[length] = '\0';    //....
            if(con->read(data, length))
            {
              Serial.println("Retrieved Data:");
              Serial.println(data);
            }
            if(con->get_error_condition() == SIM900_ERROR_READ_LIMIT_EXCEEDED)
            {
                Serial.println("We tried to Read more characters from the connection than were available!");
            }
          }
        }
      }
      con->terminate();
      delete con;
      Serial.println("------------------------------------------------");
  }else
  {
     Serial.println("Powering up modem failed !"); 
  }
  modem.powerDown(); 
}
```
