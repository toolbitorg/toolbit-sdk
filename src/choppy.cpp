/*  Toolbit SDK
 *  Copyright (C) 2020 ohamax <toolbitorg@gmail.com>
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 */
#include <stdio.h>
#include <wchar.h>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <math.h>
#include "tbi_device_manager.h"
#include "choppy.h"

#define INA228_CONFG        0x00
#define INA228_ADC_CONFG    0x01
#define INA228_SHUNT_CAL    0x02
#define INA228_SHUNT_TEMPCO 0x03
#define INA228_VSHUNT       0x04
#define INA228_VBUS         0x05
#define INA228_DIETEMP      0x06
#define INA228_CURRENT      0x07
#define INA228_POWER        0x08
#define INA228_ENERGY       0x09
#define INA228_CHARGE       0x0A
#define INA228_DIAG_ALRT    0x0B
#define INA228_SOVL         0x0C
#define INA228_SUVL         0x0D
#define INA228_BOVL         0x0E
#define INA228_BUVL         0x0F
#define INA228_TEMP_LIMIT   0x10
#define INA228_PWR_LIMIT    0x11
#define INA228_MANUFACTURER_ID 0x3E
#define INA228_DEVICE_ID       0x3F


// Logging configuration
#define LOG_LEVEL_WARN 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_NONE 0

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
	#define WARN(...) \
		do { fprintf(stderr, "[WARN] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#else
	#define WARN(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
	#define INFO(...) \
		do { fprintf(stderr, "[INFO] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#else
	#define INFO(...) ((void)0)
#endif


Choppy::Choppy() :
	i2chw(mTbiService, ATT_IC20_BASE),
	volt_updated(false), curr_updated(false)
{
	mAttReset = new Attribute(ATT_RESET, 0x00, 0x00);
	mAttTriggerMode = new Attribute(ATT_TRIGGER_MODE, 0x00, 0x00);
	mAttColor = new Attribute(ATT_COLOR, 0x00, 0x00);
	mAttVoltage = new Attribute(ATT_VOLTAGE, 0x00, 0x00);
	mAttCurrent = new Attribute(ATT_CURRENT, 0x00, 0x00);

	integratingTime = CHOPPY_DEFAULT_INTEGRATING_TIME_MS;
	data_cnt = 0;
	volt_avg = 0.0;
	curr_avg = 0.0;

	volt_latest = NAN;
	curr_latest = NAN;

	std::function<void(tbiPacket)> func = [this](tbiPacket pckt) { interruptHandler(pckt); };
	setInterruptCallback(func);
}

Choppy::~Choppy()
{
	close();

	delete mAttTriggerMode;
	delete mAttReset;
	delete mAttColor;
	delete mAttVoltage;
	delete mAttCurrent;
}

bool Choppy::open()
{
	TbiDeviceManager devm;
	if (openPath(devm.getPathByName("Choppy"))) {
		return true;
	}
	INFO("Set TRIGGER_MODE_CONTINUOUS");
	setTriggerMode(TRIGGER_MODE_CONTINUOUS);
	return false;
}

bool Choppy::open(string serial)
{
	TbiDeviceManager devm;
	if (openPath(devm.getPathByNameAndSerial("Choppy", serial))) {
		return true;
	}
	INFO("Set TRIGGER_MODE_CONTINUOUS");
	setTriggerMode(TRIGGER_MODE_CONTINUOUS);
	return false;
}

bool Choppy::close()
{
	if (mAttTriggerMode->getValueUint8() == TRIGGER_MODE_CONTINUOUS) {
		INFO("Set TRIGGER_MODE_NONE");
		setTriggerMode(TRIGGER_MODE_NONE);
	}

	if (!mTbiDevice->isOpen())
		return true;

	mTbiService->stop();
	mTbiDevice->close();
	return false;
}

bool Choppy::enableDfu()
{
	return mTbiService->writeAttribute(*mAttReset);
}

bool Choppy::setTriggerMode(uint8_t val)
{
	mAttTriggerMode->setValue(val);
	if (mTbiService->writeAttribute(*mAttTriggerMode)) {
		mAttTriggerMode->setValue(TRIGGER_MODE_NONE);
		INFO("Not support trigger mode");
		return true;
	}
	return false;
}

bool Choppy::setIntegratingTime(uint16_t ms)
{
	if (ms < CHOPPY_MEASUREMENT_INTERVAL_MS || ms > DATA_BUF_SIZE_MAX) {
		return true;
	}
	integratingTime = ms;
	return false;
}

uint8_t Choppy::getColor()
{
	if (mTbiService->readAttribute(mAttColor)) {
		// error
		return 0xFF;
	}
	return mAttColor->getValueUint8();
}

bool Choppy::setColor(uint8_t val)
{
	mAttColor->setValue(val);
	return mTbiService->writeAttribute(*mAttColor);
}

float Choppy::getVoltage()
{
	if (mAttTriggerMode->getValueUint8() == TRIGGER_MODE_CONTINUOUS) {
		int cnt = 10;
		while (!volt_updated.load() && cnt != 0) {
			cnt--;
			Sleep(5);
		}
		if (cnt == 0) {
			// error
			return NAN;
		}
		lock_guard<mutex> lock(mtx);
		volt_updated.store(false);
		return volt_latest;
	} else {
		// TRIGGER_MODE_NORMAL or TRIGGER_MODE_NONE
		if (mTbiService->readAttribute(mAttVoltage)) {
			// error
			return NAN;
		}
		return mAttVoltage->getValueFloat();
	}
}

float Choppy::getCurrent()
{
	if (mAttTriggerMode->getValueUint8() == TRIGGER_MODE_CONTINUOUS) {		
		int cnt = 10;
		while (!curr_updated.load() && cnt != 0) {
			cnt--;
			Sleep(5);
		}
		if (cnt == 0) {
			// error
			return NAN;
		}
		lock_guard<mutex> lock(mtx);
		curr_updated.store(false);
		return curr_latest;
	} else {
		// TRIGGER_MODE_NORMAL or TRIGGER_MODE_NONE
		if (mTbiService->readAttribute(mAttCurrent)) {
			// error
			return NAN;
		}
		return mAttCurrent->getValueFloat();
	}
}

string Choppy::showReg()
{
	stringstream ss;
	for (int addr = 0; addr <= INA228_PWR_LIMIT; addr++) {

		if(addr==4 || addr==5) {
			ss << hex << addr << ": 0x" << i2chw.read3byte(addr) << endl;
		} else {
			ss << hex << addr << ": 0x" << i2chw.read2byte(addr) << endl;
		}
	}
	return ss.str();
}

uint16_t Choppy::getDeviceID()
{
	return i2chw.read2byte(INA228_DEVICE_ID);
}

void Choppy::interruptHandler(tbiPacket pckt)
{
	Attribute volt(ATT_VOLTAGE, 0x00, 0x00);
	Attribute curr(ATT_CURRENT, 0x00, 0x00);
	float vol;
	float cur;

	INFO("interruptHandler is called");
	int len = (pckt.dat[0] & 0x3F) - 2;

	if (len == 32) {

		for (uint8_t i = 0; i < 4; i++) {

			volt.setValue(&pckt.dat[8 * i + 2], 4);
			curr.setValue(&pckt.dat[8 * i + 6], 4);
			vol = volt.getValueFloat();
			cur = curr.getValueFloat();

			volt_buf[data_cnt] = vol;
			curr_buf[data_cnt] = cur;
			data_cnt++;
			volt_avg += vol;
			curr_avg += cur;

			if (CHOPPY_MEASUREMENT_INTERVAL_MS * data_cnt >= integratingTime) {

				lock_guard<mutex> lock(mtx);
				volt_latest = volt_avg / data_cnt;
				curr_latest = curr_avg / data_cnt;
				volt_updated.store(true);
				curr_updated.store(true);

				volt_avg = 0.0;
				curr_avg = 0.0;
				data_cnt = 0;
			}

		}

	}
	else {
		volt_latest = NAN;
		curr_latest = NAN;
	}
}
