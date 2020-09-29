#include "detail/PetaloEventReader.h"
#include <stdio.h>
#include <stdlib.h>

namespace spd = spdlog;

next::PetaloEventReader::PetaloEventReader(int verbose): fFWVersion(0), 
	fSequenceCounter(0), fWordCounter(0), fErrorBit(0),
	verbose_(verbose)
{
	_log = spd::stdout_color_mt("eventreader");
	if(verbose_ > 0){
		_log->set_level(spd::level::debug);
	}
}


next::PetaloEventReader::~PetaloEventReader()
{
}

int CheckBit(int mask, int pos){
	return (mask & (1 << pos)) != 0;
}


void next::PetaloEventReader::readSeqCounter(int16_t* &ptr){
	int temp = 0;
	temp = (*(ptr+1) & 0x0ffff) + (*ptr << 16);
	ptr+=2;

	fSequenceCounter = temp;
	if (verbose_ >= 2){
		_log->debug("Seq counter: 0x{:04x}", fSequenceCounter);
	}
}

void next::PetaloEventReader::readFormatID(int16_t* &ptr){
	//Format ID H
	fFormatType =  *ptr & 0x000F;
	fErrorBit  = (*ptr & 0x0020) >> 5;
	ptr++;

	//Format ID L
	fFWVersion = *ptr & 0x0FFFF;
	ptr++;
	if (verbose_ >= 2){
		_log->debug("ErrorBit: {:x}", fErrorBit);
		_log->debug("Format Type {}", fFormatType);
		_log->debug("Firmware version {}", fFWVersion);
	}
}

void next::PetaloEventReader::readWordCount(int16_t* &ptr){
	fWordCounter = *ptr & 0x0FFFF;
	ptr++;
	if (verbose_ >= 2){
		_log->debug("Word counter: 0x{:04x}", fWordCounter);
	}
}

void next::PetaloEventReader::readCardID(int16_t* &ptr){
	fCardID = *ptr & 0x0FFFF;
	ptr++;
	if (verbose_ >= 2){
		_log->debug("Word counter: 0x{:04x}", fWordCounter);
	}
}

void next::PetaloEventReader::readEventID(int16_t* &ptr){
	fTriggerCounter = ((*ptr & 0x0FFFF) << 16) + (*(ptr+1) & 0x0FFFF);
	ptr+=2;
	if (verbose_ >= 2){
		_log->debug("Trigger counter: 0x{:07x}", fTriggerCounter);
	}
}


void next::PetaloEventReader::ReadCommonHeader(int16_t* &ptr){
	readSeqCounter(ptr);
	if (!fSequenceCounter){
		readFormatID(ptr);
		readWordCount(ptr);
		readEventID(ptr);
		readCardID(ptr);
	}
}

