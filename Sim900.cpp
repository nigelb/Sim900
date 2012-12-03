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

#include "Sim900.h"

Sim900::Sim900(SoftwareSerial* serial, int baud_rate, int powerPin, int statusPin)
{
	_serial = serial;	
	_powerPin = powerPin;
	_statusPin = statusPin;
	_lock = 0;
	serial->begin(baud_rate);
}

Sim900::Sim900(HardwareSerial* serial, int baud_rate, int powerPin, int statusPin)
{
	_serial = serial;	
	_powerPin = powerPin;
	_statusPin = statusPin;
	_lock = 0;
	serial->begin(baud_rate);
}

bool Sim900::isPoweredUp()
{
	return analogRead(_statusPin) > SIM900_POWERUP_THRESHOLD;
}

int Sim900::lock()
{
	if(_lock == 0)
	{
		Serial.println("Locked....");
		_lock = 1;
		return 1;
	}
	return -1;
}

int Sim900::unlock()
{
	if(_lock == 1)
	{
		Serial.println("Unlocked....");
		_lock = 0;
	}
	return 1;
}

bool Sim900::compare(char to_test[], char target[], int pos, int length, int test_len)
{
	int s = pos - length;
	if(s < 0){s = 0;}
	for(int i = 0; i < length; i++)
	{
		if(to_test[(i+s) % test_len] != target[i])
		{
			return false;
		}
	}
	return true;
}

int Sim900::waitFor(char target[], bool dropLastEOL, String* data)
{
	int len = strlen(target);
	int test_len = len;
	if(test_len < 5)
	{
		test_len = 5;
	}
	int pos = 0;
	char tmp[test_len + 1];
	tmp[test_len] = NULL;
	char _tmp;
	bool concat = data != NULL;
	while(!compare(tmp, target, pos, len, test_len))
	{
		if(compare(tmp, "ERROR", pos, 5, test_len))
		{
			return false;
		}
		if(_serial->available())
		{
			_tmp = _serial->read();
			if(concat){
				data->concat(_tmp);
			}
			tmp[pos++ % test_len] = _tmp;
		}
	}
	if(dropLastEOL)
	{
		dropEOL();
	}
	return true;

}

bool Sim900::dropEOL()
{
	while(_serial->available() && (_serial->peek() == 10 || _serial->peek() == 13))
	{
		_serial->read();
	}
}

void Sim900::powerToggle()
{
  pinMode(_powerPin, OUTPUT); 
  digitalWrite(_powerPin, LOW);
  delay(1000);
  digitalWrite(_powerPin, HIGH);
  delay(2000);
  digitalWrite(_powerPin, LOW);
  delay(3000);
}

bool Sim900::powerUp()
{
	if(!isPoweredUp())
	{
		powerToggle();
		return waitFor("Call Ready", true, NULL);
	}
	return false;
}
bool Sim900::powerDown()
{
	if(isPoweredUp())
	{
		powerToggle();
		return waitFor("NORMAL POWER DOWN", true, NULL);
	}
	return false;
}

bool Sim900::getSignalQuality(int &strength, int &error_rate)
{
	if(lock())
	{
		_serial->write("AT+CSQ\r\n");
		_serial->flush();
		delay(100);
		String d;
		if(waitFor("OK", true, &d))
		{
			int _start = d.lastIndexOf("+CSQ");
			_start = d.indexOf(" ", _start) + 1;
			int split = d.indexOf(",", _start);
			int _end = d.indexOf("\r", _start);
			strength = d.substring(_start, split).toInt();
			error_rate = d.substring(split+1, _end).toInt();
			unlock();
			return true;
		}else
		{
			unlock();
		}
	}
	return false;
}

bool Sim900::issueCommand(char command[], char ok[], bool dropLastEOL)
{
	_serial->write(command);
	waitFor(ok, dropLastEOL, NULL);

}

void Sim900::dumpStream()
{
	while(_serial->available())
	{
		Serial.write(_serial->read());
	}
}

GPRSHTTP* Sim900::createHTTPConnection(CONN settings, char URL[])
{	lock();
	if(settings.contype != NULL)	
	{
		_serial->write("AT+SAPBR=3,");
		_serial->print(settings.cid, DEC);
		_serial->write(",\"CONTYPE\",\"");
		_serial->write(settings.contype);
		_serial->println("\"");
		waitFor("OK", true, NULL);
	//	delay(100);
	//	dumpStream();
	}
	if(settings.apn != NULL)	
	{
		_serial->write("AT+SAPBR=3,");
		_serial->print(settings.cid, DEC);
		_serial->write(",\"APN\",\"");
		_serial->write(settings.apn);
		_serial->println("\"");
		waitFor("OK", true, NULL);
	//	delay(100);
	//	dumpStream();
	}

	return new GPRSHTTP(this, settings.cid, URL);
}

GPRSHTTP::GPRSHTTP(Sim900* sim, int cid, char URL[])
{
	_sim = sim;
	_cid = cid;
	url = URL;
}

bool GPRSHTTP::init()
{
	//Shutdown the connection first.
	_sim->_serial->write("AT+SAPBR=0,");
	_sim->_serial->println(_cid, DEC);
	delay(1000);
	if(!_sim->waitFor("OK", true, NULL))
	{
		Serial.println("Not Shutdown.")	;
	}
	
	//Start the connection.	
	_sim->_serial->write("AT+SAPBR=1,");
	_sim->_serial->println(_cid, DEC);
	delay(1000);
	if(!_sim->waitFor("OK", true, NULL))
	{
		return false;
	}
	Serial.println("Connected!");
	
	//Initilize the HTTP Application context.
	_sim->_serial->println("AT+HTTPINIT");
	if(!_sim->waitFor("OK", true, NULL))
	{
		return false;
	}
	Serial.println("HTTP Initilized!");

	//Set the URL
	_sim->_serial->write("AT+HTTPPARA=\"URL\",\"");
	_sim->_serial->write(url);
	_sim->_serial->println("\"");
	if(!_sim->waitFor("OK", true, NULL))
	{
		return false;
	}
	Serial.print("URL: "); Serial.println(url);
	return true;
}

//bool GPRSHTTP::
int GPRSHTTP::write(char* data)
{
	return _data.concat(data);
}
int GPRSHTTP::write(int data)
{
	return _data.concat(data);
}
int GPRSHTTP::write(String data)
{
	return _data.concat(data);
}
int GPRSHTTP::print(char* data){
	return _data.concat(data);
}
int GPRSHTTP::print(int data){
	return _data.concat(data);
}
int GPRSHTTP::print(String data){
	return _data.concat(data);
}
int GPRSHTTP::println(){
	_data.concat('\r');
	_data.concat('\n');
	return 2;
}
int GPRSHTTP::println(char* data){
	return _data.concat(data) + println();
}
int GPRSHTTP::println(int data){
	return _data.concat(data) + println();
}
int GPRSHTTP::println(String data){
	return _data.concat(data) + println();
}
int GPRSHTTP::post(int &cid, int &HTTP_CODE, int& length){
	int _length = _data.length()+1;
	_sim->_serial->write("AT+HTTPDATA=");
	_sim->_serial->print(_length, DEC);
	_sim->_serial->write(",");
	_sim->_serial->println(SIM900_HTTP_TIMEOUT, DEC);
	if(!_sim->waitFor("DOWNLOAD", true, NULL))
	{
		return false;
	}
	char d[length];
	_data.toCharArray(d, _length, 0);
	_sim->_serial->write((const uint8_t *)d, _length);
	if(!_sim->waitFor("OK", true, NULL))
	{
		return false;
	}
	_sim->_serial->write("AT+HTTPACTION=");
	_sim->_serial->println(POST, DEC);
	if(!_sim->waitFor("+HTTPACTION:", true, NULL))
	{
		return false;
	}
	String* tmp = new String();
	_sim->waitFor("\n", true, tmp);
	int _start = tmp->indexOf(",");
	cid = tmp->substring(0, _start).toInt();
	int _end = tmp->indexOf(",", _start+1);
	HTTP_CODE = tmp->substring(_start + 1, _end).toInt();
	_start = _end + 1;
	length = tmp->substring(_start).toInt();
	delete tmp;

	return true;
}

int GPRSHTTP::_read(char* data, int length)
{
	if(data != NULL)
	{
		Serial.println("Getting Data.");
		//ToDo: Read in blocks of size _SS_MAX_RX_BUFF
		_sim->_serial->print("AT+HTTPREAD=0,");
		_sim->_serial->println(length, DEC);
		if(!_sim->waitFor("+HTTPREAD:", true, NULL))
		{
			return false;
		}
		_sim->waitFor("\n", true, NULL);
		for(int i = 0; i < length; i++)
		{
			while(!_sim->_serial->available()){
				delay(200);
			}
			data[i] = _sim->_serial->read();
		}
		if(!_sim->waitFor("OK", true, NULL))
		{
			return false;
		}

	}
	return true;
}

bool GPRSHTTP::terminate()
{
	_sim->_serial->write("AT+SAPBR=0,");
	_sim->_serial->println(_cid, DEC);
	_sim->waitFor("OK", true, NULL);
	_sim->_serial->println("AT+HTTPTERM");
	_sim->waitFor("OK", true, NULL);
	_sim->unlock();
}

