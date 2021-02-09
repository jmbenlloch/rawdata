////////////////////////////////////////////////////////////////////////
// RawDataInput
//
// Reads the files provided by the DAQ and converts them into ART
// Files containing Digit, DATEHeader and Trigger data Products.
//
////////////////////////////////////////////////////////////////////////

#include "RawDataInput.h"

namespace spd = spdlog;

double const next::RawDataInput::CLOCK_TICK_ = 0.025;

next::RawDataInput::RawDataInput()
{
	verbosity_ = 0;
}

next::RawDataInput::RawDataInput(ReadConfig * config, HDF5Writer * writer) :
	run_(0),
	cfptr_(),
	entriesThisFile_(-1),
	event_(),
	eventNo_(0),
	skip_(config->skip()),
	max_events_(config->max_events()),
	buffer_(NULL),
	dualChannels(48,0),
	verbosity_(config->verbosity()),
	headOut_(),
	pmtDgts_(),
	sipmDgts_(),
	eventReader_(),
	twoFiles_(config->two_files()),
	discard_(config->discard()),
	externalTriggerCh_(config->extTrigger()),
	fileError_(0)
{
	fMaxSample = 65536;
	_log = spd::stdout_color_mt("rawdata");
	_logerr = spd::stderr_color_mt("decoder");
	if(verbosity_ > 0){
		_log->set_level(spd::level::debug);
		_logerr->set_level(spd::level::debug);
	}
	_writer = writer;

	payloadSipm = (char*) malloc(MEMSIZE*NUM_FEC_SIPM);
	config_ = config;

	// Initialize huffman to NULL
	huffman_.next[0] = NULL;
	huffman_.next[1] = NULL;
}


Huffman* next::RawDataInput::getHuffmanTree(){
	return &huffman_;
}

void next::RawDataInput::countEvents(std::FILE* file, int * nevents, int * firstEvt){
	*nevents = 0;
	*firstEvt = 0;
	unsigned char * buffer;

	*firstEvt = loadNextEvent(file, &buffer);
	free(buffer);
	int evt_number= *firstEvt;

	while(evt_number > 0){
		(*nevents)++;
		evt_number = loadNextEvent(file, &buffer);
		// If loadNextEvent enter in the first if, there is no malloc
		// and then free will give a segfault
		if (evt_number > 0){
			free(buffer);
		}
	}
}

//Loads one event from the file
int next::RawDataInput::loadNextEvent(std::FILE* file, unsigned char ** buffer){
	int evt_number = -1;

	eventHeaderStruct header;
	unsigned int headerSize = sizeof(eventHeaderStruct);
	unsigned int readSize = this->readHeaderSize(file);
	if ((readSize != headerSize) && readSize) { //if we read 0 is the end of the file
		//TODO throw art::Exception(art::errors::FileReadError;)
		//_logerr->error("Event header size of {} bytes read from data does not match expected size of {}", readSize, headerSize);
		return -1;
	}

	size_t bytes_read = fread(&header, 1, headerSize, file);
	while (bytes_read == headerSize){
		if (!isEventSelected(header)){
			fseek(file, header.eventSize-headerSize, SEEK_CUR);
		}else{
			*buffer = (unsigned char *) malloc(header.eventSize);
			//Come back to the init of event
			fseek(file, -(long)headerSize, SEEK_CUR);
			bytes_read = fread(*buffer, 1, header.eventSize, file);
			//bytes_read += fread(*buffer, 1, header.eventSize-(long)headerSize, file);
			if (bytes_read != header.eventSize ){
				//TODO throw art::Exception(art::errors::FileReadError)
				_logerr->error("Unable to read event from file");
			}
			evt_number = EVENT_ID_GET_NB_IN_RUN(header.eventId);
			//std::cout << "evt number: " << evt_number << std::endl;
			break;
		}
		bytes_read = fread(&header, 1, headerSize, file);
	}

	return evt_number;
}

std::FILE* next::RawDataInput::openDATEFile(std::string const & filename){
	_log->debug("Open file: {}", filename);
	std::FILE* file = std::fopen(filename.c_str(), "rb");
	if ( !file ){
		_log->warn("Unable to open specified DATE file {}. Retrying in 10 seconds...", filename);
		sleep(10);
		file = std::fopen(filename.c_str(), "rb");
		if ( !file ){
			_logerr->error("Unable to open specified DATE file {}", filename);
			fileError_ = true;
			exit(-1);
		}
		_log->warn("File opened succesfully", filename);
	}

	return file;
}

void next::RawDataInput::readFile(std::string const & filename)
{
	bool gdc2first = false;

	int nevents1=0, firstEvtGDC1;
	int nevents2=0, firstEvtGDC2;
	std::FILE* file1 = NULL;
	std::FILE* file2 = NULL;
	std::string filename2 = filename;

	file1 = openDATEFile(filename);
	countEvents(file1, &nevents1, &firstEvtGDC1);

	if (twoFiles_){
		filename2.replace(filename2.find("gdc1"), 4, "gdc2");
		_log->info("Reading from files {} and {}", filename, filename2);

		file2 = openDATEFile(filename2);
		countEvents(file2, &nevents2, &firstEvtGDC2);

		//Check which gdc goes first
		if (firstEvtGDC2 < firstEvtGDC1){
			gdc2first = true;
		}
	}

	entriesThisFile_ = nevents1 + nevents2;
	if(max_events_ < entriesThisFile_){
		entriesThisFile_ = max_events_;
	}
	if(twoFiles_){
		_log->info("Total events: {}, Events in file1: {}, file2: {}", entriesThisFile_, nevents1, nevents2);
		_log->debug("firstEvtGDC1: {}\t firstEvtGDC2: {}", firstEvtGDC1, firstEvtGDC2);
	}else{
		_log->info("Events in file = {}", entriesThisFile_);
		_log->debug("firstEvt: {}", firstEvtGDC1);
	}

	//Rewind back to the start of the file
	if (file1) fseek(file1, 0, SEEK_SET);
	if (file2) fseek(file2, 0, SEEK_SET);

	//TODO: Study what is this for here
	eventReader_ = new EventReader(verbosity_);
	fptr1_ = file1;
	fptr2_ = file2;

	if(twoFiles_ && gdc2first){
		cfptr_ = fptr2_;
		nextGdc2_ = true; //Changes in readNext()
	}else{
		cfptr_ = fptr1_;
		nextGdc2_ = false;
	}
}

bool next::RawDataInput::readNext()
{
	event_ = 0;
	bool toSkip = eventNo_ < skip_;

	// If we're at the end of the file, don't advance
	if ( eventNo_ == entriesThisFile_ ){
		return false;
	}

	//Try 2 times. Usually first time will be ok, but if next
	//event is missing, skip it and go on with the next file
	for(int files=0;files<2;files++){
		//Choose file
		if(twoFiles_){
			if (nextGdc2_){
				cfptr_ = fptr2_;
			}else{
				cfptr_ = fptr1_;
			}
			//Change file for next time
			nextGdc2_ = !nextGdc2_;
		}

		int evt_number = loadNextEvent(cfptr_, &buffer_);
		if (evt_number > 0){
			event_ = (eventHeaderStruct*) buffer_;

			eventNo_++;
			if(!toSkip){
				for(int indexSipm=0;indexSipm<NUM_FEC_SIPM;indexSipm++){
					sipmFec[indexSipm] = false;
				}
				bool result =  ReadDATEEvent();
				if (result){
					if(!eventError_ && discard_){
						writeEvent();
					}
					freeWaveformMemory(&*pmtDgts_);
					freeWaveformMemory(&*sipmDgts_);
				}
				free(buffer_);
				return result;
			}
			//Unless error (missing event), we should read only one event at a time
			break;
		}
	}

	return eventNo_ < entriesThisFile_;
}

bool isEventSelected(eventHeaderStruct& event){
  return event.eventType == PHYSICS_EVENT ||
	  event.eventType == CALIBRATION_EVENT;
}

bool next::RawDataInput::ReadDATEEvent()
{
	eventHeaderStruct* subEvent = nullptr;
	equipmentHeaderStruct * equipment = nullptr;
	unsigned char* position = nullptr;
	unsigned char* end = nullptr;
	int count = 0;
	fFirstFT=0; /// Position in the buffer of the first FT
	fFecId=0;   ///ID of the Front End Card
	eventError_ = false;

	//Num FEB
	std::fill(sipmPosition, sipmPosition+1792,-1);
	std::fill(pmtPosition, pmtPosition+48,-1);

	// Reset the output pointers.
	pmtDgts_.reset(new DigitCollection);
	sipmDgts_.reset(new DigitCollection);
	trigOut_.clear();
	triggerChans_.clear();

	if (!event_) return false;

	// now fill DATEEventHeader
	headOut_.reset(new EventHeaderCollection);
	(*headOut_).emplace_back();
	auto myheader = (*headOut_).rbegin();

	myheader->SetEventSize(event_->eventSize);
	myheader->SetEventMagic(event_->eventMagic);
	myheader->SetEventHeadSize(event_->eventHeadSize);
	myheader->SetEventVersion(event_->eventVersion);
	myheader->SetEventType(event_->eventType);
	myheader->SetRunNb(event_->eventRunNb);
	myheader->SetNbInRun(EVENT_ID_GET_NB_IN_RUN(event_->eventId));
	_log->info("Event number: {}", myheader->NbInRun());
	myheader->SetBurstNb( EVENT_ID_GET_BURST_NB(event_->eventId));
	myheader->SetNbInBurst( EVENT_ID_GET_NB_IN_BURST(event_->eventId));

	// check whether there are sub events
	if (event_->eventSize <= event_->eventHeadSize) return false;

	do {
		if (count > 0) position += count;

		// get the first or the next equipment if at the end of an equipment
		if (!equipment || (position >= end)) {
			equipment = nullptr;

			// get the first or the next sub event if at the end of a sub event
			if (!subEvent ||
					(position >= ((unsigned char*)subEvent) + subEvent->eventSize)) {

				// check for end of event data
				if (position >= ((unsigned char*)event_)+event_->eventSize){
					// Ready for save, return successful read.
					return true;
				}

				if (!TEST_SYSTEM_ATTRIBUTE(event_->eventTypeAttribute,
							ATTR_SUPER_EVENT)) {
					subEvent = event_;   // no super event

				} else if (subEvent) {
					subEvent = (eventHeaderStruct*) (((unsigned char*)subEvent) +
							subEvent->eventSize);
				} else {
					subEvent = (eventHeaderStruct*) (((unsigned char*)event_) +
							event_->eventHeadSize);
				}

				if ( verbosity_ >= 2 ) {
					_log->debug("subEvent ldcId: {}", subEvent->eventLdcId);
					_log->debug("subEvent eventSize: {}", subEvent->eventSize);
				}

				// check the magic word of the sub event
				if (subEvent->eventMagic != EVENT_MAGIC_NUMBER) {
					_logerr->error("NEXT1ELEventHandle::ReadDATEEvent, wrong magic number in sub event!");
					return false;
				}

				// continue if no data in the subevent
				if (subEvent->eventSize == subEvent->eventHeadSize) {
					position = end = ((unsigned char*)subEvent) + subEvent->eventSize;
					continue;
				}

				equipment = (equipmentHeaderStruct*)
					(((unsigned char*)subEvent) + subEvent->eventHeadSize);

			} else {
				equipment = (equipmentHeaderStruct*) end;
			}

			position = ((unsigned char*)equipment) + sizeof(equipmentHeaderStruct);
			if (subEvent->eventVersion <= 0x00030001) {
				end = position + equipment->equipmentSize;
			} else {
				end = ((unsigned char*)equipment) + equipment->equipmentSize;
			}
		}

		// continue with the next sub event if no data left in the payload
		if (position >= end){
			printf("Going to next subevent \n");
			continue;
		}

		if ( verbosity_ >= 2 ) {
			_log->debug("equipmentSize: {}", equipment->equipmentSize);
			_log->debug("equipmentType: {}", equipment->equipmentType);
			_log->debug("equipmentId: {}", equipment->equipmentId);
			_log->debug("equipmentBasicElementSize: {}", equipment->equipmentBasicElementSize);
		}

		unsigned char *buffer = position;
		unsigned int size = equipment->equipmentSize - sizeof(equipmentHeaderStruct);

		//////////Getting firmware version: foxtrot, golf, etc //////////////////
		//

		//Flip words
		int16_t * buffer_cp = (int16_t*) buffer;
		int16_t * payload_flip = (int16_t *) malloc(MEMSIZE);
		int16_t * payload_flip_free = payload_flip;
		flipWords(size, buffer_cp, payload_flip);

//		for(int i=0; i<100; i++){
//			printf("payload[%d] = 0x%04x\n", i, payload_flip[i]);
//		}
		eventReader_->ReadCommonHeader(payload_flip);
		fwVersion = eventReader_->FWVersion();
		int FECtype = eventReader_->FecType();
		if(eventReader_->TriggerCounter() != myheader->NbInRun()){
			_logerr->error("EventID ({}) & TriggerCounter ({}) mismatch, possible loss of data in DATE in FEC {}", myheader->NbInRun(), eventReader_->TriggerCounter(), eventReader_->FecId());
		}else{
			if(verbosity_ >= 2){
				_log->debug("EventID ({}) & TriggerCounter ({}) ok in FEC {}", myheader->NbInRun(), eventReader_->TriggerCounter(), eventReader_->FecId());
			}
		}

		//FWVersion HOTEL
		if (fwVersion == 8){
			if (FECtype==0){
				if( verbosity_ >= 1 ){
					_log->debug("This is a PMT FEC");
				}
//				for(unsigned int i=0; i<pmtDgts_->size(); i++){
//					if((*pmtDgts_)[i].active()){
//						std::cout << (*pmtDgts_)[i].nSamples() << " before read pmtDgts " << (*pmtDgts_)[i].chID() << "\t charge[0]: " << (*pmtDgts_)[i].waveform()[0] << std::endl;
//					}
//				}
				ReadHotelPmt(payload_flip,size);
				fwVersionPmt = fwVersion;
//				for(unsigned int i=0; i<pmtDgts_->size(); i++){
//					if((*pmtDgts_)[i].active()){
//						std::cout << (*pmtDgts_)[i].nSamples() << "after read pmtDgts " << (*pmtDgts_)[i].chID() << "\t charge[0]: " << (*pmtDgts_)[i].waveform()[0] << std::endl;
//					}
//				}
			}else if (FECtype==2){
				if( verbosity_ >= 1 ){
					_log->debug("This is a Trigger FEC");
				}
				ReadHotelTrigger(payload_flip,size);
			}else  if (FECtype==1){
				if( verbosity_ >= 1 ){
					_log->debug("This is a SIPM FEC");
				}
				ReadHotelSipm(payload_flip, size);
			}
		}

		//FWVersion INDIA
		if (fwVersion == 9){
			if (FECtype==0){
				if( verbosity_ >= 1 ){
					_log->debug("This is a PMT FEC");
				}
				ReadIndiaJuliettPmt(payload_flip,size);
				fwVersionPmt = fwVersion;
			}else if (FECtype==2){
				if( verbosity_ >= 1 ){
					_log->debug("This is a Trigger FEC");
				}
				ReadIndiaTrigger(payload_flip,size);
			}else  if (FECtype==1){
				if( verbosity_ >= 1 ){
					_log->debug("This is a SIPM FEC");
				}
				ReadHotelSipm(payload_flip, size);
			}
		}

		//FWVersion JULIETT
		if (fwVersion == 10){
			if (FECtype==0){
				if( verbosity_ >= 1 ){
					_log->debug("This is a PMT FEC");
				}
				ReadIndiaJuliettPmt(payload_flip,size);
				fwVersionPmt = fwVersion;
			}else if (FECtype==2){
				if( verbosity_ >= 1 ){
					_log->debug("This is a Trigger FEC");
				}
				ReadIndiaTrigger(payload_flip,size);
			}else  if (FECtype==1){
				if( verbosity_ >= 1 ){
					_log->debug("This is a SIPM FEC");
				}
				ReadHotelSipm(payload_flip, size);
			}
		}

		count = end - position;
		free(payload_flip_free);

	}while(1);

  return true;
}

void flipWords(unsigned int size, int16_t* in, int16_t* out){
	unsigned int pos_in = 0, pos_out = 0;
	// This will stop just before FAFAFAFA, usually there are FFFFFFFF before
	// With compression mode there could be some FAFAFAFA along the data
	// The pattern FF...FFFF FAFAFAFA is a valid one 0,0,0.... 8,8,8,8...
	// The only way to stop safely is to count the words.
	// Each 16-bit word has 2 bytes, therefore pos_in*2 in the condition
	while((pos_in*2 < size)) {
		//Size taken empirically from data (probably due to
		//UDP headers and/or DATE)
		if (pos_in > 0 && pos_in % 3996 == 0){
			pos_in += 2;
		}
		out[pos_out]   = in[pos_in+1];
		out[pos_out+1] = in[pos_in];

//		if(size < 100){
			// printf("out[%d]: 0x%04x\n", pos_out, out[pos_out]);
			// printf("out[%d]: 0x%04x\n", pos_out+1, out[pos_out+1]);

//		}

		pos_in  += 2;
		pos_out += 2;
	}
	// printf("pos_in: %d, size: %d\n", pos_in, size);
}

void next::RawDataInput::ReadIndiaTrigger(int16_t * buffer, unsigned int size){
	int BufferSamples = eventReader_->BufferSamples();
	triggerType_ = eventReader_->TriggerType();

	if(verbosity_ >= 2){
		for(int dbg=0;dbg<9;dbg++){
			_log->debug("TrgConf[{}] = 0x{:04x}", dbg, buffer[dbg]);
			//printf("TrgConf[%d] = 0x%04x\n", dbg, buffer[dbg]);
		}
		_log->debug("Ch info:");
		for(int dbg=0;dbg<4;dbg++){
			_log->debug("Ch info[{}] = 0x{:04x}", dbg, buffer[dbg+6]);
//			printf("Ch info[%d] = 0x%04x\n", dbg, buffer[dbg+6]);
		}
		for(int dbg=0;dbg<4;dbg++){
			_log->debug("Trigger lost[{}] = 0x{:04x}", dbg, buffer[dbg+12]);
//			printf("Trigger lost[%d] = 0x%04x\n", dbg, buffer[dbg+12]);
		}
	}

	//TRG conf 8
	int triggerMask = (*buffer & 0x003FF) << 16;
	buffer++;

	//TRG conf 7
	triggerMask = triggerMask | (*buffer & 0x0FFFF);
	buffer++;

	//TRG conf 6
	int triggerDiff1 = *buffer & 0x0FFFF;
	buffer++;

	//TRG conf 5
	int triggerDiff2 = *buffer & 0x0FFFF;
	buffer++;

	//TRG conf 4
	int triggerWindowA1  =  *buffer & 0x003f;
	int triggerChanA1    = (*buffer & 0x01FC0) >> 6;
	bool autoTrigger     = (*buffer & 0x02000) >> 13;
	bool dualTrigger     = (*buffer & 0x04000) >> 14;
	bool externalTrigger = (*buffer & 0x08000) >> 15;
	buffer++;

	//TRG conf 3
	int triggerWindowB1 =  *buffer & 0x003f;
	int triggerChanB1   = (*buffer & 0x01FC0) >> 6;
	bool mask           = (*buffer & 0x02000) >> 13;
	bool triggerB2      = (*buffer & 0x04000) >> 14;
	bool triggerB1      = (*buffer & 0x08000) >> 15;
	buffer++;

	//TRG conf 2
	int triggerWindowA2 =  *buffer & 0x003f;
	int triggerChanA2   = (*buffer & 0x01FC0) >> 6;
	buffer++;

	//TRG conf 1
	int triggerWindowB2 =  *buffer & 0x003f;
	int triggerChanB2   = (*buffer & 0x01FC0) >> 6;
	buffer++;

	//TRG conf 0
	int triggerExtN =  *buffer & 0x000F;
	int triggerIntN = (*buffer & 0x0FFF0) >> 4;
	buffer++;

	//Trigger type
	int triggerType = (*buffer & 0x0FFFF) >> 15;
	buffer++;

	//Channels producing trigger
	int activePMT = 0;
	int channelNumber = 47;
	for (int chinfo = 0; chinfo < 3; chinfo++){
		for (int j=15;j>=0;j--){
			activePMT = 0;
			activePMT = CheckBit(*buffer & 0x0FFFF, j);

			if (activePMT > 0){
				if( verbosity_ >= 2){
					_log->debug("PMT Number {} is {}", channelNumber, activePMT);
				}
				//printf("ch %d: %d\n", channelNumber, activePMT);
				triggerChans_.push_back(channelNumber);
			}
			channelNumber--;
		}
		buffer++;
	}

	//Trigger lost type 2
	int triggerLost2 = (*buffer & 0x0FFFF) << 16;
	buffer++;
	triggerLost2 = triggerLost2 | (*buffer & 0x0FFFF);
	buffer++;

	//Trigger lost type 1
	int triggerLost1 = (*buffer & 0x0FFFF) << 16;
	buffer++;
	triggerLost1 = triggerLost1 | (*buffer & 0x0FFFF);
	buffer++;

	if(verbosity_ >= 2){
		_log->debug("triggerMask: 0x{:04x}\n", triggerMask);
		_log->debug("triggerDiff1: 0x{:04x}\n", triggerDiff1);
		_log->debug("triggerDiff2: 0x{:04x}\n", triggerDiff2);
		_log->debug("Trg Chan A 1: 0x{:04x}\n", triggerChanA1);
		_log->debug("Trg Window A 1: 0x{:04x}\n", triggerWindowA1);
		_log->debug("autoTrigger: 0x{:04x}04x\n", autoTrigger);
		_log->debug("dualTrigger: 0x{:04x}\n", dualTrigger);
		_log->debug("externalTrigger: 0x{:04x}\n", externalTrigger);
		_log->debug("Trg Chan B 1: 0x{:04x}\n", triggerChanB1);
		_log->debug("Trg Window B 1: 0x{:04x}\n", triggerWindowB1);
		_log->debug("mask: 0x{:04x}\n", mask);
		_log->debug("triggerB2: 0x{:04x}\n", triggerB2);
		_log->debug("triggerB1: 0x{:04x}\n", triggerB1);
		_log->debug("Trg Chan A 2: 0x{:04x}\n", triggerChanA2);
		_log->debug("Trg Window A 2: 0x{:04x}\n", triggerWindowA2);
		_log->debug("Trg Chan B 2: 0x{:04x}\n", triggerChanB2);
		_log->debug("Trg Window B 2: 0x{:04x}\n", triggerWindowB2);
		_log->debug("triggerIntN: 0x{:04x}\n", triggerIntN);
		_log->debug("triggerExtN: 0x{:04x}\n", triggerExtN);
		_log->debug("triggerType: 0x{:04x}\n", triggerType);
		_log->debug("triggerLost2: 0x{:04x}\n", triggerLost2);
		_log->debug("triggerLost1: 0x{:04x}\n", triggerLost1);
	}

	trigOut_.push_back(std::make_pair("triggerType", triggerType));
	trigOut_.push_back(std::make_pair("triggerLost1", triggerLost1));
	trigOut_.push_back(std::make_pair("triggerLost2", triggerLost2));
	trigOut_.push_back(std::make_pair("triggerMask", triggerMask));
	trigOut_.push_back(std::make_pair("triggerDiff1", triggerDiff1));
	trigOut_.push_back(std::make_pair("triggerDiff2", triggerDiff2));
	trigOut_.push_back(std::make_pair("autoTrigger", autoTrigger));
	trigOut_.push_back(std::make_pair("dualTrigger", dualTrigger));
	trigOut_.push_back(std::make_pair("externalTrigger", externalTrigger));
	trigOut_.push_back(std::make_pair("mask", mask));
	trigOut_.push_back(std::make_pair("triggerB2", triggerB2));
	trigOut_.push_back(std::make_pair("triggerB1", triggerB1));
	trigOut_.push_back(std::make_pair("chanA1", triggerChanA1));
	trigOut_.push_back(std::make_pair("chanA2", triggerChanA2));
	trigOut_.push_back(std::make_pair("chanB1", triggerChanB1));
	trigOut_.push_back(std::make_pair("chanB2", triggerChanB2));
	trigOut_.push_back(std::make_pair("windowA1", triggerWindowA1));
	trigOut_.push_back(std::make_pair("windowB1", triggerWindowB1));
	trigOut_.push_back(std::make_pair("windowA2", triggerWindowA2));
	trigOut_.push_back(std::make_pair("windowB2", triggerWindowB2));
	trigOut_.push_back(std::make_pair("triggerIntN", triggerIntN));
	trigOut_.push_back(std::make_pair("triggerExtN", triggerExtN));
}

void next::RawDataInput::ReadHotelTrigger(int16_t * buffer, unsigned int size){
	int temp = 0;
	int BufferSamples = eventReader_->BufferSamples();

	if(verbosity_ >= 2){
		for(int dbg=0;dbg<6;dbg++){
			_log->debug("TrgConf[{}] = 0x{:04x}", dbg, buffer[dbg]);
		}
		_log->debug("Ch info:");
		for(int dbg=0;dbg<4;dbg++){
			_log->debug("Ch info[{}] = 0x{:04x}", dbg, buffer[dbg+6]);
		}
	}

	// Read new fields in the header (TRG conf)
	//TRG conf 5
	temp = (*buffer+0x10000) & 0x0FFFF;
	int triggerMask = ((temp & 0x003FF) << 16);
	int triggerCode = (temp & 0x0FC00) >> 10;
	if(verbosity_ >= 2){
		_log->debug("temp = 0x{:04x}", temp);
		_log->debug("triggerCode: {:x}", triggerCode);
	}

	//TRG conf 4
	buffer++;
	temp = (*buffer+0x10000) & 0x0FFFF;
	triggerMask = triggerMask + temp;
	if(verbosity_ >= 2){
		_log->debug("temp = 0x{:04x}", temp);
		_log->debug("triggerMask: {:x}", triggerMask);
	}

	//TRG conf 3
	buffer++;
	temp = (*buffer+0x10000) & 0x0FFFF;
	int triggerDiff = temp;
	if(verbosity_ >= 2){
		_log->debug("temp = 0x{:04x}", temp);
		_log->debug("triggerDiff: {:x}", triggerDiff);
	}

	//TRG conf 2
	buffer++;
	temp = (*buffer+0x10000) & 0x0FFFF;
	int triggerWindow1 = temp & 0x003f;
	int triggerChan1 = (temp & 0x1FC0) >> 6;
	bool autoTrigger = (temp & 0x02000) >> 13;
	bool dualTrigger = (temp & 0x04000) >> 14;
	bool externalTrigger = (temp & 0x08000) >> 15;
	if(verbosity_ >= 2){
		_log->debug("temp = 0x{:04x}", temp);
		_log->debug("autoTrigger: {:x}", autoTrigger);
		_log->debug("dualTrigger: {:x}", dualTrigger);
		_log->debug("externalTrigger: {:x}", externalTrigger);
		_log->debug("triggerWindow1: {:x}", triggerWindow1);
		_log->debug("triggerChan1: {:x}", triggerChan1);
	}

	//TRG conf 1
	buffer++;
	temp = (*buffer+0x10000) & 0x0FFFF;
	int triggerWindow2 = temp & 0x003f;
	int triggerChan2 = (temp & 0x1FC0) >> 6;
	bool mask = (temp & 0x04000) >> 14;
	bool twoTrigger =  (temp & 0x08000) >> 15;
	if(verbosity_ >= 2){
		_log->debug("temp = 0x{:04x}", temp);
		_log->debug("mask: {:x}", mask);
		_log->debug("twoTrigger: {:x}", twoTrigger);
		_log->debug("triggerWindow2: {:x}", triggerWindow2);
		_log->debug("triggerChan2: {:x}", triggerChan2);
	}

	//TRG conf 0
	buffer++;
	temp = (*buffer+0x10000) & 0x0FFFF;
	int triggerIntN = (temp & 0x0FFF0) >> 4;
	int triggerExtN = temp & 0x000F;
	if(verbosity_ >= 2){
		_log->debug("temp = 0x{:04x}", temp);
		_log->debug("triggerIntN: {:x}", triggerIntN);
		_log->debug("triggerExtN: {:x}", triggerExtN);
	}

	buffer++;
	int activePMT = 0;
	int channelNumber = 63;
	for (int chinfo = 0; chinfo < 4; chinfo++){
		for (int j=15;j>=0;j--){
			activePMT = 0;
			temp = (buffer[chinfo] + 0x10000) & 0x0FFFF;
			activePMT = CheckBit(temp, j);

			if (activePMT > 0){
				if( verbosity_ >= 2){
					_log->debug("PMT Number {} is {}", channelNumber, activePMT);
				}
				triggerChans_.push_back(channelNumber);
			}
			channelNumber--;
		}
	}
	trigOut_.push_back(std::make_pair("triggerCode", triggerCode));
	trigOut_.push_back(std::make_pair("triggerMask", triggerMask));
	trigOut_.push_back(std::make_pair("triggerDiff", triggerDiff));
	trigOut_.push_back(std::make_pair("autoTrigger", autoTrigger));
	trigOut_.push_back(std::make_pair("dualTrigger", dualTrigger));
	trigOut_.push_back(std::make_pair("externalTrigger", externalTrigger));
	trigOut_.push_back(std::make_pair("mask", mask));
	trigOut_.push_back(std::make_pair("channel1", triggerChan1));
	trigOut_.push_back(std::make_pair("channel2", triggerChan2));
	trigOut_.push_back(std::make_pair("window1", triggerWindow1));
	trigOut_.push_back(std::make_pair("window2", triggerWindow2));
	trigOut_.push_back(std::make_pair("triggerIntN", triggerIntN));
	trigOut_.push_back(std::make_pair("triggerExtN", triggerExtN));
}

void next::RawDataInput::ReadHotelPmt(int16_t * buffer, unsigned int size){
	double timeinmus = 0.;
	int time = -1;

	fFecId = eventReader_->FecId();
	eventTime_ = eventReader_->TimeStamp();
	int ZeroSuppression = eventReader_->ZeroSuppression();
	int Baseline = eventReader_->Baseline();
	fPreTrgSamples = eventReader_->PreTriggerSamples();
	int BufferSamples = eventReader_->BufferSamples();
	int TriggerFT = eventReader_->TriggerFT();
	int FTBit = eventReader_->GetFTBit();
	int ErrorBit = eventReader_->GetErrorBit();
	int FWVersion = eventReader_->FWVersion();

	if (ErrorBit){
		auto myheader = (*headOut_).rbegin();
		_logerr->error("Event {} ErrorBit is {}, fec: {}", myheader->NbInRun(), ErrorBit, fFecId);
		fileError_ = true;
		eventError_ = true;
		if(discard_){
			return;
		}
	}

	// Get here channel numbers & dualities
	unsigned int TotalNumberOfPMTs = setDualChannels(eventReader_);

	///Reading the payload
	fFirstFT = TriggerFT;
	timeinmus = 0 - CLOCK_TICK_;
	if(ZeroSuppression){
		int singleBuff = TriggerFT + FTBit*fMaxSample;
		int trigDiff = BufferSamples - fPreTrgSamples;
		fFirstFT = (singleBuff + trigDiff) % BufferSamples;
	}

	int nextFT = -1; //At start we don't know next FT value
	int nextFThm = -1;

	//Map febid -> channelmask
	std::map<int, std::vector<int> > fec_chmask;
	fec_chmask.emplace(fFecId, std::vector<int>());
	int ChannelMask = eventReader_->ChannelMask();
	pmtsChannelMask(ChannelMask, fec_chmask[fFecId], fFecId, FWVersion);

	//Create digits and waveforms for active channels
	CreatePMTs(&*pmtDgts_, pmtPosition, &(fec_chmask[fFecId]), BufferSamples, ZeroSuppression);
	setActiveSensors(&(fec_chmask[fFecId]), &*pmtDgts_, pmtPosition);


	for(unsigned int i=0; i<pmtDgts_->size(); i++){
		int elecID = (*pmtDgts_)[i].chID();
		int pos = pmtPosition[elecID];
//		printf("index: %d, elecID: %d, position: %d\n", i, elecID, pos);
	}

	///Write pedestal
	if(Baseline){
		writePmtPedestals(eventReader_, &*pmtDgts_, &(fec_chmask[fFecId]), pmtPosition);
	}

	//TODO maybe size of payload could be used here to stop, but the size is
	//2x size per link and there are manu FFFF at the end, which are the actual
	//stop condition...
	while (true){
		int FT = *buffer & 0x0FFFF;
		//printf("FT: 0x%04x\n", FT);

		timeinmus = timeinmus + CLOCK_TICK_;
		time++;
		buffer++;

		//TODO ZS
		if(ZeroSuppression){
		//	for(int i=0; i<6; i++){
		//		printf("buffer[%d] = 0x%04x\n", i, buffer[i]);
		//	}

			//stop condition
			//printf("nextword = 0x%04x\n", *buffer);
			if(FT == 0x0FFFF){
				break;
			}

			// Read new FThm bit and channel mask
			int ftHighBit = (*buffer & 0x8000) >> 15;
			ChannelMask = (*buffer & 0x0FF0) >> 4;
			//printf("fthbit: %d, chmask: %d\n", ftHighBit, ChannelMask);
			pmtsChannelMask(ChannelMask, fec_chmask[fFecId], fFecId, FWVersion);

//			for(int i=0; i<fec_chmask[fFecId].size(); i++){
//				printf("pmt channel: %d\n", fec_chmask[fFecId][i]);
//			}

			FT = FT + ftHighBit*fMaxSample - fFirstFT;
			if ( FT < 0 ){
				FT += BufferSamples;
			}

			timeinmus = FT * CLOCK_TICK_;

//			printf("timeinmus: %lf\n", timeinmus);
//			printf("FT: %d\n", FT);

			decodeChargeHotelPmtZS(buffer, *pmtDgts_, fec_chmask[fFecId], pmtPosition, FT);

		}else{
			//If not ZS check next FT value, if not expected (0xffff) end of data
			if(!ZeroSuppression){
				computeNextFThm(&nextFT, &nextFThm, eventReader_);
				if(FT != (nextFThm & 0x0FFFF)){
					if( verbosity_ >= 2 ){
						_log->debug("nextFThm != FT: 0x{:04x}, 0x{:04x}", (nextFThm&0x0ffff), FT);
					}
					break;
				}
			}
			//decodeCharge(buffer, *pmtDgts_, fec_chmask[fFecId], pmtPosition, timeinmus);
			decodeCharge(buffer, *pmtDgts_, fec_chmask[fFecId], pmtPosition, time);
		}
	}

//	for(unsigned int i=0; i<pmtDgts_->size(); i++){
//		if((*pmtDgts_)[i].active()){
//			std::cout << (*pmtDgts_)[i].nSamples() << " hotel pmtDgts " << (*pmtDgts_)[i].chID() << "\t charge[0]: " << (*pmtDgts_)[i].waveform()[0] << std::endl;
//		}
//	}
}

void next::RawDataInput::ReadIndiaJuliettPmt(int16_t * buffer, unsigned int size){
	int time = -1;
	int current_bit = 31;

	fFecId = eventReader_->FecId();
	eventTime_ = eventReader_->TimeStamp();
	triggerType_ = eventReader_->TriggerType();
	int ZeroSuppression = eventReader_->ZeroSuppression();
	int Baseline = eventReader_->Baseline();
	fPreTrgSamples = eventReader_->PreTriggerSamples();
	int BufferSamples = eventReader_->BufferSamples();
	if (eventReader_->FWVersion() == 10){
		if (eventReader_->TriggerType() >= 8){
			fPreTrgSamples = eventReader_->PreTriggerSamples2();
			BufferSamples  = eventReader_->BufferSamples2();
		}
	}
	int TriggerFT = eventReader_->TriggerFT();
	int FTBit = eventReader_->GetFTBit();
	int ErrorBit = eventReader_->GetErrorBit();
	int FWVersion = eventReader_->FWVersion();

	if (ZeroSuppression){
		if ((!huffman_.next[0]) && (!huffman_.next[1])){
			auto myheader = (*headOut_).rbegin();
			run_ = myheader->RunNb();
			getHuffmanFromDB(config_, &huffman_, run_);
			if( verbosity_ >= 1 ){
				_log->debug("Huffman tree:\n");
				print_huffman(_log, &huffman_, 1);
			}
		}
	}

	if (ErrorBit){
		auto myheader = (*headOut_).rbegin();
		_logerr->error("Event {} ErrorBit is {}, fec: {}", myheader->NbInRun(), ErrorBit, fFecId);
		fileError_ = true;
		eventError_ = true;
		if(discard_){
			return;
		}
	}

	// Get here channel numbers & dualities
	unsigned int TotalNumberOfPMTs = setDualChannels(eventReader_);

	///Reading the payload
	fFirstFT = TriggerFT;
	int nextFT = -1; //At start we don't know next FT value
	int nextFThm = -1;

	//Map febid -> channelmask
	std::map<int, std::vector<int> > fec_chmask;
	fec_chmask.emplace(fFecId, std::vector<int>());
	int ChannelMask = eventReader_->ChannelMask();
	pmtsChannelMask(ChannelMask, fec_chmask[fFecId], fFecId, FWVersion);

	//Create digits and waveforms for active channels
	CreatePMTs(&*pmtDgts_, pmtPosition, &(fec_chmask[fFecId]), BufferSamples, ZeroSuppression);
	setActiveSensors(&(fec_chmask[fFecId]), &*pmtDgts_, pmtPosition);


	for(unsigned int i=0; i<pmtDgts_->size(); i++){
		int elecID = (*pmtDgts_)[i].chID();
		int pos = pmtPosition[elecID];
	}

	///Write pedestal
	if(Baseline){
		writePmtPedestals(eventReader_, &*pmtDgts_, &(fec_chmask[fFecId]), pmtPosition);
	}

	//TODO maybe size of payload could be used here to stop, but the size is
	//2x size per link and there are manu FFFF at the end, which are the actual
	//stop condition...
	while (true){
		// timeinmus = timeinmus + CLOCK_TICK_;
		time++;

		if(ZeroSuppression){
			// Skip FTm
			if (time == 0) {
				buffer++;
			}
			if (time == BufferSamples){
				break;
			}
			decodeChargeIndiaPmtCompressed(buffer, &current_bit, *pmtDgts_, &huffman_, fec_chmask[fFecId], pmtPosition, time);
		}else{
			int FT = *buffer & 0x0FFFF;
			buffer++;

			//If not ZS check next FT value, if not expected (0xffff) end of data
			if(!ZeroSuppression){
				computeNextFThm(&nextFT, &nextFThm, eventReader_);
				if(FT != (nextFThm & 0x0FFFF)){
					if( verbosity_ >= 2 ){
						_log->debug("nextFThm != FT: 0x{:04x}, 0x{:04x}", (nextFThm&0x0ffff), FT);
					}
					break;
				}
			}
			decodeCharge(buffer, *pmtDgts_, fec_chmask[fFecId], pmtPosition, time);
		}
	}
}

int next::RawDataInput::setDualChannels(next::EventReader * reader){
	int counter = 0;
	int ElecID = 0;
	int ChannelNumber = 0;
	int CheckBitCounter = 0;
	int dualCh = 0;
	unsigned int numberOfChannels = reader->NumberOfChannels();
	int ChannelMask = reader->ChannelMask();
	int DualModeMask = reader->DualModeMask();
	int FWVersion = eventReader_->FWVersion();

	int maxChannels = 8;
	if (FWVersion == 9){
		maxChannels = 12;
	}

	unsigned int NumberOfPMTs = 0;
	for (int nCh=0; nCh < maxChannels; nCh++){
		if(NumberOfPMTs < numberOfChannels){
			// Check which ones are working
			while (!CheckBitCounter){
				CheckBitCounter = CheckBit(ChannelMask,counter);
				dualCh = CheckBit(DualModeMask,counter);
				counter++;
			}
			CheckBitCounter = 0;
			NumberOfPMTs += 1;

			// Compute PMT ID
			ChannelNumber = counter-1;
			ElecID = computePmtElecID(reader->FecId(), ChannelNumber, FWVersion);
			if (dualCh){
				dualChannels[ElecID] = dualCh;
				int pairCh = channelsRelation[ElecID];
				if (FWVersion == 9){
					pairCh = channelsRelationIndia[ElecID];
				}
				dualChannels[pairCh] = dualCh;
			}
		}
	}
	return NumberOfPMTs;
}

void writePmtPedestals(next::EventReader * reader, next::DigitCollection * pmts, std::vector<int> * elecIDs, int * positions){
	int elecID;
	for(int j=0; j<elecIDs->size(); j++){
		elecID = (*elecIDs)[j];
		auto dgt = pmts->begin() + positions[elecID];
		dgt->setPedestal(reader->Pedestal(j));
	}
}

int computePmtElecID(int fecid, int channel, int fwversion){
	int ElecID;
	int channels_per_fec = 8;

	if(fwversion == 8){
		if (fecid < 4){
			//FECs 2-3
			ElecID = (fecid-2) * channels_per_fec + channel;
		}else{
			//FECs 10-11
			ElecID = (fecid-8) * channels_per_fec + channel;
		}
	}

	if(fwversion == 9){
		/***************************************
		 * 2 -> 0,2,4,6,8,12,14,16,18,20,22    *
		 * 3 -> 1,3,5,7,9,11,13,15,17,19,21,23 *
		 * 10-> 24,26,28, ..., 46              *
		 * 11-> 25,27,29, ..., 47              *
		 * ************************************/
		if (fecid % 2 == 0){
			ElecID = channel*2;
		}else{
			ElecID = channel*2+1;
		}
		if (fecid >= 10){
			ElecID += 24;
		}
	}

	return ElecID;
}

void next::RawDataInput::computeNextFThm(int * nextFT, int * nextFThm, next::EventReader * reader){
	int PreTrgSamples = reader->PreTriggerSamples();
	int BufferSamples = reader->BufferSamples();
	if (reader->FWVersion() == 10){
		if (reader->TriggerType() >= 8){
			fPreTrgSamples = reader->PreTriggerSamples2();
			BufferSamples  = reader->BufferSamples2();
		}
	}
	int FTBit         = reader->GetFTBit();
	int TriggerFT     = reader->TriggerFT();

	//Compute actual FT taking into account FTh bit
	// FTm = FT - PreTrigger
	if (*nextFT == -1){
		*nextFT = (FTBit << 16) + (TriggerFT & 0x0FFFF);
		// Decrease one due to implementation in FPGA
		if (PreTrgSamples > *nextFT){
			*nextFT -= 1;
		}
	}else{
		*nextFT = (*nextFT + 1) % BufferSamples;
	}

	//Compute FTm (with high (17) bit)
	if (PreTrgSamples > *nextFT){
		*nextFThm = BufferSamples - PreTrgSamples + *nextFT;
	}else{
		*nextFThm = *nextFT - PreTrgSamples;
	}

	if( verbosity_ >= 3 ){
		_log->debug("nextFThm: 0x{:05x}", *nextFThm);
		_log->debug("nextFT: 0x{:05x}", *nextFT);
	}
}

void next::RawDataInput::ReadHotelSipm(int16_t * buffer, unsigned int size){
    int16_t *payloadsipm_ptrA;
    int16_t *payloadsipm_ptrB;
	int ErrorBit = eventReader_->GetErrorBit();
	int FecId = eventReader_->FecId();
	int ZeroSuppression = eventReader_->ZeroSuppression();
	unsigned int numberOfFEB = eventReader_->NumberOfChannels();
	int BufferSamples = eventReader_->BufferSamples();
	if (eventReader_->FWVersion() == 10){
		if (eventReader_->TriggerType() >= 8){
			BufferSamples  = eventReader_->BufferSamples2();
		}
	}

	int ChannelMask = eventReader_->ChannelMask();
//	std::cout << "fec: " << FecId << "\tsipm channel mask: " << ChannelMask << std::endl;
//	std::cout << "febs: " << numberOfFEB << std::endl;

	//Check if digits & waveforms has been created
	if(sipmDgts_->size() == 0){
		CreateSiPMs(&(*sipmDgts_), sipmPosition);
		createWaveforms(&*sipmDgts_, BufferSamples/40); //sipms samples 40 slower than pmts
	}

    double timeinmus = 0.;

	if(ErrorBit){
		auto myheader = (*headOut_).rbegin();
		_logerr->error("Event {} ErrorBit is {}, fec: {}", myheader->NbInRun(), ErrorBit, FecId);
		fileError_ = true;
		eventError_ = true;
		if(discard_){
			return;
		}
	}

	//Copy pointers to each FEC payload
	for(int i=0;i<NUM_FEC_SIPM;i++){
		payloadSipmPtr[i] = (int16_t *) (payloadSipm+MEMSIZE*i);
	}

	//Copy data
	for(unsigned int i=0; i<size; i++){
		payloadSipmPtr[FecId][i] = buffer[i];
	}

	//Mark sipm as found
	sipmFec[FecId] = true;

	//Find partner
	int channelA,channelB;
	if(FecId % 2 == 0){
		channelA = FecId;
		channelB = FecId+1;
	}else{
		channelA = FecId-1;
		channelB = FecId;
	}

	//Check if we already have read 2i and 2i+1 channels
	if(sipmFec[channelA] && sipmFec[channelB]){
		_log->debug("A pair of SIPM FECs has been read, decoding...");
		//Choose the correct pointers
		payloadsipm_ptrA = payloadSipmPtr[channelA];
		payloadsipm_ptrB = payloadSipmPtr[channelB];

		//Rebuild payload from the two links
		//TODO free && put correct size
		//int16_t *payload_ptr = (int16_t*) malloc(size*2);
		int16_t *payload_ptr = (int16_t*) malloc(MEMSIZE);
		int16_t *mem_to_free = payload_ptr;
		buildSipmData(size, payload_ptr, payloadsipm_ptrA, payloadsipm_ptrB);

		//read data
		int time = -1;
		//std::vector<int> channelMaskVec;

		//Map febid -> channelmask
		std::map<int, std::vector<int> > feb_chmask;

		std::vector<int> activeSipmsInFeb;
		activeSipmsInFeb.reserve(NUMBER_OF_FEBS);

		int previousFT = 0;
		int nextFT = 0;
		bool endOfData = false;
		while (!endOfData){
			time = time + 1;
			for(unsigned int j=0; j<numberOfFEB; j++){
				//Stop condition for while and for
				if(*payload_ptr == 0xFFFFFFFF){
					endOfData = true;
					break;
				}

				int FEBId = ((*payload_ptr) & 0x0FC00) >> 10;
				payload_ptr++;
				if(verbosity_ >= 3){
					_log->debug("Feb ID is 0x{:04x}", FEBId);
				}

				int FT = (*payload_ptr) & 0x0FFFF;
				if (!ZeroSuppression){
					if(time < 1){
						previousFT = FT;
					}else{
						//New FT only after reading all FEBs in the FEC
						if (j == 0){
							nextFT = ((previousFT + 1) & 0x0FFFF) % (BufferSamples/40);
						}else{
							nextFT = previousFT;
						}
//						printf("pFT: 0x%04x, FT: 0x%04x, condition: %d\n", previousFT, FT, nextFT == FT);
						if(nextFT != FT){
							auto myheader = (*headOut_).rbegin();
							_logerr->error("SiPM Error! Event {}, FECs ({:x}, {:x}), expected FT was {:x}, current FT is {:x}, time {}", myheader->NbInRun(), channelA, channelB, nextFT, FT, time);
							fileError_ = true;
							eventError_ = true;
							//for(int i=-20; i<20; i++){
							//	printf("data[%d]: 0x%04x\n", i, *(payload_ptr+i));
							//}
							if(discard_){
								return;
							}
						}
						previousFT = nextFT;
					}
				}

				timeinmus = computeSipmTime(payload_ptr, eventReader_);

				//If RAW mode, channel mask will appear the first time
				//If ZS mode, channel mask will appear each time
				if (time < 1 || ZeroSuppression){
					feb_chmask.emplace(FEBId, std::vector<int>());
					sipmChannelMask(payload_ptr, feb_chmask[FEBId], FEBId);
					setActiveSensors(&(feb_chmask[FEBId]), &*sipmDgts_, sipmPosition);
				}
				if(ZeroSuppression){
					decodeCharge(payload_ptr, *sipmDgts_, feb_chmask[FEBId], sipmPosition, timeinmus);
				}else{
					decodeCharge(payload_ptr, *sipmDgts_, feb_chmask[FEBId], sipmPosition, time);
				}
				//TODO check this time (originally was time for RAW and timeinmus for ZS)
				//decodeSipmCharge(payload_ptr, channelMaskVec, TotalNumberOfSiPMs, FEBId, timeinmus);
			}
		}
		free(mem_to_free);
	}
}

void setActiveSensors(std::vector<int> * channelMaskVec, next::DigitCollection * pmts, int * positions){
	for(unsigned int i=0; i < channelMaskVec->size(); i++){
//		std::cout << "mask: " << (*channelMaskVec)[i] << std::endl;
		auto dgt = pmts->begin() + positions[(*channelMaskVec)[i]];
		dgt->setActive(true);
	}
}

int next::RawDataInput::pmtsChannelMask(int16_t chmask, std::vector<int> &channelMaskVec, int fecId, int fwversion){
	int TotalNumberOfPMTs = 0;
	int ElecID;

	channelMaskVec.clear();
	for (int t=0; t < 16; t++){
		int bit = CheckBit(chmask, t);
		if(bit>0){
			ElecID = computePmtElecID(fecId, t, fwversion);
			channelMaskVec.push_back(ElecID);
			TotalNumberOfPMTs++;
		}
	}

	//if(verbosity_>=2){
	//	_log->debug("Channel mask is 0x{:04x}", temp);
	//}
	return TotalNumberOfPMTs;
}

//There are 4 16-bit words with the channel mask for SiPMs
//MSB ch63, LSB ch0
//Data came after from 0 to 63
int next::RawDataInput::sipmChannelMask(int16_t * &ptr, std::vector<int> &channelMaskVec, int febId){
	int TotalNumberOfSiPMs = 0;
	int temp;

	int ElecID, sipmNumber;

	channelMaskVec.clear();
	for(int l=4; l>0; l--){
		temp = (*ptr) & 0x0FFFF;
		for (int t=0; t < 16; t++){
			int bit = CheckBit(temp, 15-t);

			ElecID = (febId+1)*1000 + l*16 - t - 1;
			sipmNumber = SipmIDtoPosition(ElecID);

			if(bit>0){
				channelMaskVec.push_back(sipmNumber);
				TotalNumberOfSiPMs++;
			}
		}
		//if(verbosity_>=2){
		//	_log->debug("Channel mask is 0x{:04x}", temp);
		//}
		ptr++;
	}
	std::sort(channelMaskVec.begin(), channelMaskVec.end());
	return TotalNumberOfSiPMs;
}

int next::RawDataInput::computeSipmTime(int16_t * &ptr, next::EventReader * reader){
	int FTBit = reader->GetFTBit();
	int TriggerFT = reader->TriggerFT();
	int PreTrgSamples = reader->PreTriggerSamples();
	int BufferSamples = reader->BufferSamples();
	if (reader->FWVersion() == 10){
		if (reader->TriggerType() >= 8){
			fPreTrgSamples = reader->PreTriggerSamples2();
			BufferSamples  = reader->BufferSamples2();
		}
	}
	int ZeroSuppression = reader->ZeroSuppression();

	int FT = (*ptr) & 0x0FFFF;
	ptr++;

	if(ZeroSuppression){
		int startPosition = ((FTBit<<16)+TriggerFT - PreTrgSamples + BufferSamples)/40 % int(BufferSamples*CLOCK_TICK_);
		FT = FT - startPosition;
		if(FT < 0){
			FT += BufferSamples*CLOCK_TICK_;
		}
	}

	if(verbosity_>=3){
		_log->debug("FT is 0x{:04x}", FT);
	}

	return FT;
}

//Odd words are in ptrA and even words in ptrB
void buildSipmData(unsigned int size, int16_t * ptr, int16_t * ptrA, int16_t * ptrB){
	unsigned int count = 0;
	int temp1, temp2;
	while(count < size){
		temp1 = *ptrA;
		temp2 = *ptrB;

		ptrA++;
		ptrB++;
		count++;

		*ptr = temp1;
		ptr++;
		*ptr = temp2;
		ptr++;
	}
}

void next::RawDataInput::decodeChargeIndiaPmtCompressed(int16_t* &ptr,
	   	int * current_bit, next::DigitCollection &digits, Huffman * huffman,
	   	std::vector<int> &channelMaskVec, int* positions, int time){
	int data = 0;

	for(int chan=0; chan<channelMaskVec.size(); chan++){
		// It is important to keep datatypes, memory allocation changes with them
		int16_t * charge_ptr = (int16_t *) &data;

		if(*current_bit < 16){
			ptr++;
			*current_bit += 16;
		}

		memcpy(charge_ptr+1, ptr  , 2);
		memcpy(charge_ptr  , ptr+1, 2);

//		printf("charge_ptr: 0x%04x\n", data);

		// Get previous value
		auto dgt = digits.begin() + positions[channelMaskVec[chan]];
		int previous = 0;
		if (time){
			previous = dgt->waveform()[time-1];
		}


//		printf("word: 0x%04x\t, ElecID is %d\t Time is %d\t", data, channelMaskVec[chan], time);
		int wfvalue = decode_compressed_value(previous, data, current_bit, huffman);

		if(verbosity_ >= 4){
			 _log->debug("ElecID is {}\t Time is {}\t Charge is 0x{:04x}", channelMaskVec[chan], time, wfvalue);
		}

		//Save data in Digits
		dgt->waveform()[time] = wfvalue;
	}
}

void next::RawDataInput::decodeCharge(int16_t* &ptr, next::DigitCollection &digits, std::vector<int> &channelMaskVec, int* positions, int time){
	//Raw Mode
	int Charge = 0;
	int16_t * payloadCharge_ptr = ptr;

	//We have 64 SiPM per FEB
	for(int chan=0; chan<channelMaskVec.size(); chan++){
		// It is important to keep datatypes, memory allocation changes with them
		int16_t * charge_ptr = (int16_t *) &Charge;

		memcpy(charge_ptr+1, payloadCharge_ptr, 2);
		memcpy(charge_ptr, payloadCharge_ptr+1, 2);

		switch(chan % 4){
			case 0:
				Charge = Charge >> 20;
				Charge = Charge & 0x0fff;
				break;
			case 1:
				Charge = Charge >> 8;
				Charge = Charge & 0x0fff;
				break;
			case 2:
				Charge = Charge >> 12;
				Charge = Charge & 0x0fff;
				break;
			case 3:
				Charge = Charge & 0x0fff;
				break;
		}
		if((chan % 4)==1){
			payloadCharge_ptr+=1;
		}
		if((chan % 4)==3){
			payloadCharge_ptr+=2;
		}

		if(verbosity_ >= 4){
			_log->debug("ElecID is {}\t Time is {}\t Charge is 0x{:04x}", channelMaskVec[chan], time, Charge);
		}

		//Save data in Digits
		auto dgt = digits.begin() + positions[channelMaskVec[chan]];
		dgt->waveform()[time] = Charge;

		// Channel 3 does not add new words
		// 3 words (0123,4567,89AB) give 4 charges (012,345,678,9AB)
		// (012)(3 45)(67 8)(9AB)
		if((chan % 4) != 3){
			ptr++;
		}
	}
}

void next::RawDataInput::decodeChargeHotelPmtZS(int16_t* &ptr, next::DigitCollection &digits, std::vector<int> &channelMaskVec, int* positions, int time){
	//Raw Mode
	int Charge = 0;
	int16_t * payloadCharge_ptr = ptr;

	//We have up to 12 PMTs per link
	for(int chan=0; chan<channelMaskVec.size(); chan++){
		// It is important to keep datatypes, memory allocation changes with them
		int16_t * charge_ptr = (int16_t *) &Charge;

		memcpy(charge_ptr+1, payloadCharge_ptr, 2);
		memcpy(charge_ptr, payloadCharge_ptr+1, 2);

		//There are 12 channels but only 4 cases
		switch(chan % 4){
			case 0:
				Charge = Charge >> 8;
				Charge = Charge & 0x0fff;
				break;
			case 1:
				Charge = Charge >> 12;
				Charge = Charge & 0x0fff;
				break;
			case 2:
				Charge = Charge & 0x0fff;
				break;
			case 3:
				Charge = Charge >> 4;
				Charge = Charge & 0x0fff;
				break;
		}
		if(chan != 1 && chan != 5){
			payloadCharge_ptr+=1;
		}

		// Channel 3 and 7 do not add new words
		if(chan != 2 && chan != 6){
			ptr++;
		}

		if(verbosity_ >= 4){
			_log->debug("ElecID is {}\t Time is {}\t Charge is 0x{:04x}", channelMaskVec[chan], time, Charge);
		}

		//Save data in Digits
		auto dgt = digits.begin() + positions[channelMaskVec[chan]];
		dgt->waveform()[time] = Charge;
	}
	ptr++;
}

void next::RawDataInput::decodeChargeIndiaPmtZS(int16_t* &ptr, next::DigitCollection &digits, std::vector<int> &channelMaskVec, int* positions, int time){
	//Raw Mode
	int Charge = 0;
	int16_t * payloadCharge_ptr = ptr;

	//We have 64 SiPM per FEB
	for(int chan=0; chan<channelMaskVec.size(); chan++){
		// It is important to keep datatypes, memory allocation changes with them
		int16_t * charge_ptr = (int16_t *) &Charge;

		memcpy(charge_ptr+1, payloadCharge_ptr, 2);
		memcpy(charge_ptr, payloadCharge_ptr+1, 2);

		//There are 8 channels but only 4 cases
		switch(chan % 4){
			case 0:
				Charge = Charge >> 4;
				Charge = Charge & 0x0fff;
				break;
			case 1:
				Charge = Charge >> 8;
				Charge = Charge & 0x0fff;
				break;
			case 2:
				Charge = Charge >> 12;
				Charge = Charge & 0x0fff;
				break;
			case 3:
				Charge = Charge & 0x0fff;
				break;
		}
		if(chan != 2 && chan != 6 && chan != 10){
			payloadCharge_ptr+=1;
		}

		// Channel 3 and 7 do not add new words
		//if(chan != 2 && chan != 6){
		if(chan != 3 && chan != 7 && chan != 11){
			ptr++;
		}

		if(verbosity_ >= 4){
			_log->debug("ElecID is {}\t Time is {}\t Charge is 0x{:04x}", channelMaskVec[chan], time, Charge);
		}

		//Save data in Digits
		auto dgt = digits.begin() + positions[channelMaskVec[chan]];
		dgt->waveform()[time] = Charge;
	}
	ptr++;
}

void CreateSiPMs(next::DigitCollection * sipms, int * positions){
	///Creating one class Digit per each SiPM
	int elecID;
	sipms->reserve(1792);
	for (int ch=0; ch<1792; ch++){
		elecID = PositiontoSipmID(ch);
		sipms->emplace_back(elecID, next::digitType::RAW, next::chanType::SIPM);
		positions[ch] = sipms->size() - 1;
	}
}

void createWaveform(next::Digit * sensor, int bufferSamples){
	unsigned int size = bufferSamples * sizeof(unsigned short int);
	unsigned short int * waveform = (unsigned short int*) malloc(size);
	memset(waveform, 0, size);
	sensor->setWaveformNew(waveform);
	sensor->setnSamples(bufferSamples);
}

void createWaveforms(next::DigitCollection * sensors, int bufferSamples){
	for(unsigned int i=0; i<sensors->size(); i++){
		createWaveform(&((*sensors)[i]), bufferSamples);
	}
}

void CreatePMTs(next::DigitCollection * pmts, int * positions, std::vector<int> * elecIDs, int bufferSamples, bool zs){
	///Creating one class Digit per each MT
	for (unsigned int i=0; i < elecIDs->size(); i++){
		if (zs){
			pmts->emplace_back((*elecIDs)[i], next::digitType::RAWZERO, next::chanType::PMT);
		}else{
			pmts->emplace_back((*elecIDs)[i], next::digitType::RAW,     next::chanType::PMT);
		}
		positions[(*elecIDs)[i]] = pmts->size() - 1;
		next::Digit * dgt = &(pmts->back());
		createWaveform(dgt, bufferSamples);
	}
}

unsigned int next::RawDataInput::readHeaderSize(std::FILE* fptr) const
{
	unsigned char * buf = new unsigned char[4];
	eventHeadSizeType size;
	fseek(fptr, 8, SEEK_CUR);
	fread(buf, 1, 4, fptr);
	size = *buf;
	fseek(fptr, -12, SEEK_CUR);
	return (unsigned int)size;
}


void next::RawDataInput::writeEvent(){
	DigitCollection extPmt;
	DigitCollection pmts;
	DigitCollection blrs;
	DigitCollection sipms;
	pmts.reserve(32);
	blrs.reserve(32);
	sipms.reserve(1792);

	auto erIt = pmtDgts_->begin();
	while ( erIt != pmtDgts_->end() ){
		if (fwVersionPmt == 8){
			int chid = (*erIt).chID();
			bool delCh = false;
			if (dualChannels[chid] && (*erIt).active()){
				//In the first fec
//				if(chid <=15){
					// the upper channel is the BLR one
					if(chid > channelsRelation[chid]){
						(*erIt).setChID(channelsRelation[chid]);
						blrs.push_back(*erIt);
						erIt = (*pmtDgts_).erase( erIt );
						delCh = true;
					}else{
						pmts.push_back(*erIt);
					}
//				}else{
//					// In the second fec the lower channel is the BLR one
//					if(chid < channelsRelation[chid]){
//						(*erIt).setChID(channelsRelation[chid]);
//						blrs.push_back(*erIt);
//						erIt = (*pmtDgts_).erase( erIt );
//						delCh = true;
//					}else{
//						pmts.push_back(*erIt);
//					}
//				}
			}else{
				if (chid == externalTriggerCh_){
					if( (*erIt).active()){
						extPmt.push_back(*erIt);
					}
				}else{
					pmts.push_back(*erIt);
				}
			}
			if(!delCh){
				++erIt;
			}
		}

		if(fwVersionPmt >= 9){
			int chid = (*erIt).chID();
			if ((*erIt).active()){
				// 0-11 ->   0- 11 Real
				//12-23 -> 100-111 Dual
				//24-35 ->  12- 23 Real
				//36-47 -> 112-123 Dual
				if(chid <= 11){
					pmts.push_back(*erIt);
				}
				if((chid >= 12) && (chid <= 23)){
					int newid = channelsRelationIndia[chid];
					(*erIt).setChID(newid);
					blrs.push_back(*erIt);
				}
				if((chid >= 24) && (chid <= 35)){
					int newid = chid - 12;
					(*erIt).setChID(newid);
					pmts.push_back(*erIt);
				}
				if((chid >= 36) && (chid <= 47)){
					int newid = channelsRelationIndia[chid] - 12;
					(*erIt).setChID(newid);
					blrs.push_back(*erIt);
				}
			}
			++erIt;
		}
	}

	auto date_header = (*headOut_).rbegin();
	unsigned int event_number = date_header->NbInRun();
	run_ = date_header->RunNb();

	_writer->Write(pmts, blrs, extPmt, *sipmDgts_, trigOut_, triggerChans_, triggerType_, eventTime_, event_number, run_);
}

void freeWaveformMemory(next::DigitCollection * sensors){
	for(unsigned int i=0; i< sensors->size(); i++){
		free((*sensors)[i].waveform());
	}
}
