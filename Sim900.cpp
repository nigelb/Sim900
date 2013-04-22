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

void set_sim900_debug_mode(bool mode)
{
	SIM900_DEBUG_OUTPUT = mode;
}

void set_sim900_debug_stream(Stream* stream)
{
	SIM900_DEBUG_OUTPUT_STREAM = stream;
}

//
//Set the SIM900 input timeout in milliseconds
//
void set_sim900_input_timeout(unsigned long timeout)
{
	SIM900_INPUT_TIMEOUT  = timeout;
}

bool is_valid_connection_type(char* to_check)
{
	if(to_check == NULL)
	{
		return false;
	}
	for(uint32_t i = 0; VALID_CONNECTION_TYPES[i] != NULL; i++)
	{
		if(strcmp(VALID_CONNECTION_TYPES[i], to_check) == 0)	
		{
			return true;
		}
	}
	return false;
}

bool is_valid_connection_rate(char* to_check)
{
	uint32_t speed = String(to_check).toInt();
	for(uint32_t i = 0; VALID_CONNECTION_SPEEDS[i] != NULL; i++)
	{
		if(speed == VALID_CONNECTION_SPEEDS[i])
		{
			return true;
		}
	}
	return false;
}

char* get_error_message(int error_code)
{
	for(uint32_t i = 0; messages[i].code != SIM900_ERROR_LIST_TERMINATOR; i++)
	{
		if(messages[i].code == error_code)
		{
			return messages[i].message;
		}
	}
	return "Could not find error message.\0";//SIM900_UNKNOWN_ERROR_MESSAGE;
}

Sim900::Sim900(SoftwareSerial* serial, int baud_rate, int powerPin, int statusPin,  enum MODEM_VARIANT varient)
{
	_serial = serial;	
	_powerPin = powerPin;
	_statusPin = statusPin;
	_lock = 0;
	_error_condition = SIM900_ERROR_NO_ERROR;	
	_ser = serial;
	handle_varient(varient);
	serial->begin(baud_rate);
}

Sim900::Sim900(HardwareSerial* serial, int baud_rate, int powerPin, int statusPin,  enum MODEM_VARIANT varient)
{
	_serial = serial;	
	_powerPin = powerPin;
	_statusPin = statusPin;
	_lock = 0;
	_error_condition = SIM900_ERROR_NO_ERROR;	
	_ser = NULL;
	handle_varient(varient);
	serial->begin(baud_rate);
}

void Sim900::handle_varient(MODEM_VARIANT varient)
{
	this->varient = varient;
	switch(varient)
	{
	case VARIANT_1:
		max_http_post_size = SIM900_MAX_POST_DATA_V1;
		break;
	case VARIANT_2:
		max_http_post_size = SIM900_MAX_POST_DATA_V2;
		break;
	}
}

MODEM_VARIANT Sim900::get_varient()
{
	return varient;
}
uint32_t Sim900::get_max_http_post_size()
{
	return max_http_post_size;
}

void Sim900::set_error_condition(int error_value)
{
	_error_condition = error_value;
}

int Sim900::get_error_condition()
{
	return _error_condition;
}

bool Sim900::isPoweredUp()
{
	pinMode(_statusPin, INPUT);
	return analogRead(_statusPin) > SIM900_POWERUP_THRESHOLD;

}

bool Sim900::lock()
{
	if(_lock == 0)
	{
		if(SIM900_DEBUG_OUTPUT){
			SIM900_DEBUG_OUTPUT_STREAM->println("Locked....");
		}
		_lock = 1;
		return true;
	}
	set_error_condition(SIM900_ERROR_COULD_NOT_AQUIRE_LOCK);
	return false;
}

bool Sim900::unlock()
{
	if(_lock == 1)
	{
		if(SIM900_DEBUG_OUTPUT){
			SIM900_DEBUG_OUTPUT_STREAM->println("Unlocked....");
		}
		_lock = 0;
	}
	return true;
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
	return waitFor(target, dropLastEOL, data, SIM900_INPUT_TIMEOUT);
}

int Sim900::waitFor(char target[], bool dropLastEOL, String* data, unsigned long timeout)
{
	set_error_condition(SIM900_ERROR_NO_ERROR);	
	if(SIM900_DEBUG_OUTPUT){
		SIM900_DEBUG_OUTPUT_STREAM->print("Waiting for: ");
		SIM900_DEBUG_OUTPUT_STREAM->println(target);
	}
	unsigned long time = millis();
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
			if(SIM900_DEBUG_OUTPUT){SIM900_DEBUG_OUTPUT_STREAM->println("");SIM900_DEBUG_OUTPUT_STREAM->println("ERROR");}
			set_error_condition(SIM900_ERROR_MODEM_ERROR);
			return false;
		}
		if(_serial->available())
		{
			_tmp = _serial->read();
			if(SIM900_DEBUG_OUTPUT){
				SIM900_DEBUG_OUTPUT_STREAM->write(_tmp);
			}
			if(concat){
				data->concat(_tmp);
			}
			tmp[pos++ % test_len] = _tmp;
			time = millis();
		}else if((millis() - time) > timeout)
		{
			set_error_condition(SIM900_ERROR_TIMEOUT);
			if(SIM900_DEBUG_OUTPUT){
				SIM900_DEBUG_OUTPUT_STREAM->println("");
				SIM900_DEBUG_OUTPUT_STREAM->print("Timed out waiting for: ");
				SIM900_DEBUG_OUTPUT_STREAM->println(target);
			}
			return false;
		}
	}
	if(dropLastEOL)
	{
		dropEOL();
	}
	if(SIM900_DEBUG_OUTPUT){SIM900_DEBUG_OUTPUT_STREAM->println("");SIM900_DEBUG_OUTPUT_STREAM->println("Found it!");}
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
	if(SIM900_DEBUG_OUTPUT){
		SIM900_DEBUG_OUTPUT_STREAM->println("Powering up Modem!");
	}
	if(!isPoweredUp())
	{
		//if(_ser != NULL)
		//{
		//	_ser->listen();
		//}
		powerToggle();
		if(waitFor("Call Ready", true, NULL))
		{
			return true;
		}else
		{
			if(isPoweredUp())
			{
				powerToggle();
			}
		}

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

bool Sim900::waitForSignal(int iterations, int wait_time)
{
  int strength = -1, error_rate = -1, strength_count = 0;
  while(strength <= 0)
  {
    if(getSignalQuality(strength, error_rate))
    {
    	if(SIM900_DEBUG_OUTPUT){
    		SIM900_DEBUG_OUTPUT_STREAM->print("Strength: ");
    		SIM900_DEBUG_OUTPUT_STREAM->print(strength);
    		SIM900_DEBUG_OUTPUT_STREAM->print(" Error Rate: ");
    		SIM900_DEBUG_OUTPUT_STREAM->println(error_rate);
    	}
    }
    if(strength <= 0){
    	if(SIM900_DEBUG_OUTPUT){
    		SIM900_DEBUG_OUTPUT_STREAM->println("Waiting for modem to establish connection...");
    	}

    	delay(wait_time);
    	strength_count++;
    }
    if(strength_count > iterations)
    {
    	if(SIM900_DEBUG_OUTPUT){
    		SIM900_DEBUG_OUTPUT_STREAM->println("Could not establish connection. Not uploading data.");
    	}
    	return false;
    }
  }
  return true;
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

bool Sim900::is_valid_connection_settings(CONN settings)
{
	if(!(settings.cid >= 0 && settings.cid <= 5))
	{
		set_error_condition(SIM900_ERROR_INVALID_CID_VALUE);
		return false;
	}
	if(!is_valid_connection_type(settings.contype))
	{
		set_error_condition(SIM900_ERROR_INVALID_CONNECTION_TYPE);
		return false;
	}
	if(settings.apn != NULL && !(strlen(settings.apn) <= SIM900_MAX_CONNECTION_SETTING_CHARACTERS))
	{
		set_error_condition(SIM900_ERROR_CHARACTER_LIMIT_EXCEEDED);
		return false;
	}
	if(settings.user!= NULL && !(strlen(settings.user) <= SIM900_MAX_CONNECTION_SETTING_CHARACTERS))
	{
		set_error_condition(SIM900_ERROR_CHARACTER_LIMIT_EXCEEDED);
		return false;
	}
	if(settings.pwd != NULL && !(strlen(settings.pwd) <= SIM900_MAX_CONNECTION_SETTING_CHARACTERS))
	{
		set_error_condition(SIM900_ERROR_CHARACTER_LIMIT_EXCEEDED);
		return false;
	}
	if(settings.rate != NULL && !is_valid_connection_rate(settings.rate))
	{
		set_error_condition(SIM900_ERROR_INVALID_CONNECTION_RATE);
		return false;
	}
	return true;
}

GPRSHTTP* Sim900::createHTTPConnection(CONN settings, char URL[])
{
	set_error_condition(SIM900_ERROR_NO_ERROR);
	if(is_valid_connection_settings(settings) && lock()){
		if(settings.contype != NULL)	
		{
			_serial->write("AT+SAPBR=3,");
			_serial->print(settings.cid, DEC);
			_serial->write(",\"CONTYPE\",\"");
			_serial->write(settings.contype);
			_serial->println("\"");
			waitFor("OK", true, NULL);
		}
		if(settings.apn != NULL)	
		{
			_serial->write("AT+SAPBR=3,");
			_serial->print(settings.cid, DEC);
			_serial->write(",\"APN\",\"");
			_serial->write(settings.apn);
			_serial->println("\"");
			waitFor("OK", true, NULL);
		}
		if(settings.user != NULL)	
		{
			_serial->write("AT+SAPBR=3,");
			_serial->print(settings.cid, DEC);
			_serial->write(",\"USER\",\"");
			_serial->write(settings.user);
			_serial->println("\"");
			waitFor("OK", true, NULL);
		}
		if(settings.pwd != NULL)	
		{
			_serial->write("AT+SAPBR=3,");
			_serial->print(settings.cid, DEC);
			_serial->write(",\"PWD\",\"");
			_serial->write(settings.pwd);
			_serial->println("\"");
			waitFor("OK", true, NULL);
		}
		if(settings.phone != NULL)	
		{
			_serial->write("AT+SAPBR=3,");
			_serial->print(settings.cid, DEC);
			_serial->write(",\"PHONENUM\",\"");
			_serial->write(settings.phone);
			_serial->println("\"");
			waitFor("OK", true, NULL);
		}
		if(settings.rate != NULL)	
		{
			_serial->write("AT+SAPBR=3,");
			_serial->print(settings.cid, DEC);
			_serial->write(",\"RATE\",\"");
			_serial->write(settings.rate);
			_serial->println("\"");
			waitFor("OK", true, NULL);
		}

		return new GPRSHTTP(this, settings.cid, URL);
	}
	return NULL;
}

GPRSHTTP::GPRSHTTP(Sim900* sim, int cid, char URL[])
{
	_sim = sim;
	_cid = cid;
	url = URL;
	initialized = false;
	_data_ready = false;
	write_count = 0;
	write_limit = 0;
	read_limit = 0;
	read_count = 0;
	sequence_bit_map = 0;
}

bool GPRSHTTP::setParam(char* param, String value)
{
	if(!initialized)
	{
		Serial.println("GPRSHTTP must have been initialized before setParam can be called.");
		return false;
	}
        //Set the PARAM
	Serial.print("Setting HTTP Param ");
	Serial.print(param);
	Serial.print(" to ");
	Serial.println(value);
        _sim->_serial->write("AT+HTTPPARA=\"");
	_sim->_serial->print(param);
	_sim->_serial->print("\",\"");
        _sim->_serial->print(value);
        _sim->_serial->println("\"");
        if(!_sim->waitFor("OK", true, NULL))
        {	
                return false;
        }
	return true;
}
bool GPRSHTTP::setParam(char* param, uint32_t value)
{
	return setParam(param, String(value));
}
bool GPRSHTTP::setParam(char* param, char* value)
{
	return setParam(param, String(value));
}
int GPRSHTTP::isCGATT()
{
	_sim->_serial->println("AT+CGATT?");
	String connected;
	if(!_sim->waitFor("OK", true, &connected))
	{
		return -1;
	}
	int pos = connected.indexOf("+CGATT: ");
	pos = connected.indexOf(" ", pos);
	connected = connected.substring(pos, connected.indexOf("\r", pos));
	connected.trim();
	if(SIM900_DEBUG_OUTPUT)
	{
		SIM900_DEBUG_OUTPUT_STREAM->print("CGATT result:  ");
		SIM900_DEBUG_OUTPUT_STREAM->println(connected);
	}
	return connected.toInt();

}
bool GPRSHTTP::HTTPINIT(int retries, int _delay)
{
	//Initialize the HTTP Application context.
	for(int i = 0; i < retries; i++)
	{
		_sim->_serial->println("AT+HTTPINIT");
		if(_sim->waitFor("OK", true, NULL))
		{

			SIM900_DEBUG_OUTPUT_STREAM->println("HTTP Initialized!");
			return true;
		}
		SIM900_DEBUG_OUTPUT_STREAM->println("Failed to initialize HTTP context, waiting to retry.");
		delay(_delay);
	}
	return false;
}

bool GPRSHTTP::stopBearer(int retries, int _delay)
{
		//Shutdown the connection first.
	for(int i = 0; i < retries; i++)
	{
		_sim->_serial->write("AT+SAPBR=0,");
		_sim->_serial->println(_cid, DEC);
		if(_sim->waitFor("OK", true, NULL))
		{
			return true;
		}else
		{
			if(SIM900_DEBUG_OUTPUT)
			{
				SIM900_DEBUG_OUTPUT_STREAM->println("Not Shutdown.");
			}
		}
		delay(_delay);
	}
	return false;
}
bool GPRSHTTP::startBearer(int retries, int _delay)
{
	//Start the connection.	
	for(int i = 0; i < retries; i++)
	{
		_sim->_serial->write("AT+SAPBR=1,");
		_sim->_serial->println(_cid, DEC);
		if(_sim->waitFor("OK", true, NULL))
		{
			if(SIM900_DEBUG_OUTPUT)
			{
				SIM900_DEBUG_OUTPUT_STREAM->println("Connected!");
			}
			return true;
		}
		if(SIM900_DEBUG_OUTPUT)
		{
			SIM900_DEBUG_OUTPUT_STREAM->println("Failed to connect, waiting to retry.");
		}
		delay(_delay);
	}
	return false;
}

bool GPRSHTTP::init(int timeout)
{
	if(timeout > 1000 || timeout < 0)
	{
		_error_condition = SIM900_ERROR_INVALID_HTTP_TIMEOUT;
		if(SIM900_DEBUG_OUTPUT)
		{
			SIM900_DEBUG_OUTPUT_STREAM->println(get_error_message(SIM900_ERROR_INVALID_HTTP_TIMEOUT));
		}
		return false;
	}
	int connected = isCGATT();
	if(connected == -1)
	{
		return false;
	}
	delay(1000);

	if(connected == 1){
		stopBearer(1, 0);
	}
	
	if(!startBearer(5, 2000))
	{
		return false;
	}

	initialized = HTTPINIT(5, 2000);
	if(!initialized)
	{
		return false;
	}

	//Set the CID
	if(!setParam("CID", _cid))
	{
		return false;
	}

	//Set the URL
	if(!setParam("URL", url))
	{
		return false;
	}

	//Set the HTTP Timeout
	if(!setParam("TIMEOUT", timeout))
	{
		return false;
	}

	Serial.print("URL: "); Serial.println(url);
	return true;
}

bool GPRSHTTP::post_init(uint32_t content_length){
	set_error_condition(SIM900_ERROR_NO_ERROR);
	if(content_length > _sim->max_http_post_size)
	{
		if(SIM900_DEBUG_OUTPUT)
		{
			SIM900_DEBUG_OUTPUT_STREAM->print("Specified Content Length: ");
			SIM900_DEBUG_OUTPUT_STREAM->print(content_length);
			SIM900_DEBUG_OUTPUT_STREAM->print(" is greater than the maximum allowed post size of ");
			SIM900_DEBUG_OUTPUT_STREAM->println(_sim->max_http_post_size);
		}
		set_error_condition(SIM900_ERROR_MAX_POST_DATA_SIZE_EXCEEDED);
		return false;
	}
	_sim->_serial->write("AT+HTTPDATA=");
	_sim->_serial->print(content_length, DEC);
	_sim->_serial->write(",");
	_sim->_serial->println(SIM900_HTTP_TIMEOUT, DEC);
	if(!_sim->waitFor("DOWNLOAD", true, NULL))
	{
		return false;
	}
	write_limit = content_length;
//	return _sim->_serial;
	return true;

}


//If the response from the server does not have a Content-Length header
//then length will always be zero.
bool GPRSHTTP::post(int &cid, int &HTTP_CODE, uint32_t &length){
	unsigned long upload_time_out = SIM900_INPUT_TIMEOUT;
	if(SIM900_DEBUG_OUTPUT)
	{
		SIM900_DEBUG_OUTPUT_STREAM->print("Write Count: ");
		SIM900_DEBUG_OUTPUT_STREAM->print(write_count);
		SIM900_DEBUG_OUTPUT_STREAM->print(" Write Limit: ");
		SIM900_DEBUG_OUTPUT_STREAM->println(write_limit);
	}
	if(write_limit * 10 > upload_time_out)
	{
		upload_time_out = write_limit * 10;
	}
	if(!_sim->waitFor("OK", true, NULL))
	{
		return false;
	}
	_sim->_serial->write("AT+HTTPACTION=");
	_sim->_serial->println(POST, DEC);
	if(!_sim->waitFor("+HTTPACTION:", true, NULL, upload_time_out))
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
	read_limit = length;
	delete tmp;
	return true;

}

int GPRSHTTP::init_retrieve()
{
	Serial.println("Getting Data.");
	_sim->_serial->print("AT+HTTPREAD=0,");
	_sim->_serial->println(read_limit, DEC);
	if(!_sim->waitFor("+HTTPREAD:", true, NULL))
	{
		return false;
	}
	_sim->waitFor("\n", true, NULL);
	_data_ready = true;
	return true;
}

bool GPRSHTTP::terminate()
{
	_sim->_serial->println("AT+HTTPTERM");
	_sim->waitFor("OK", true, NULL);
	stopBearer(5,1000);
	_sim->unlock();
}

size_t GPRSHTTP::write(uint8_t byte)
{
	if(write_count++ < write_limit)
	{
		
		size_t toRet = _sim->_serial->write(byte);
		if(SIM900_DEBUG_OUTPUT && _sim->_serial->available())
		{
			SIM900_DEBUG_OUTPUT_STREAM->println();
			while(_sim->_serial->available())
			{
				SIM900_DEBUG_OUTPUT_STREAM->print((char)_sim->_serial->read());
			}
			SIM900_DEBUG_OUTPUT_STREAM->println();
		}
		return toRet;
	}
	return -1;
}


size_t GPRSHTTP::read(char* buf, int length)
{
	return read((byte*)buf, length);
}

size_t GPRSHTTP::read(byte* buf, int length)
{
	int tmp = -1;
	for(int i = 0; i < length;)	
	{
		tmp = read();
		if(tmp < 0)
		{
			return i;
		}
		buf[i] = tmp;
		i++;
	}
}

int GPRSHTTP::read()
{
	set_error_condition(SIM900_ERROR_NO_ERROR);
	unsigned long time = millis();
	int avail = 0;
	if(!_data_ready)
	{
		set_error_condition(SIM900_ERROR_DATA_NOT_READY);
		return SIM900_ERROR_DATA_NOT_READY;
	}
	if(read_count++ <= read_limit)
	{
		while((avail = available()) == false)
		{
			if((millis() - time) > SIM900_INPUT_TIMEOUT)
			{
				if(SIM900_DEBUG_OUTPUT)
				{
					SIM900_DEBUG_OUTPUT_STREAM->println("The timeout was reached whilst trying to read the HTTP response.");
				}
				return SIM900_ERROR_TIMEOUT;
			}
		}
		if(avail > 0)
		{
			return _sim->_serial->read();
		}else
		{
			set_error_condition(avail);
			return avail;
		}
	}
	else
	{
		if(SIM900_DEBUG_OUTPUT)
		{
			set_error_condition(SIM900_ERROR_READ_LIMIT_EXCEEDED);
			SIM900_DEBUG_OUTPUT_STREAM->print("Read limit: ");
			SIM900_DEBUG_OUTPUT_STREAM->print(read_limit);
			SIM900_DEBUG_OUTPUT_STREAM->print(" has been exceed by read count: ");
			SIM900_DEBUG_OUTPUT_STREAM->println(read_count);
		}
	}
	return -1;
}

int GPRSHTTP::available()
{
	if(read_count <= read_limit)
	{
		return _sim->_serial->available();
	}
	else
	{
		if(SIM900_DEBUG_OUTPUT)
		{
			SIM900_DEBUG_OUTPUT_STREAM->print("Read limit: ");
			SIM900_DEBUG_OUTPUT_STREAM->print(read_limit);
			SIM900_DEBUG_OUTPUT_STREAM->print(" has been exceed by read count: ");
			SIM900_DEBUG_OUTPUT_STREAM->println(read_count);
		}
		return SIM900_ERROR_READ_LIMIT_EXCEEDED;
	}
	return false;
}

void GPRSHTTP::flush()
{
	_sim->_serial->flush();
}

int GPRSHTTP::peek()
{
	set_error_condition(SIM900_ERROR_NO_ERROR);
	if(read_count <= read_limit)
	{
		return _sim->_serial->peek();
	}
	else
	{
		if(SIM900_DEBUG_OUTPUT)
		{
			set_error_condition(SIM900_ERROR_READ_LIMIT_EXCEEDED);
			SIM900_DEBUG_OUTPUT_STREAM->print("Read limit: ");
			SIM900_DEBUG_OUTPUT_STREAM->print(read_limit);
			SIM900_DEBUG_OUTPUT_STREAM->print(" has been exceed by read count: ");
			SIM900_DEBUG_OUTPUT_STREAM->print(read_count);
		}
	}
	return -1;
	
}

void GPRSHTTP::set_error_condition(int error_value)
{
	_error_condition = error_value;
}

int GPRSHTTP::get_error_condition()
{
	return _error_condition;
}

