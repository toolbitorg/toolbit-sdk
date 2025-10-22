/*  Toolbit SDK
 *  Copyright (C) 2020 ohamax <toolbitorg@gmail.com>
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 */
#ifndef TOOLBITSDK_DMM_H_
#define TOOLBITSDK_DMM_H_

#include <stdint.h>
#include "i2c_hw.h"
#include "attribute.h"

// Platform commom attribute ID
#define ATT_RESET      0x1100
#define ATT_GPIO0_BASE 0x1200
#define ATT_ADC_BASE   0x1300
#define ATT_IC20_BASE  0x1400
// Product specific attribute ID
#define ATT_TRIGGER_MODE 0x8000
#define ATT_CALIBRATION  0x8100
#define ATT_VOLTAGE      0x8101
#define ATT_CURRENT      0x8102

// Trigger Mode value
#define TRIGGER_MODE_NORMAL     0x00
#define TRIGGER_MODE_CONTINUOUS 0x01

#define DATA_BUF_SIZE_MAX 1000
#define SAMPLING_TIME     10.0
#define DMM_INTEGRATING_TIME  20



class Dmm : public TbiCore
{
public:
	Dmm();
	~Dmm();

	// Hardware module
	I2cHw  i2chw;

	bool open();
	bool open(string serial);
	bool enableDfu();
	bool setTriggerMode(uint8_t val);
	bool setIntegratingTime(uint16_t ms);	  
	bool calibration();
	string getCalibrationData();
	float getVoltage();
	float getCurrent();

	string showReg();
	uint16_t getDieID();

	void interruptHandler(tbiPacket pckt);

protected:

private:
	// Platform common attribute ID
	Attribute* mAttReset;

	// Product specific attribute ID
	Attribute* mAttTriggerMode;
	Attribute* mAttCalibration;
	Attribute* mAttVoltage;
	Attribute* mAttCurrent;
	
	uint16_t integratingTime;  // unit: msec

	// For recieved data handing
	float volt_buf[DATA_BUF_SIZE_MAX];
	float curr_buf[DATA_BUF_SIZE_MAX];
	uint16_t data_cnt;
	float volt_avg;
	float curr_avg;
	float volt_latest;
	float curr_latest;

};

#endif /* TOOLBITSDK_DMM_H_ */
