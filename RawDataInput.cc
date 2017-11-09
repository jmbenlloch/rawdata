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
	dualChannels(32,0),
	verbosity_(config->verbosity()),
	trigOut_(),
	headOut_(),
	pmtDgts_(),
	sipmDgts_(),
	eventReader_(),
	twoFiles_(config->two_files()),
	discard_(config->discard()),
	externalTriggerCh_(config->extTrigger())
{
	fMaxSample = 65536;
	_log = spd::stdout_color_mt("rawdata");
	if(verbosity_ > 0){
		_log->set_level(spd::level::debug);
	}
	_writer = writer;

	payloadSipm = (char*) malloc(5000000*NUM_FEC_SIPM);
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
		free(buffer);
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
		//_log->error("Event header size of {} bytes read from data does not match expected size of {}", readSize, headerSize);
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
				_log->error("Unable to read event from file");
			}
			evt_number = EVENT_ID_GET_NB_IN_RUN(header.eventId);
			//std::cout << "evt number: " << evt_number << std::endl;
			break;
		}
		bytes_read = fread(&header, 1, headerSize, file);
	}

	return evt_number;
}


void next::RawDataInput::readFile(std::string const & filename)
{
	_log->debug("Open file: {}", filename);

	bool gdc2first = false;

	int nevents1=0, firstEvtGDC1;
	int nevents2=0, firstEvtGDC2;
	std::FILE* file1 = NULL;
	std::FILE* file2 = NULL;
	std::string filename2 = filename;

	//TODO find out how to refactor the file openings
	file1 = std::fopen(filename.c_str(), "rb");
	if ( !file1 ){
		// TODO Failure to open file: must throw FileOpenError.
		_log->error("Unable to open specified DATE file {}", filename);
	}

	countEvents(file1, &nevents1, &firstEvtGDC1);

	if (twoFiles_){
		filename2.replace(filename2.find("gdc1"), 4, "gdc2");
		_log->info("Reading from files {} and {}", filename, filename2);

		file2 = std::fopen(filename2.c_str(), "rb");
		_log->debug("Open file: {}", filename2);
		if ( !file2 ){
			// TODO Failure to open file: must throw FileOpenError.
			_log->error("Unable to open specified DATE file {}", filename2);
		}

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
					//writeEvent();
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

	//Num FEB
	std::fill(sipmPosition, sipmPosition+1792,-1);
	std::fill(pmtPosition, pmtPosition+32,-1);

	// Reset the output pointers.
	pmtDgts_.reset(new DigitCollection);
	sipmDgts_.reset(new DigitCollection);
	CreateSiPMs(&(*sipmDgts_), sipmPosition);
//	(*sipmDgts_).reserve(1792); // 64 * numFEBs

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
					_log->error("NEXT1ELEventHandle::ReadDATEEvent, wrong magic number in sub event!");
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
		int16_t * payload_flip = (int16_t *) malloc(5000000);
		int16_t * payload_flip_free = payload_flip;
		flipWords(size, buffer_cp, payload_flip);

		for(int i=0; i<100; i++){
			printf("payload[%d] = 0x%04x\n", i, payload_flip[i]);
		}
		eventReader_->ReadHotelCommonHeader(payload_flip);
		int FWVersion = eventReader_->FWVersion();
		int FECtype = eventReader_->FecType();

		//FWVersion HOTEL
		if (FWVersion == 8){
			if (FECtype==0){
				if( verbosity_ >= 1 ){
					_log->debug("This is a PMT FEC");
				}
				ReadHotelPmt(payload_flip,size);
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

		///Read and write trigger type class
		int16_t trigger = eventReader_->TriggerType();
		//TODO What is this here?
		SetTriggerType(trigger);

		count = end - position;
		free(payload_flip_free);

	}while(1);

  return true;
}

void flipWords(unsigned int size, int16_t* in, int16_t* out){
	unsigned int pos_in = 0, pos_out = 0;
	//This will stop just before FAFAFAFA, usually there are FFFFFFFF before
	while((pos_in < size) && *(uint32_t *)(&in[pos_in])!= 0xFAFAFAFA ) {
		//Size taken empirically from data (probably due to
		//UDP headers and/or DATE)
		if (pos_in > 0 && pos_in % 3996 == 0){
			pos_in += 2;
		}
		out[pos_out]   = in[pos_in+1];
		out[pos_out+1] = in[pos_in];

		pos_in  += 2;
		pos_out += 2;
	}
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

	//Get channels info
	if ( trigOut_ == nullptr ){
		trigOut_.reset(new next::TriggerCollection);
		(*trigOut_).emplace_back( );
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
					_log->debug("N triggers in vec = {}", (*trigOut_).rbegin()->nTrigChan());
				}
				(*trigOut_).rbegin()->addTrigChan(channelNumber);
			}
			channelNumber--;
		}
	}

	//Copy results to Trigger object
	(*trigOut_).rbegin()->setTriggerCode(triggerCode);
	(*trigOut_).rbegin()->setMask(mask);
	(*trigOut_).rbegin()->setTriggerMask(triggerMask);
	(*trigOut_).rbegin()->setExternalTrigger(externalTrigger);
	(*trigOut_).rbegin()->setDualTrigger(dualTrigger);
	(*trigOut_).rbegin()->setAutoTrigger(autoTrigger);
	(*trigOut_).rbegin()->setTwoTrigger(twoTrigger);
	(*trigOut_).rbegin()->setDiff(triggerDiff);
	(*trigOut_).rbegin()->setChan1(triggerChan1);
	(*trigOut_).rbegin()->setChan2(triggerChan2);
	(*trigOut_).rbegin()->setWindow1(triggerWindow1);
	(*trigOut_).rbegin()->setWindow2(triggerWindow2);
	(*trigOut_).rbegin()->setTriggerIntN(triggerIntN);
	(*trigOut_).rbegin()->setTriggerExtN(triggerExtN);
	(*trigOut_).rbegin()->setDAQbufferSize(BufferSamples);
}

void next::RawDataInput::ReadHotelPmt(int16_t * buffer, unsigned int size){
	double timeinmus = 0.;

	fFecId = eventReader_->FecId();
	eventTime_ = eventReader_->TimeStamp();
	int ZeroSuppression = eventReader_->ZeroSuppression();
	int Baseline = eventReader_->Baseline();
	fPreTrgSamples = eventReader_->PreTriggerSamples();
	int BufferSamples = eventReader_->BufferSamples();
	int TriggerFT = eventReader_->TriggerFT();
	int FTBit = eventReader_->GetFTBit();
	int ErrorBit = eventReader_->GetErrorBit();

	CreatePMTs(fFecId, ZeroSuppression);

	if (ErrorBit){
		auto myheader = (*headOut_).rbegin();
		_log->warn("Event {} ErrorBit is {}, fec: {}", myheader->NbInRun(), ErrorBit, fFecId);
		if(discard_){
			return;
		}
	}

	// Get here channel numbers & dualities
	unsigned int TotalNumberOfPMTs = setDualChannels(eventReader_);

	///Write pedestal
	if(Baseline){
		writePmtPedestals(eventReader_);
	}

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
	pmtsChannelMask(ChannelMask, fec_chmask[fFecId], fFecId);


	//TODO maybe size of payload could be used here to stop, but the size is
	//2x size per link and there are manu FFFF at the end, which are the actual
	//stop condition...
	while (true){
		int FT = *buffer & 0x0FFFF;
		printf("FT: 0x%04x\n", FT);

		timeinmus = timeinmus + CLOCK_TICK_;
		buffer++;

		//TODO ZS
		if(ZeroSuppression){
			for(int i=0; i<6; i++){
				printf("buffer[%d] = 0x%04x\n", i, buffer[i]);
			}

			//stop condition
			printf("nextword = 0x%04x\n", *buffer);
			if(FT == 0x0FFFF){
				break;
			}

			// Read new FThm bit and channel mask
			int ftHighBit = (*buffer & 0x8000) >> 15;
			ChannelMask = (*buffer & 0x0FF0) >> 4;
			printf("fthbit: %d, chmask: %d\n", ftHighBit, ChannelMask);
			pmtsChannelMask(ChannelMask, fec_chmask[fFecId], fFecId);

			for(int i=0; i<fec_chmask[fFecId].size(); i++){
				printf("pmt channel: %d\n", fec_chmask[fFecId][i]);
			}

			FT = FT + ftHighBit*fMaxSample - fFirstFT;
			if ( FT < 0 ){
				FT += BufferSamples;
			}

			timeinmus = FT * CLOCK_TICK_;

			printf("timeinmus: %lf\n", timeinmus);
			printf("FT: %d\n", FT);

			decodeChargePmtZS(buffer, *pmtDgts_, fec_chmask[fFecId], pmtPosition, timeinmus);

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
			decodeCharge(buffer, *pmtDgts_, fec_chmask[fFecId], pmtPosition, timeinmus);
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

	unsigned int NumberOfPMTs = 0;
	for (int nCh=0; nCh < 8; nCh++){
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
			ElecID = computePmtElecID(reader->FecId(), ChannelNumber);
			if (dualCh){
				dualChannels[ElecID] = dualCh;
				int pairCh = channelsRelation[ElecID];
				dualChannels[pairCh] = dualCh;
			}
		}
	}
	return NumberOfPMTs;
}

void next::RawDataInput::writePmtPedestals(next::EventReader * reader){
	int elecID;
	for(int j=0; j<8; j++){
		elecID = computePmtElecID(reader->FecId(), j);
		auto dgt = (*pmtDgts_).begin() + pmtPosition[elecID];
		dgt->setPedestal(reader->Pedestal(j));
	}
}

int computePmtElecID(int fecid, int channel){
	int ElecID;
	if (fecid < 4){
		//FECs 2-3
		ElecID = (fecid-2)*8 + channel;
	}else{
		//FECs 10-11
		ElecID = (fecid-8)*8 + channel;
	}
	return ElecID;
}

void next::RawDataInput::computeNextFThm(int * nextFT, int * nextFThm, next::EventReader * reader){
	int PreTrgSamples = reader->PreTriggerSamples();
	int BufferSamples = reader->BufferSamples();
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

    double timeinmus = 0.;

	if(ErrorBit){
		auto myheader = (*headOut_).rbegin();
		_log->warn("Event {} ErrorBit is {}, fec: {}", myheader->NbInRun(), ErrorBit, FecId);
		if(discard_){
			return;
		}
	}

	//Copy pointers to each FEC payload
	for(int i=0;i<NUM_FEC_SIPM;i++){
		payloadSipmPtr[i] = (int16_t *) (payloadSipm+5000000*i);
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
		int16_t *payload_ptr = (int16_t*) malloc(5000000);
		int16_t *mem_to_free = payload_ptr;
		buildSipmData(size, payload_ptr, payloadsipm_ptrA, payloadsipm_ptrB);

		//read data
		int time = -1;
		//std::vector<int> channelMaskVec;

		//Map febid -> channelmask
		std::map<int, std::vector<int> > feb_chmask;

		std::vector<int> activeSipmsInFeb;
		activeSipmsInFeb.reserve(NUMBER_OF_FEBS);

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

				timeinmus = computeSipmTime(payload_ptr, eventReader_);

				//If RAW mode, channel mask will appear the first time
				//If ZS mode, channel mask will appear each time
				if (time < 1 || ZeroSuppression){
					feb_chmask.emplace(FEBId, std::vector<int>());
					sipmChannelMask(payload_ptr, feb_chmask[FEBId], FEBId);
				}

				decodeCharge(payload_ptr, *sipmDgts_, feb_chmask[FEBId], sipmPosition, time);
				//TODO check this time (originally was time for RAW and timeinmus for ZS)
				//decodeSipmCharge(payload_ptr, channelMaskVec, TotalNumberOfSiPMs, FEBId, timeinmus);
			}
		}
		free(mem_to_free);
	}
}

int next::RawDataInput::pmtsChannelMask(int16_t chmask, std::vector<int> &channelMaskVec, int fecId){
	int TotalNumberOfPMTs = 0;
	int ElecID;

	channelMaskVec.clear();
	for (int t=0; t < 16; t++){
		int bit = CheckBit(chmask, t);
		if(bit>0){
			ElecID = computePmtElecID(fecId, t);
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
int next::RawDataInput::sipmChannelMask(int16_t * &ptr, std::vector<int> &channelMaskVec, int febId){
	int TotalNumberOfSiPMs = 0;
	int temp;

	int ElecID, sipmNumber;

	channelMaskVec.clear();
	for(int l=0; l<4; l++){
		temp = (*ptr) & 0x0FFFF;
		for (int t=0; t < 16; t++){
			int bit = CheckBit(temp, 15-t);

			ElecID = (febId+1)*1000 + l*16 + t;
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
	return TotalNumberOfSiPMs;
}

int next::RawDataInput::computeSipmTime(int16_t * &ptr, next::EventReader * reader){
	int FTBit = reader->GetFTBit();
	int TriggerFT = reader->TriggerFT();
	int PreTrgSamples = reader->PreTriggerSamples();
	int BufferSamples = reader->BufferSamples();
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

//TODO take RAW or RAWZERO as input arg
void next::RawDataInput::decodeCharge(int16_t* &ptr, next::DigitCollection &digits, std::vector<int> &channelMaskVec, int* positions, double time){
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
		dgt->newSample_end(time, Charge);

		// Channel 3 does not add new words
		// 3 words (0123,4567,89AB) give 4 charges (012,345,678,9AB)
		// (012)(3 45)(67 8)(9AB)
		if((chan % 4) != 3){
			ptr++;
		}
	}
}

void next::RawDataInput::decodeChargePmtZS(int16_t* &ptr, next::DigitCollection &digits, std::vector<int> &channelMaskVec, int* positions, double time){
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

		//_log->debug("ElecID is {}\t Time is {}\t Charge is 0x{:04x}", channelMaskVec[chan], time, Charge);
		printf("ElecID is %d\t Time is %lf\t Charge is 0x%04x\n", channelMaskVec[chan], time, Charge);
		if(verbosity_ >= 4){
			_log->debug("ElecID is {}\t Time is {}\t Charge is 0x{:04x}", channelMaskVec[chan], time, Charge);
		}

		//Save data in Digits
		auto dgt = digits.begin() + positions[channelMaskVec[chan]];
		dgt->newSample_end(time, Charge);
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

void next::RawDataInput::CreatePMTs(int fecid, bool zs){
  ///Creating one class Digit per each PMT
	int elecID;
	(*pmtDgts_).reserve(PMTS_PER_FEC);
	// Loop over the existing pmt channels
	for (int j=0; j < PMTS_PER_FEC; j++){
		// We are using links 2-3 and 8-9 for PMTs
		elecID = computePmtElecID(fecid, j);
		if (zs){
			(*pmtDgts_).emplace_back(elecID, digitType::RAW,     chanType::PMT);
		}else{
			(*pmtDgts_).emplace_back(elecID, digitType::RAWZERO, chanType::PMT);
		}
		pmtPosition[elecID] = (*pmtDgts_).size() - 1;
	}
}

void next::RawDataInput::SetTriggerType(int16_t Trigger)
{
	if ( trigOut_ == nullptr ){
		// Get the information into the unique_ptr
		trigOut_.reset(new next::TriggerCollection);
		(*trigOut_).emplace_back( Trigger );
	} else
		(*(*trigOut_).rbegin()).setTriggerType( Trigger );

	(*(*trigOut_).rbegin()).setPreTrigger(fPreTrgSamples);
	(*(*trigOut_).rbegin()).setFecID(fFecId);
	(*(*trigOut_).rbegin()).setBufferPosition(fFirstFT);
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
		int chid = (*erIt).chID();
		bool delCh = false;
		if (dualChannels[chid] && (*erIt).waveform().size() > 0){
			//In the first fec
			if(chid <=15){
				// the upper channel is the BLR one
				if(chid > channelsRelation[chid]){
					(*erIt).setChID(channelsRelation[chid]);
					blrs.push_back(*erIt);
					erIt = (*pmtDgts_).erase( erIt );
					delCh = true;
				}else{
					pmts.push_back(*erIt);
				}
			}else{
				// In the second fec the lower channel is the BLR one
				if(chid < channelsRelation[chid]){
					(*erIt).setChID(channelsRelation[chid]);
					blrs.push_back(*erIt);
					erIt = (*pmtDgts_).erase( erIt );
					delCh = true;
				}else{
					pmts.push_back(*erIt);
				}
			}
		}else{
			if (chid == externalTriggerCh_){
				if( (*erIt).waveform().size() > 0){
					extPmt.push_back(*erIt);
				}
			}
		}
		if(!delCh){
			++erIt;
		}
	}

	auto date_header = (*headOut_).rbegin();
	unsigned int event_number = date_header->NbInRun();
	run_ = date_header->RunNb();

	_writer->Write(pmts, blrs, extPmt, *sipmDgts_, eventTime_, event_number, run_);
}
