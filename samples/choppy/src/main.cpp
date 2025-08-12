#include <stdio.h>
#include <iostream>
#include <string>
#include <functional>
#include "getopt.h"
#include "choppy.h"
#include "pin.h"
#include "adc.h"


#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std;


int main(int argc, char* argv[])
{
    Choppy *choppy = new Choppy();
	
	choppy->open();

	if (!choppy->isConnected()) {
		cout << "Fail to connect to choppy device" << endl;
#ifdef WIN32
		system("pause");
#endif
		return 1;
	}

	cout << "ProductName: " << choppy->getProductName() << endl;
	cout << "ProductRevision: " << choppy->getProductRevision() << endl;
	cout << "ProductSerial: " << choppy->getProductSerial() << endl;
	cout << "FirmwareVersion: " << choppy->getFirmVersion() << endl;

//	choppy->setColor(COLOR_GREEN);
//	choppy->setColor(COLOR_BLUE);
//	choppy->setColor(COLOR_BROWN);
//	choppy->setColor(COLOR_RED);
//	cout << "Color: " << unsigned(choppy->getColor()) << endl;

//	cout << "showReg: " << choppy->showReg() << endl;

	cout << "Start" << endl;

	uint16_t delay = 1000;
	Sleep(delay);

	cout << "Voltage: " << choppy->getVoltage() << endl;
	cout << "Current: " << choppy->getCurrent() << endl;
	Sleep(delay);

	cout << "Voltage: " << choppy->getVoltage() << endl;
	cout << "Current: " << choppy->getCurrent() << endl;
	Sleep(delay);

	cout << "Voltage: " << choppy->getVoltage() << endl;
	cout << "Current: " << choppy->getCurrent() << endl;
	Sleep(delay);

	cout << "Voltage: " << choppy->getVoltage() << endl;
	cout << "Current: " << choppy->getCurrent() << endl;
	Sleep(delay);

	cout << "Voltage: " << choppy->getVoltage() << endl;
	cout << "Current: " << choppy->getCurrent() << endl;
	Sleep(delay);

	cout << "Voltage: " << choppy->getVoltage() << endl;
	cout << "Current: " << choppy->getCurrent() << endl;
	Sleep(delay);

	cout << "Voltage: " << choppy->getVoltage() << endl;
	cout << "Current: " << choppy->getCurrent() << endl;
	Sleep(delay);

	cout << "Voltage: " << choppy->getVoltage() << endl;
	cout << "Current: " << choppy->getCurrent() << endl;
	Sleep(delay);

	cout << "Stop" << endl;


#ifdef WIN32
//	system("pause");
#endif
	return 0;
}
