/*  Toolbit SDK
 *  Copyright (C) 2020 ohamax <toolbitorg@gmail.com>
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 */
#ifndef TOOLBITSDK_CHOPPY_H_
#define TOOLBITSDK_CHOPPY_H_

#include <stdint.h>
#include "i2c_hw.h"
#include "attribute.h"

// Platform commom attribute ID
#define ATT_RESET      0x1100
#define ATT_GPIO0_BASE 0x1200
#define ATT_ADC_BASE   0x1300
#define ATT_IC20_BASE  0x1400
// Product specific attribute ID
#define ATT_COLOR      0x8100
#define ATT_VOLTAGE    0x8101
#define ATT_CURRENT    0x8102

// Color value
#define COLOR_BLACK    0x00
#define COLOR_BROWN    0x01
#define COLOR_RED      0x02
#define COLOR_ORANGE   0x03
#define COLOR_YELLOW   0x04
#define COLOR_GREEN    0x05
#define COLOR_BLUE     0x06
#define COLOR_VIOLET   0x07
#define COLOR_GRAY     0x08
#define COLOR_WHITE    0x09


class Choppy : public TbiCore
{
public:
	Choppy();
	~Choppy();

	// Hardware module
	I2cHw  i2chw;

	bool open();
	bool open(string serial);
	bool enableDfu();
	uint8_t getColor();
	bool setColor(uint8_t val);
	float getVoltage();
	float getCurrent();

	string showReg();
	uint16_t getDeviceID();

protected:

private:
	// Platform common attribute ID
	Attribute* mAttReset;

	// Product specific attribute ID
	Attribute* mAttColor;
	Attribute* mAttVoltage;
	Attribute* mAttCurrent;
};

#endif /* TOOLBITSDK_DMM_H_ */
