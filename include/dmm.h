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
#include <atomic>
#include <mutex>
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
#define TRIGGER_MODE_NONE       0x00
#define TRIGGER_MODE_NORMAL     0x01
#define TRIGGER_MODE_CONTINUOUS 0x02

#define DATA_BUF_SIZE_MAX 1000
#define DMM_MEASUREMENT_INTERVAL_MS 5  // 5ms interval
#define DMM_DEFAULT_INTEGRATING_TIME_MS 20

class Dmm : public TbiCore
{
public:
	Dmm();
	~Dmm();

	// Hardware module
	I2cHw  i2chw;

	bool open();
	bool open(string serial);
	bool close();
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

	// Make it thread-safe for interruptHanlder
	mutex mtx;
	atomic<bool> volt_updated;
	atomic<bool> curr_updated;
};

#endif /* TOOLBITSDK_DMM_H_ */
