//
// uspibind.cpp
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <stdarg.h>
#include <uspios.h>
#include <FreeRTOS.h>
#include <task.h>
#include <video.h>
#include <mailbox.h>
#include <mem.h>

__attribute__((no_instrument_function))
void MsDelay (unsigned nMilliSeconds){
	volatile int* timeStamp = (int*)0x3f003004;
	int stop = *timeStamp + nMilliSeconds * 1000;
	while (*timeStamp < stop) __asm__("nop");
	//vTaskDelay(nMilliSeconds);
}

__attribute__((no_instrument_function))
void usDelay (unsigned nMicroSeconds){
	volatile int* timeStamp = (int*)0x3f003004;
	int stop = *timeStamp + nMicroSeconds;
	while (*timeStamp < stop) __asm__("nop");
}

unsigned StartKernelTimer (unsigned nDelay, TKernelTimerHandler *pHandler, void *pParam, void *pContext){
	println("StartKernelTimer", 0xFFFFFFFF);
	return 1;//TimerStartKernelTimer (TimerGet (), nDelay, pHandler, pParam, pContext);
}

void CancelKernelTimer (unsigned hTimer){
	println("CancelKernelTimer", 0xFFFFFFFF);//TimerCancelKernelTimer (TimerGet (), hTimer);
}

//void ConnectInterrupt (unsigned nIRQ, TInterruptHandler *pfnHandler, void *pParam){
void ConnectInterrupt (unsigned nIRQ, FN_INTERRUPT_HANDLER pfnHandler, void *pParam){
	//DisableInterrupts();
	RegisterInterrupt(nIRQ, pfnHandler, pParam);
	EnableInterrupt(nIRQ);
	//EnableInterrupts();
}

int SetPowerStateOn (unsigned nDeviceId){
	unsigned int mailbuffer[8] __attribute__((aligned (16)));

	//set power state
	mailbuffer[0] = 8 * 4;		//mailbuffer size
	mailbuffer[1] = 0;			//response code
	mailbuffer[2] = 0x00028001;	//set power state
	mailbuffer[3] = 8;			//value buffer size
	mailbuffer[4] = 8;			//Req. + value length (bytes)
	mailbuffer[5] = nDeviceId;	//device id 3
	mailbuffer[6] = 3;			//state
	mailbuffer[7] = 0;			//terminate buffer

	//spam mail until the response code is ok
	while(mailbuffer[1] != 0x80000000){
		mailboxWrite((int)mailbuffer, 8);
		mailboxRead(8);
	}

	if(!(mailbuffer[6] & 1) || (mailbuffer[6] & 2)) return 0;
	return 1;
}

int GetMACAddress (unsigned char Buffer[6]){
	unsigned int mailbuffer[7] __attribute__((aligned (16)));

	//get MAC
	mailbuffer[0] = 8 * 4;		//mailbuffer size
	mailbuffer[1] = 0;			//response code
	mailbuffer[2] = 0x00010003;	//get mac
	mailbuffer[3] = 6;			//value buffer size
	mailbuffer[4] = 6;			//Req. + value length (bytes)
	mailbuffer[5] = 0;			//12 34 56 AB
	mailbuffer[6] = 0;			//12 34 00 00
	mailbuffer[7] = 0;			//terminate buffer

	//spam mail until the response code is ok
	while(mailbuffer[1] != 0x80000000){
		mailboxWrite((int)mailbuffer, 8);
		mailboxRead(8);
	}

	//memcpy2(Buffer, *(&mailbuffer + 24), 6);
	Buffer[0] = (char)(mailbuffer[5] >> 0);
	Buffer[1] = (char)(mailbuffer[5] >> 8);
	Buffer[2] = (char)(mailbuffer[5] >> 16);
	Buffer[3] = (char)(mailbuffer[5] >> 24);
	Buffer[4] = (char)(mailbuffer[6] >> 0);
	Buffer[5] = (char)(mailbuffer[6] >> 8);

	return 1;
}

__attribute__((no_instrument_function))
void LogWrite (const char *pSource, unsigned Severity, const char *pMessage, ...)
{
	va_list var;
	va_start (var, pMessage);

	// LoggerWriteV (LoggerGet (), pSource, (TLogSeverity) Severity, pMessage, var);
	#define LEN 320
	char buf[LEN];
	int i;
	for (i = 0; i < (sizeof(buf)-1); i++) {
		if ('\0' == *pMessage)
			break;
		if ('%' == *pMessage) {
			pMessage++;			// get next char, the type qualifier;
			char type = *(pMessage++);
			// As long as we have some length specification (for int/string) --> continue
			while (type >= '0' && type <= '9')
				type = *(pMessage++);

			switch (type) {
				case 'u': {
					    unsigned int div = 1000*1000*1000;
						unsigned int v = va_arg(var, unsigned int);
						// println("Unsigned int", 0xffffffff);
						while (div >= 10) {
							char digit = v / div;
							if (0 != digit || 10 == div) {
								buf[i++] = digit + '0';
								v = v % div;
							}
							if (i == sizeof(buf)-1)
								break;
							div = div / 10;
						}
						break;
					}
				case 'x':
				case 'X': {
					unsigned int div = (1 << 28);
					unsigned int v = va_arg(var, unsigned int);
					// println("Unsigned int Hex", 0xffffffff);
					while (div >= 16) {
						char digit = v / div;
						if (0 != digit || 16 == div) {
							if (digit < 10) {
								buf[i++] = digit + '0';
							} else {
								buf[i++] = digit + 'A';
							}
							v = v % div;
						}
						if (i == sizeof(buf)-1)
							break;
						div = div / 16;
					}
				}
				case 's': {
					char * p = va_arg(var, char *);
					// println("String", 0xffffffff);
					while (*p) {
						buf[i++] = *p++;
						if (i == sizeof(buf)-1)
							break;
					}
				}
				default:
				    buf[i++]=type;
			}
		} else
			buf[i] = *pMessage++;
	}
	va_end (var);
	buf[i] = '\0';
	int color;
	switch (Severity) {
		case LOG_ERROR:
			color = RED_TEXT;
			break;
		case LOG_WARNING:
		    color = ORANGE_TEXT;
			break;
		case LOG_NOTICE:
			color = BLUE_TEXT;
			break;
		case LOG_DEBUG:
		default:
			color = WHITE_TEXT;
	}
	println(buf, color);

}

#ifndef NDEBUG

void uspi_assertion_failed (const char *pExpr, const char *pFile, unsigned nLine){	
	println(pExpr, 0xFFFFFFFF);
	println(pFile, 0xFFFFFFFF);
	printHex("Line ", nLine, 0xFFFFFFFF);
	while(1){;} //system failure
}

void DebugHexdump (const void *pBuffer, unsigned nBufLen, const char *pSource){
	println("DebugHexdump", 0xFFFFFFFF);//debug_hexdump (pBuffer, nBufLen, pSource);
}

#endif

void* malloc(unsigned nSize){
	uspi_EnterCritical();
//if(loaded == 2) println("malloc", 0xFFFFFFFF);
	void* temp = pvPortMalloc(nSize);
	uspi_LeaveCritical();
	return temp;

}

void free(void* pBlock){
	uspi_EnterCritical();
	vPortFree(pBlock);
	uspi_LeaveCritical();
}
