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



void inthandler(tbiPacket pckt)
{
	cout << "Interrupt received!!" << endl;
	return;
}


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
	cout << "ProductVersion: " << choppy->getFirmVersion() << endl;

	choppy->setColor(COLOR_RED);
	cout << "Color: " << unsigned(choppy->getColor()) << endl;


	cout << "showReg: " << choppy->showReg() << endl;

	cout << "Current: " << choppy->getCurrent() << endl;

	/*
	std::function<void(tbiPacket)> func = inthandler;
	choppy->setInterruptCallback(func);

	function<void(tbiPacket)> callback(tbiPacket pckt);
	*/


#ifdef WIN32
	system("pause");
#endif
	return 0;
}
