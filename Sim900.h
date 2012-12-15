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

#define SIM900_ERROR_NO_ERROR 0
#define SIM900_ERROR_MODEM_ERROR -1
#define SIM900_ERROR_TIMEOUT -2
#define SIM900_ERROR_DATA_NOT_READY -3
#define SIM900_ERROR_READ_LIMIT_EXCEDED -4

#define SIM900_MAX_POST_DATA 318976
#define SIM900_CONNECTION_INIT = 2;

#include <Stream.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <SoftwareSerial.h>


static bool   SIM900_DEBUG_OUTPUT = false;
static Stream* SIM900_DEBUG_OUTPUT_STREAM = &Serial;
//static uint32_t SIM900_DEBUG_OUTPUT_TIMEOUT = 60000;
static unsigned long SIM900_INPUT_TIMEOUT  = 60000l;

struct CONN
{
	int  cid;
	char* contype;
	char* apn;
	char* user;
	char* pwd;
	char* phone;
	char* rate;

};


void set_sim900_debug_mode(bool mode);
void set_sim900_debug_stream(Stream* stream);
//void set_sim900_outout_timeout(uint32_t timeout);
void set_sim900_input_timeout(unsigned long timeout);

class GPRSHTTP;

class Sim900
{
	private:
		Stream* _serial;
		int _error_condition;
		int _powerPin;
		int _statusPin;
		int _lock;
		int lock();
		int unlock();
		bool dropEOL();
		int waitFor(char target[], bool dropLastEOL, String* data);
		int waitFor(char target[], bool dropLastEOL, String* data, unsigned long timeout);
		bool compare(char to_test[], char target[], int pos, int length, int test_len);
		void powerToggle();
		bool issueCommand(char command[], char ok[], bool dropLastEOL);
		void dumpStream();
		void set_error_condition(int error_value);
	public:
		Sim900(SoftwareSerial* serial, int baud_rate, int powerPin, int statusPin);
		Sim900(HardwareSerial* serial, int baud_rate, int powerPin, int statusPin);
		bool getSignalQuality(int &strength, int &error_rate);
		bool isPoweredUp();
		bool powerUp();
		bool powerDown();
		GPRSHTTP* createHTTPConnection(CONN settings, char URL[]);
		bool startGPRS();
		bool stopGPRS();
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
		int write_limit, write_count;
		int read_limit,  read_count;
		bool initialized, _data_ready;
		int isCGATT();
		bool HTTPINIT(int retries, int _delay);
		bool stopBearer();//int retries, int _delay);
		bool startBearer(int retries, int _delay);
		void set_error_condition(int error_value);
	public:
		GPRSHTTP(Sim900* sim, int cid, char URL[]);
		bool init();
		bool setParam(char* param, String value);
		bool setParam(char* param, char* value);
		bool setParam(char* param, int value);


		//Stream* post_init(int content_length);
		bool post_init(int content_length);
		bool post(int &cid, int &HTTP_CODE, int &length);
		//int _read(char* data, int length);
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
