/*  Toolbit SDK
 *  Copyright (C) 2020 oamax <toolbitorg@gmail.com>
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 */
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "tbi_service.h"


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


TbiService::TbiService(TbiDevice *p) :
	resque(4)
{
	tdev = p;
	thAbort = true;
	interruptHandler = NULL;
}

TbiService::~TbiService()
{
	stop();
}

bool TbiService::start()
{
	if(!thAbort) {
		WARN("Worker thread is already active");
		return true;
	}

	INFO("Start TbiService");
	thAbort = false;
	th = new thread(&TbiService::worker, this);
	return false;
}

bool TbiService::stop()
{
	if(thAbort) {
		WARN("Worker thread is not active");
		return true;
	}
	
	INFO("Stop TbiService");
	thAbort = true;
	th->join();
	delete th;
	return false;
}

bool TbiService::readAttribute(Attribute *att)
{
	tbiPacket pckt;

	pckt.dat[0] = PROTOCOL_VERSION + 4;  // The first byte is Version(7-6bit) + Length(5-0bit).
	pckt.dat[1] = OP_ATT_VALUE_GET;     // Operation Code
	pckt.dat[2] = (att->getAttid() & 0xFF00) >> 8;
	pckt.dat[3] = att->getAttid() & 0xFF;

	if (tdev->isOpen()) {
		INFO("(%04x) OUT : %02x %02x %02x %02x",
			tdev, pckt.dat[0], pckt.dat[1], pckt.dat[2], pckt.dat[3]);
		tdev->write(pckt.dat, 4);
	}
	else {
		return true;
	}

	tbiPacket rcvp = resque.dequeue();
//	tbiPacket rcvp;
//	while(tdev->read(rcvp.dat) == 0);
	INFO("(%04x) IN  : %02x %02x %02x %02x %02x %02x %02x %02x",
		tdev, rcvp.dat[0], rcvp.dat[1], rcvp.dat[2], rcvp.dat[3], rcvp.dat[4], rcvp.dat[5], rcvp.dat[6], rcvp.dat[7]);
	if ((rcvp.dat[0] & 0xC0) == PROTOCOL_VERSION  // Check received packet
		&& rcvp.dat[1] == OP_ATT_VALUE_GET
		&& rcvp.dat[2] == RC_OK)
	{
		int len = (rcvp.dat[0] & 0x3F) - 3;
		return att->setValue(&rcvp.dat[3], len);
	}
	return true;
}

bool TbiService::writeAttribute(Attribute att)
{
	tbiPacket pckt;
	uint8_t len = att.getValueLength();
	uint16_t id = att.getAttid();

	pckt.dat[0] = PROTOCOL_VERSION + 4 + len;  // The first byte is Version(7-6bit) + Length(5-0bit).
	pckt.dat[1] = OP_ATT_VALUE_SET;           // Operation Code
	pckt.dat[2] = (id & 0xFF00) >> 8;
	pckt.dat[3] = id & 0xFF;
	char *p = att.getValueStr();
	for (int i = 0; i < len; i++) {
		pckt.dat[4 + i] = *p++;                // Set value
	}
	if (tdev->isOpen()) {
		INFO("(%04x) OUT : %02x %02x %02x %02x %02x",
			tdev, pckt.dat[0], pckt.dat[1], pckt.dat[2], pckt.dat[3], pckt.dat[4]);
		tdev->write(pckt.dat, 4+len);
	}
	else {
		return true;
	}

	tbiPacket rcvp = resque.dequeue();
//	tbiPacket rcvp;
//	while(tdev->read(rcvp.dat) == 0);
	INFO("(%04x) IN  : %02x %02x %02x %02x %02x %02x %02x %02x",
		tdev, rcvp.dat[0], rcvp.dat[1], rcvp.dat[2], rcvp.dat[3], rcvp.dat[4], rcvp.dat[5], rcvp.dat[6], rcvp.dat[7]);
	if (rcvp.dat[0] == PROTOCOL_VERSION + 3  // Check received packet
		&& rcvp.dat[1] == OP_ATT_VALUE_SET
		&& rcvp.dat[2] == RC_OK)
		return false;

	return true;
}

void TbiService::setInterruptHandler(std::function<void(tbiPacket)> callback)
{
	INFO("setInterruptHandler() is called");
	interruptHandler = callback;
}

struct thread_aborted {};
void TbiService::worker()
{
	tbiPacket rcvp;

//	try {
		while (1) {

			if (tdev->isOpen()) {				
				if (tdev->read(rcvp.dat) > 0) { 
					// Check received packet
					if ((rcvp.dat[0] & 0xC0) == PROTOCOL_VERSION) {
						if (rcvp.dat[1] == OP_INTERRUPT_TRANSFER) {
							INFO("(%04x) IN  : %02x %02x %02x %02x %02x %02x %02x %02x Interrupt",
								tdev, rcvp.dat[0], rcvp.dat[1], rcvp.dat[2], rcvp.dat[3], rcvp.dat[4], rcvp.dat[5], rcvp.dat[6], rcvp.dat[7]);
							if(interruptHandler) {
								interruptHandler(rcvp);
							}
						}
						else if (rcvp.dat[1] == OP_EVT_NOTIFY) {
							eventHandler(rcvp);
						}
						else {
							resque.enqueue(rcvp);
						}
					}	
				}
				else {
//					this_thread::sleep_for(chrono::milliseconds(1));
					this_thread::yield();
				}
			}

			if (thAbort)
				break;
//				throw thread_aborted{};
		}
//	}
//	catch (thread_aborted& e) {
		// nothing to do just exit
//	}
}

/*
void TbiService::interruptHandler(tbiPacket pckt)
{
	int len = (pckt.dat[0] & 0x3F) - 2;
}
*/


void TbiService::eventHandler(tbiPacket pckt)
{
	int len = (pckt.dat[0] & 0x3F) - 3;
	//att->setValue(&pckt.dat[3], 2);
	//att->setValue(&pckt.dat[5], len-2);
}
