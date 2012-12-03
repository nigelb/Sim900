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


#include <Stream.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

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


class GPRSHTTP;

class Sim900
{
	private:
		Stream* _serial;
		int _powerPin;
		int _statusPin;
		int _lock;
		int lock();
		int unlock();
		bool dropEOL();
		int waitFor(char target[], bool dropLastEOL, String* data);
		bool compare(char to_test[], char target[], int pos, int length, int test_len);
		void powerToggle();
		bool issueCommand(char command[], char ok[], bool dropLastEOL);
		void dumpStream();
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

	friend class GPRSHTTP; 
};

class GPRSHTTP
{
	private:
		Sim900* _sim;
		int _cid;
		char* url;
		String _data;
	public:
		GPRSHTTP(Sim900* sim, int cid, char URL[]);
		bool init();
		bool setParam();
		
		int write(char* data);
		int write(int data);
		int write(String data);

		int print(char* data);
		int print(int data);
		int print(String data);

		int println();
		int println(char* data);
		int println(int data);
		int println(String data);
		int post(int &cid, int &HTTP_CODE, int& length);	
		int _read(char* data, int length);
		bool terminate();

};

#endif
