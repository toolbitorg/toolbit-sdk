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
#include "dmm.h"

#define REG_CONFIG 0x00
#define REG_SHUNTV_1 0x01
#define REG_BUSV_1   0x02
#define REG_SHUNTV_2 0x03
#define REG_BUSV_2   0x04
#define REG_SHUNTV_3 0x05
#define REG_BUSV_3   0x06
#define REG_CRITICAL_LIMIT_1 0x07
#define REG_WARNING_LIMIT_1  0x08
#define REG_CRITICAL_LIMIT_2 0x09
#define REG_WARNING_LIMIT_2  0x0A
#define REG_CRITICAL_LIMIT_3 0x0B
#define REG_WARNING_LIMIT_3  0x0C
#define REG_SHUNTV_SUM       0x0D
#define REG_SHUNTV_SUM_LIMIT 0x0E
#define REG_MASK_ENABLE      0x0F
#define REG_POWER_VALID_UPPER_LIMIT 0x10
#define REG_POWER_VALID_LOWER_LIMIT 0x11
#define REG_MANUFACTURER_ID 0xFE
#define REG_DIE_ID 0xFF


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


Dmm::Dmm() :
	i2chw(mTbiService, ATT_IC20_BASE),
	volt_updated(false), curr_updated(false)
{
	mAttReset = new Attribute(ATT_RESET, 0x00, 0x00);
	mAttTriggerMode = new Attribute(ATT_TRIGGER_MODE, 0x00, 0x00);
	mAttCalibration = new Attribute(ATT_CALIBRATION, 0x00, 0x00);
	mAttVoltage = new Attribute(ATT_VOLTAGE, 0x00, 0x00);
	mAttCurrent = new Attribute(ATT_CURRENT, 0x00, 0x00);
	
	integratingTime = DMM_DEFAULT_INTEGRATING_TIME_MS;
	data_cnt = 0;
	volt_avg = 0.0;
	curr_avg = 0.0;

	volt_latest = NAN;
	curr_latest = NAN;

	std::function<void(tbiPacket)> func = [this](tbiPacket pckt) { interruptHandler(pckt); };
	setInterruptCallback(func);
}

Dmm::~Dmm()
{
	close();

	delete mAttTriggerMode;
	delete mAttReset;
	delete mAttCalibration;
	delete mAttVoltage;
	delete mAttCurrent;
}

bool Dmm::open()
{
	TbiDeviceManager devm;
	if (openPath(devm.getPathByName("DMM1"))) {
		return true;
	}
	INFO("Set TRIGGER_MODE_CONTINUOUS");
	setTriggerMode(TRIGGER_MODE_CONTINUOUS);
	return false;
}

bool Dmm::open(string serial)
{
	TbiDeviceManager devm;
		if (openPath(devm.getPathByNameAndSerial("DMM1", serial))) {
		return true;
	}
	INFO("Set TRIGGER_MODE_CONTINUOUS");
	setTriggerMode(TRIGGER_MODE_CONTINUOUS);
	return false;
}

bool Dmm::close()
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

bool Dmm::enableDfu()
{
	return mTbiService->writeAttribute(*mAttReset);
}

bool Dmm::setTriggerMode(uint8_t val)
{
	mAttTriggerMode->setValue(val);
	if (mTbiService->writeAttribute(*mAttTriggerMode)) {
		mAttTriggerMode->setValue(TRIGGER_MODE_NONE);
		INFO("Not support trigger mode");
		return true;
	}
	return false;
}

bool Dmm::setIntegratingTime(uint16_t ms)
{
	if (ms < DMM_MEASUREMENT_INTERVAL_MS || ms > DATA_BUF_SIZE_MAX) {
		return true;
	}
	integratingTime = ms;
	return false;
}

bool Dmm::calibration()
{
	// Triger calibration by writting any value on mAttCalibration attribute
	mAttCalibration->setValue(0x00);
	return mTbiService->writeAttribute(*mAttCalibration);
}

string Dmm::getCalibrationData()
{
	stringstream ss;
	if (mTbiService->readAttribute(mAttCalibration)) {
		// error
	}
	else {
		uint32_t dat = mAttCalibration->getValueUint32();
		uint8_t u8;
		int8_t  i8;

		u8 = dat & 0xFF;
		ss << "LOW_CURRENT_OFFSET: " << int(*(int8_t*)& u8)  << endl;
		dat = dat >> 8;
		u8 = dat & 0xFF;
		ss << "HIGH_CURRENT_OFFSET: " << int(*(int8_t*)& u8) << endl;
		dat = dat >> 8;
		u8 = dat & 0xFF;
		ss << "LOW_VOLTAGE_OFFSET: " << int(*(int8_t*)& u8) << endl;
		dat = dat >> 8;
		u8 = dat & 0xFF;
		ss << "HIGH_VOLTAGE_OFFSET: " << int(*(int8_t*)& u8) << endl;
	}
	return ss.str();
}

float Dmm::getVoltage()
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

float Dmm::getCurrent()
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

string Dmm::showReg()
{
	stringstream ss;
	for (int addr = 0; addr <= REG_POWER_VALID_LOWER_LIMIT; addr++) {
		ss << hex << addr << ": 0x" << i2chw.read2byte(addr) << endl;
	}
	return ss.str();
}

uint16_t Dmm::getDieID()
{
	return i2chw.read2byte(REG_DIE_ID);
}

void Dmm::interruptHandler(tbiPacket pckt)
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

			if (DMM_MEASUREMENT_INTERVAL_MS * data_cnt >= integratingTime) {

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
