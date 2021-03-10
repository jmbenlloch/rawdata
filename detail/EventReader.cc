#include "detail/EventReader.h"
#include <stdio.h>
#include <stdlib.h>

namespace spd = spdlog;

next::EventReader::EventReader(int verbose): fFWVersion(0), fFecType(0),
	fFecId(0), fSequenceCounter(0), fWordCounter(0), fTimestamp(0),
	fNumberOfChannels(0), fBaseline(false), fZeroSuppression(false),
	fChannelMask(0), peds(), fPreTriggerSamples(0), fBufferSamples(0),
	fTriggerFT(0), fErrorBit(0), fDualModeBit(0), fDualModeMask(0),
	verbose_(verbose)
{
	_log = spd::stdout_color_mt("eventreader");
	if(verbose_ > 0){
		_log->set_level(spd::level::debug);
	}
}


next::EventReader::~EventReader()
{
}

int CheckBit(int mask, int pos){
	return (mask & (1 << pos)) != 0;
}

int next::EventReader::Pedestal(int i){
	int pedestal;
	if (peds.size() > ((unsigned int)i)){
		pedestal = peds[i];

	}else{
		pedestal=-1;
	}

	return pedestal;
}

void next::EventReader::readSeqCounter(int16_t* &ptr){
	int temp = 0;
	temp = (*(ptr+1) & 0x0ffff) + (*ptr << 16);
	ptr+=2;

	fSequenceCounter = temp;
	if (verbose_ >= 2){
		_log->debug("Seq counter: 0x{:04x}", fSequenceCounter);
	}
}

void next::EventReader::readFormatID(int16_t* &ptr){
	//Format ID H
	fFecType         =  *ptr & 0x000F;
	fZeroSuppression = (*ptr & 0x0010) >>  4;
	fBaseline        = (*ptr & 0x0040) >>  6;
	fDualModeBit     = (*ptr & 0x0080) >>  7;
	fErrorBit        = (*ptr & 0x4000) >> 14;
	ptr++;

	//Format ID L
	fFWVersion = *ptr & 0x0FFFF;
	ptr++;
	if (verbose_ >= 2){
		_log->debug("ErrorBit: {:x}", fErrorBit);
		_log->debug("DualModeBit: {:x}", fDualModeBit);
		_log->debug("Fec type {}", fFecType);
		_log->debug("Baseline {}", fBaseline);
		_log->debug("Zero Suppression {}", fZeroSuppression);
		_log->debug("Firmware version {}", fFWVersion);
	}
}

void next::EventReader::readWordCount(int16_t* &ptr){
	fWordCounter = *ptr & 0x0FFFF;
	ptr++;
	if (verbose_ >= 2){
		_log->debug("Word counter: 0x{:04x}", fWordCounter);
	}
}

void next::EventReader::readEventID(int16_t* &ptr){
	fTriggerType = *ptr & 0x000F;
	fTriggerCounter = ((*ptr & 0x0FFF0) << 12) + (*(ptr+1) & 0x0FFFF);
	ptr+=2;
	if (verbose_ >= 2){
		_log->debug("Trigger type: 0x{:04x}", fTriggerType);
		_log->debug("Trigger counter: 0x{:07x}", fTriggerCounter);
	}
}

void next::EventReader::readEventConf(int16_t* &ptr){
	//Event conf0
	fBufferSamples = 2 * (*ptr & 0x0FFFF);
	ptr++;

	//Event conf1
	fPreTriggerSamples = 2 * (*ptr & 0x0FFFF);
	ptr++;

	//Event conf2
	fChannelMask = *ptr & 0x0FFFF;
	ptr++;

	if (verbose_ >= 2){
		_log->debug("Buffer samples: 0x{:04x}", fBufferSamples);
		_log->debug("Pretrigger samples: 0x{:04x}", fPreTriggerSamples);
		_log->debug("Channel mask: 0x{:04x}", fChannelMask);
	}
}

void next::EventReader::readEventConfJuliett(int16_t* &ptr){
	//Event conf0
	fBufferSamples = 2 * (*ptr & 0x0FFFF);
	ptr++;

	//Event conf1
	fPreTriggerSamples = 2 * (*ptr & 0x0FFFF);
	ptr++;

	//Event conf2
	fBufferSamples2 = 2 * (*ptr & 0x0FFFF);
	ptr++;

	//Event conf3
	fPreTriggerSamples2 = 2 * (*ptr & 0x0FFFF);
	ptr++;

	//Event conf4
	fChannelMask = *ptr & 0x0FFFF;
	ptr++;

	if (verbose_ >= 2){
		_log->debug("Buffer samples: 0x{:04x}", fBufferSamples);
		_log->debug("Pretrigger samples: 0x{:04x}", fPreTriggerSamples);
		_log->debug("Buffer samples 2: 0x{:04x}", fBufferSamples2);
		_log->debug("Pretrigger samples 2: 0x{:04x}", fPreTriggerSamples2);
		_log->debug("Channel mask: 0x{:04x}", fChannelMask);
	}
}

void next::EventReader::readIndiaFecID(int16_t* &ptr){
	//FEC ID
	fNumberOfChannels = *ptr & 0x001F;
	fFecId = (*ptr & 0x0FFE0) >> 5;
	ptr++;

	if (verbose_ >= 2){
		_log->debug("Number of channels: 0x{:04x}", fNumberOfChannels);
		_log->debug("FEC ID: 0x{:04x}", fFecId);
	}
}

void next::EventReader::readFecID(int16_t* &ptr){
	//FEC ID
	fNumberOfChannels = *ptr & 0x001F;
	fFecId = (*ptr & 0x0FFE0) >> 5;
	ptr++;

	//This is for PMTs
	if (fDualModeBit){
		if (fFecId%2==1){
			fDualModeMask = fChannelMask & 0x0FF;
			fChannelMask  = fChannelMask >> 8;
		}else{
			fDualModeMask = (fChannelMask & 0x0FF00) >> 8;
			fChannelMask  = fChannelMask & 0x0FF;
		}
	}

	if (verbose_ >= 2){
		_log->debug("Number of channels: 0x{:04x}", fNumberOfChannels);
		_log->debug("FEC ID: 0x{:04x}", fFecId);
	}
}

void next::EventReader::readCTandFTh(int16_t* &ptr){
	//Timestamp high
	fTimestamp = (*ptr & 0x0FFFF) << 16;
	ptr++;

	//Timestamp Low
	fTimestamp = fTimestamp + (*ptr & 0x0ffff);
	ptr++;

	//FTH & CTms
	fTimestamp = (fTimestamp << 10) + (*ptr & 0x03FF);
	fTimestamp = fTimestamp & 0x03FFFFFFFFFF;

	fFTBit = CheckBit(*ptr, 15);
	ptr++;
	if (verbose_ >= 2){
		_log->debug("Timestamp: {}", fTimestamp);
		_log->debug("FTH: {}", fFTBit);
	}
}

void next::EventReader::readFTl(int16_t* &ptr){
	//Trigger FTl
	fTriggerFT = *ptr & 0x0FFFF;
	if (verbose_ >= 2){
		_log->debug("Trigger FT: 0x{:04x}", fTriggerFT);
	}
	ptr++;
}

void next::EventReader::readHotelBaselines(int16_t* &ptr){
	// Baselines
	// Pattern goes like this (two times):
	// 0xFFF0, 0x000F, 12 bits,  4 bits; ch0, ch1
	// 0xFF00, 0x00FF,  8 bits,  8 bits; ch1, ch2
	// 0x000F, 0x0FFF,  4 bits, 12 bits; ch2, ch3
	int baselineTemp;
	peds.clear();

	//Baseline ch0
	baselineTemp = (*ptr & 0xFFF0) >> 4;
	peds.push_back(baselineTemp);

	//Baseline ch1
	baselineTemp = (*ptr & 0x000F) << 8;
	ptr++;
	baselineTemp = baselineTemp + ((*ptr & 0xFF00)>>8);
	peds.push_back(baselineTemp);

	//Baseline ch2
	baselineTemp = (*ptr & 0x00FF) << 4;
	ptr++;
	baselineTemp = baselineTemp + ((*ptr & 0xF000)>>12);
	peds.push_back(baselineTemp);

	//Baseline ch3
	baselineTemp = (*ptr & 0x0FFF);
	peds.push_back(baselineTemp);

	//Baseline ch4
	ptr++;
	baselineTemp = (*ptr & 0xFFF0) >> 4;
	peds.push_back(baselineTemp);
	baselineTemp = (*ptr & 0x000F) << 8;

	//Baseline ch5
	ptr++;
	baselineTemp = baselineTemp + ((*ptr & 0xFF00)>>8);
	peds.push_back(baselineTemp);
	baselineTemp = (*ptr & 0x00FF) << 4;

	//Baseline ch6
	ptr++;
	baselineTemp = baselineTemp + ((*ptr & 0xF000)>>12);
	peds.push_back(baselineTemp);

	//Baseline ch7
	baselineTemp = (*ptr & 0x0FFF);
	peds.push_back(baselineTemp);
	ptr++;

	if (verbose_ >= 2){
		for(int i=0; i<8; i++){
			_log->debug("Baseline {} is {}", i, peds[i]);
		}
	}
}

void next::EventReader::readIndiaBaselines(int16_t* &ptr){
	// Baselines
	// Pattern goes like this (two times):
	// 0xFFF0, 0x000F, 12 bits,  4 bits; ch0, ch1
	// 0xFF00, 0x00FF,  8 bits,  8 bits; ch1, ch2
	// 0x000F, 0x0FFF,  4 bits, 12 bits; ch2, ch3
	int baselineTemp;
	peds.clear();

	//Baseline ch0
	baselineTemp = (*ptr & 0xFFF0) >> 4;
	peds.push_back(baselineTemp);

	//Baseline ch1
	baselineTemp = (*ptr & 0x000F) << 8;
	ptr++;
	baselineTemp = baselineTemp + ((*ptr & 0xFF00)>>8);
	peds.push_back(baselineTemp);

	//Baseline ch2
	baselineTemp = (*ptr & 0x00FF) << 4;
	ptr++;
	baselineTemp = baselineTemp + ((*ptr & 0xF000)>>12);
	peds.push_back(baselineTemp);

	//Baseline ch3
	baselineTemp = (*ptr & 0x0FFF);
	peds.push_back(baselineTemp);

	//Baseline ch4
	ptr++;
	baselineTemp = (*ptr & 0xFFF0) >> 4;
	peds.push_back(baselineTemp);
	baselineTemp = (*ptr & 0x000F) << 8;

	//Baseline ch5
	ptr++;
	baselineTemp = baselineTemp + ((*ptr & 0xFF00)>>8);
	peds.push_back(baselineTemp);
	baselineTemp = (*ptr & 0x00FF) << 4;
	ptr++;

	if (verbose_ >= 2){
		for(int i=0; i<6; i++){
			_log->debug("Baseline {} is {}", i, peds[i]);
		}
	}
}

void next::EventReader::ReadCommonHeader(int16_t* &ptr){
	readSeqCounter(ptr);
	if (!fSequenceCounter){
		readFormatID(ptr);
		readWordCount(ptr);
		readEventID(ptr);
		if(fFWVersion == 10){
			readEventConfJuliett(ptr);
		}
		else{
			readEventConf(ptr);
		}
		if(fFWVersion == 8){
			if(fBaseline){
				readHotelBaselines(ptr);
			}
			readFecID(ptr);
		}
		if(fFWVersion >= 9){
			if(fBaseline){
				readIndiaBaselines(ptr);
			}
			readIndiaFecID(ptr);
		}
		readCTandFTh(ptr);
		readFTl(ptr);
	}
}

