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
#include <atomic>
#include <mutex>
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

// Make it thread-safe for interruptHanlder
inline mutex mtx;
inline atomic<bool> volt_updated{ false };
inline atomic<bool> curr_updated{ false };


Dmm::Dmm() :
	i2chw(mTbiService, ATT_IC20_BASE)
{
	mAttReset = new Attribute(ATT_RESET, 0x00, 0x00);
	mAttTriggerMode = new Attribute(ATT_TRIGGER_MODE, 0x00, 0x00);
	mAttCalibration = new Attribute(ATT_CALIBRATION, 0x00, 0x00);
	mAttVoltage = new Attribute(ATT_VOLTAGE, 0x00, 0x00);
	mAttCurrent = new Attribute(ATT_CURRENT, 0x00, 0x00);
	
	integratingTime = DMM_INTEGRATING_TIME;
	data_cnt = 0;
	volt_avg = 0.0;
	curr_avg = 0.0;

	volt_updated.store(false);
	curr_updated.store(false);
	volt_latest = -999.9;
	curr_latest = -999.9;

	std::function<void(tbiPacket)> func = [this](tbiPacket pckt) { interruptHandler(pckt); };
	setInterruptCallback(func);
}

Dmm::~Dmm()
{
	setTriggerMode(TRIGGER_MODE_NORMAL);
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
	setTriggerMode(TRIGGER_MODE_CONTINUOUS);
	return false;
}

bool Dmm::open(string serial)
{
	TbiDeviceManager devm;
		if (openPath(devm.getPathByNameAndSerial("DMM1", serial))) {
		return true;
	}
	setTriggerMode(TRIGGER_MODE_CONTINUOUS);
	return false;
}

bool Dmm::enableDfu()
{
	return mTbiService->writeAttribute(*mAttReset);
}

bool Dmm::setTriggerMode(uint8_t val)
{
	uint8_t previous_val = mAttTriggerMode->getValueUint8();

	mAttTriggerMode->setValue(val);
	if (mTbiService->writeAttribute(*mAttTriggerMode)) {
		mAttTriggerMode->setValue(previous_val);
		return true;
	}
	return false;
}

bool Dmm::setIntegratingTime(uint16_t ms)
{
	if (ms < 1 || ms > DATA_BUF_SIZE_MAX) {
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
	if (mTbiService->readAttribute(mAttVoltage)) {
		// error
		return -999.9;
	}
	return mAttVoltage->getValueFloat();
}

float Dmm::getCurrent()
{
		if (mAttTriggerMode->getValueUint8() == TRIGGER_MODE_CONTINUOUS) {
		while (!curr_updated.load()) {
			Sleep(0);
		}
		lock_guard<mutex> lock(mtx);
		curr_updated.store(false);
		return curr_latest;
	}
	if (mAttTriggerMode->getValueUint8() == TRIGGER_MODE_NORMAL) {
		if (mTbiService->readAttribute(mAttCurrent)) {
			// error
			return -999.9;
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

			if (data_cnt * SAMPLING_TIME == integratingTime) {

				lock_guard<mutex> lock(mtx);

				volt_latest = volt_avg / integratingTime;
				curr_latest = curr_avg / integratingTime;
				volt_updated.store(true);
				curr_updated.store(true);

				volt_avg = 0.0;
				curr_avg = 0.0;
				data_cnt = 0;
			}

		}

	}
	else {
		volt_latest = -999.9;
		curr_latest = -999.9;
	}
}
