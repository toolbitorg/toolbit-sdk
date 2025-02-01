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

Choppy::Choppy() :
	i2chw(mTbiService, ATT_IC20_BASE)
{
	mAttReset = new Attribute(ATT_RESET, 0x00, 0x00);
	mAttColor = new Attribute(ATT_COLOR, 0x00, 0x00);
	mAttVoltage = new Attribute(ATT_VOLTAGE, 0x00, 0x00);
	mAttCurrent = new Attribute(ATT_CURRENT, 0x00, 0x00);
}

Choppy::~Choppy()
{
	close();

	delete mAttReset;
	delete mAttColor;
	delete mAttVoltage;
	delete mAttCurrent;
}

bool Choppy::open()
{
	TbiDeviceManager devm;
	return openPath(devm.getPathByName("Choppy"));
}

bool Choppy::open(string serial)
{
	TbiDeviceManager devm;
	return openPath(devm.getPathByNameAndSerial("Choppy", serial));
}

bool Choppy::enableDfu()
{
	return mTbiService->writeAttribute(*mAttReset);
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
	if (mTbiService->readAttribute(mAttVoltage)) {
		// error
		return -999.9;
	}
	return mAttVoltage->getValueFloat();
}

float Choppy::getCurrent()
{
	if (mTbiService->readAttribute(mAttCurrent)) {
		// error
		return -999.9;
	}
	return mAttCurrent->getValueFloat();
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
