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

#ifndef __SIM_900_H__
#define __SIM_900_H__

#define NULL 0
#define GET  0
#define POST 1
#define HEAD 2

#ifndef SIM900_POWERUP_THRESHOLD
#define SIM900_POWERUP_THRESHOLD 100
#endif

#ifndef SIM900_HTTP_TIMEOUT
#define SIM900_HTTP_TIMEOUT 100000
#endif


#define SIM900_ERROR_LIST_TERMINATOR 1
#define SIM900_ERROR_NO_ERROR 0
#define SIM900_ERROR_COULD_NOT_AQUIRE_LOCK -1
#define SIM900_ERROR_MODEM_ERROR -10
#define SIM900_ERROR_TIMEOUT -20
#define SIM900_ERROR_DATA_NOT_READY -30
#define SIM900_ERROR_MAX_POST_DATA_SIZE_EXCEEDED -40
#define SIM900_ERROR_READ_LIMIT_EXCEEDED -41
#define SIM900_ERROR_INVALID_CID_VALUE -50
#define SIM900_ERROR_CHARACTER_LIMIT_EXCEEDED -51
#define SIM900_ERROR_INVALID_CONNECTION_TYPE -52
#define SIM900_ERROR_INVALID_CONNECTION_RATE -53
#define SIM900_ERROR_INVALID_HTTP_TIMEOUT -54

#define SIM900_MAX_POST_DATA 318976
#define SIM900_MAX_HTTP_TIMEOUT 120000
#define SIM900_CONNECTION_INIT = 2;
#define SIM900_MAX_CONNECTION_SETTING_CHARACTERS 50


#include <Stream.h>

//#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
//#else
//#include "WProgram.h"
//#endif

#include <SoftwareSerial.h>


static bool   SIM900_DEBUG_OUTPUT = true;
static Stream* SIM900_DEBUG_OUTPUT_STREAM = &Serial;
static unsigned long SIM900_INPUT_TIMEOUT  = 60000l;

struct CONN
{
	int  cid;      // Bearer profile identifier.
	char* contype; // Valid Options: GPRS, CSD.
	char* apn;     // Provided by telco. Sometimes preprogrammed into the SIM card.
	char* user;    // Username, MAX 50 Characters.
	char* pwd;     // Password, MAX 50 Charcters.
	char* phone;   // Phone number for CSD call.
	char* rate;    // CSD connection rate.

	CONN() : cid(-1), contype(NULL), apn(NULL), user(NULL), pwd(NULL), phone(NULL), rate(NULL) {}
};

struct error_message
{
	int code;
	char* message;
};

static error_message messages[] =
{
	{SIM900_ERROR_NO_ERROR, "No Error"},
	{SIM900_ERROR_COULD_NOT_AQUIRE_LOCK, "Only one connection at a time can use the modem."},
	{SIM900_ERROR_MODEM_ERROR, "Modem Error"},
	{SIM900_ERROR_TIMEOUT, "Timed out waiting for modem response."},
	{SIM900_ERROR_DATA_NOT_READY, "init_retrieve needs to be called before data can be read."},
	{SIM900_ERROR_MAX_POST_DATA_SIZE_EXCEEDED, "The maximum post data size was exceeded."},
	{SIM900_ERROR_READ_LIMIT_EXCEEDED, "The read limit was exceeded."},
	{SIM900_ERROR_INVALID_CID_VALUE, "Invalid Bearer profile Identifier"},
	{SIM900_ERROR_CHARACTER_LIMIT_EXCEEDED, "The Maximum character limit was exceeded"},
	{SIM900_ERROR_INVALID_CONNECTION_TYPE, "The specified connection type is not valid."},
	{SIM900_ERROR_INVALID_CONNECTION_RATE, "The specified connection rate is not valid."},
	{SIM900_ERROR_INVALID_HTTP_TIMEOUT, "The HTTP Timeout value must be between 0 and 1000."},


	//This needs to be the last element or things will go badly wrong.
	{SIM900_ERROR_LIST_TERMINATOR, NULL} 
};

static const char *VALID_CONNECTION_TYPES[] = {
	"GPRS",
	"CSD",
	NULL
};

static const int VALID_CONNECTION_SPEEDS[] =
{
	2400,
	4800,
	9600,
	14400,
	NULL
};

bool is_valid_connection_type(char* to_check);
bool is_valid_connection_rate(char* to_check);
void set_sim900_debug_mode(bool mode);
void set_sim900_debug_stream(Stream* stream);
void set_sim900_input_timeout(unsigned long timeout);
char* get_error_message(int error_code);

class GPRSHTTP;

class Sim900
{
	private:
		Stream* _serial;
		SoftwareSerial* _ser;
		int _error_condition;
		int _powerPin;
		int _statusPin;
		int _lock;
		bool lock();
		bool unlock();
		bool dropEOL();
		int waitFor(char target[], bool dropLastEOL, String* data);
		int waitFor(char target[], bool dropLastEOL, String* data, unsigned long timeout);
		bool compare(char to_test[], char target[], int pos, int length, int test_len);
		void powerToggle();
		bool issueCommand(char command[], char ok[], bool dropLastEOL);
		void dumpStream();
		void set_error_condition(int error_value);
		bool is_valid_connection_settings(CONN settings);
	public:
		Sim900(SoftwareSerial* serial, int baud_rate, int powerPin, int statusPin);
		Sim900(HardwareSerial* serial, int baud_rate, int powerPin, int statusPin);

		bool getSignalQuality(int &strength, int &error_rate);
		bool waitForSignal(int iterations, int wait_time);
		bool isPoweredUp();
		bool powerUp();
		bool powerDown();
		GPRSHTTP* createHTTPConnection(CONN settings, char URL[]);
		/*bool startGPRS();
		bool stopGPRS();*/
		int get_error_condition();


	friend class GPRSHTTP; 
};

class GPRSHTTP : public Stream
{
	private:

		uint32_t sequence_bit_map;
		Sim900* _sim;
		int _error_condition;
		int _cid;
		char* url;
		String _data;
		uint32_t write_limit, write_count;
		uint32_t read_limit,  read_count;
		bool initialized, _data_ready;
		int isCGATT();
		bool HTTPINIT(int retries, int _delay);
		bool stopBearer(int retries, int _delay);
		bool startBearer(int retries, int _delay);
		void set_error_condition(int error_value);
	public:
		GPRSHTTP(Sim900* sim, int cid, char URL[]);
		bool init(int timeout = 120);
		bool setParam(char* param, String value);
		bool setParam(char* param, char* value);
		bool setParam(char* param, uint32_t value);


		bool post_init(uint32_t content_length);

		//If the response from the server does not have a Content-Length header
		//then length will always be zero.
		bool post(int &cid, int &HTTP_CODE, uint32_t &length);
		int init_retrieve();
		int get_error_condition();
		bool terminate();

		size_t read(char* buf, int length);
		size_t read(byte* buf, int length);
		virtual size_t write(uint8_t byte);
		virtual int read();
		virtual int available();
		virtual void flush();
		virtual int peek();
		using Print::write;

};

#endif
