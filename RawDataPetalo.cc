////////////////////////////////////////////////////////////////////////
// RawDataPetalo
//
// Reads the files provided by the DAQ and converts them into ART
// Files containing Digit, DATEHeader and Trigger data Products.
//
////////////////////////////////////////////////////////////////////////

#include "RawDataPetalo.h"

namespace spd = spdlog;

next::RawDataPetalo::RawDataPetalo()
{
	verbosity_ = 0;
}

next::RawDataPetalo::RawDataPetalo(ReadConfig * config, PetaloWriter * writer) :
	run_(0),
	entriesThisFile_(-1),
	event_(),
	eventNo_(0),
	skip_(config->skip()),
	max_events_(config->max_events()),
	buffer_(NULL),
	verbosity_(config->verbosity()),
	headOut_(),
	eventReader_(),
	discard_(config->discard()),
	fileError_(0)
{
	_log = spd::stdout_color_mt("rawdata");
	_logerr = spd::stderr_color_mt("decoder");
	if(verbosity_ > 0){
		_log->set_level(spd::level::debug);
		_logerr->set_level(spd::level::debug);
	}
	_writer = writer;

	config_ = config;
}


void next::RawDataPetalo::countEvents(std::FILE* file, int * nevents, int * firstEvt){
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
int next::RawDataPetalo::loadNextEvent(std::FILE* file, unsigned char ** buffer){
	int evt_number = -1;

	eventHeaderStruct header;
	unsigned int headerSize = sizeof(eventHeaderStruct);
	unsigned int readSize = this->readHeaderSize(file);
	if ((readSize != headerSize) && readSize) { //if we read 0 is the end of the file
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

std::FILE* next::RawDataPetalo::openDATEFile(std::string const & filename){
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

void next::RawDataPetalo::readFile(std::string const & filename)
{
	int nevents=0, firstEvt;

	fptr_ = openDATEFile(filename);
	countEvents(fptr_, &nevents, &firstEvt);

	_log->info("Reading from file {}", filename);

	entriesThisFile_ = nevents;
	if(max_events_ < entriesThisFile_){
		entriesThisFile_ = max_events_;
	}

	_log->info("Events in file = {}", entriesThisFile_);
	_log->debug("firstEvt: {}", firstEvt);

	//Rewind back to the start of the file
	if (fptr_) fseek(fptr_, 0, SEEK_SET);

	//TODO: Study what is this for here
	eventReader_ = new PetaloEventReader(verbosity_);
}

bool next::RawDataPetalo::readNext()
{
	event_ = 0;
	bool toSkip = eventNo_ < skip_;

	// If we're at the end of the file, don't advance
	if ( eventNo_ == entriesThisFile_ ){
		return false;
	}

	int evt_number = loadNextEvent(fptr_, &buffer_);
	if (evt_number > 0){
		event_ = (eventHeaderStruct*) buffer_;

		eventNo_++;
		if(!toSkip){
			bool result =  ReadDATEEvent();
			std::cout << "result: " << result << std::endl;
			if (result){
				// TODO: deal with event error
	//			if(!eventError_ && discard_){
					std::cout << "call writeEvent" << std::endl;
					writeEvent();
	//			}
			}
			free(buffer_);
			return result;
		}
	}

	return eventNo_ < entriesThisFile_;
}

bool isEventSelected(eventHeaderStruct& event){
  return event.eventType == PHYSICS_EVENT ||
	  event.eventType == CALIBRATION_EVENT;
}

bool next::RawDataPetalo::ReadDATEEvent()
{
	eventHeaderStruct* subEvent = nullptr;
	equipmentHeaderStruct * equipment = nullptr;
	unsigned char* position = nullptr;
	unsigned char* end = nullptr;
	int count = 0;
	eventError_ = false;

	// Reset the output pointers.
	// TODO petalo data: pmtDgts_.reset(new DigitCollection);
	dataVector_.reset(new std::vector<petalo_t>);

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

		//////////Getting firmware version

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
		if(eventReader_->TriggerCounter() != myheader->NbInRun()){
			_logerr->error("EventID ({}) & TriggerCounter ({}) mismatch, possible loss of data in DATE in card {}", myheader->NbInRun(), eventReader_->TriggerCounter(), eventReader_->CardID());
		}else{
			if(verbosity_ >= 2){
				_log->debug("EventID ({}) & TriggerCounter ({}) ok in card {}", myheader->NbInRun(), eventReader_->TriggerCounter(), eventReader_->CardID());
			}
		}

		// Check firmware version here
		// if (fwVersion == 0){
			if( verbosity_ >= 1 ){
				_log->debug("card: {}", eventReader_->CardID());
			}
			// ReadHotelSipm(payload_flip, size);
			ReadTOFPET(payload_flip, size);
		// }

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

unsigned int next::RawDataPetalo::readHeaderSize(std::FILE* fptr) const
{
	unsigned char * buf = new unsigned char[4];
	eventHeadSizeType size;
	fseek(fptr, 8, SEEK_CUR);
	fread(buf, 1, 4, fptr);
	size = *buf;
	fseek(fptr, -12, SEEK_CUR);
	return (unsigned int)size;
}


void next::RawDataPetalo::writeEvent(){
	std::cout << "rawdata: writeEvent" << std::endl;
	auto date_header = (*headOut_).rbegin();
	unsigned int event_number = date_header->NbInRun();
	run_ = date_header->RunNb();

	_writer->Write(*dataVector_);
}


void next::RawDataPetalo::ReadTOFPET(int16_t * buffer, unsigned int size){
	int ErrorBit = eventReader_->GetErrorBit();
	int cardID = eventReader_->CardID();

	if (ErrorBit){
		auto myheader = (*headOut_).rbegin();
		_logerr->error("Event {} ErrorBit is {}, card: {}", myheader->NbInRun(), ErrorBit, cardID);
		fileError_ = true;
		eventError_ = true;
		if(discard_){
			return;
		}
	}

	// TODO: Stop condition?
	for (int i=0; i < 10; i++){
	//while (true){
	//	if ((*buffer == 0xFFFF) && (*(buffer+1) == 0xFFFF)){
	//		break;
	//	}
		printf("decode tofpet %d\n", i);
		decodeTOFPET(buffer, *dataVector_);
	}
}

void next::RawDataPetalo::decodeTOFPET(int16_t * buffer, std::vector<petalo_t>& dataVector){
	petalo_t data;
	data.evt_number  = 0xFFFFFFFF;

	data.tofpet_id   = (*buffer & 0x0E000) >> 13;
	data.wordtype_id = (*buffer & 0x000C0) >>  6;
	data.channel_id  = (*buffer & 0x0003F);
	buffer++;

	data.tac_id      = (*buffer & 0x0C000) >> 14;
	data.tcoarse     = ((*buffer & 0x03FFF) << 2) || ((*(buffer+1) & 0x0C000) >> 14);
	buffer++;

	data.ecoarse     = (*buffer & 0x03FF0) >> 4;
	data.tfine       = ((*buffer & 0x0000F) << 6) || ((*(buffer+1) & 0x0000F) >> 10);
	buffer++;

	data.efine       = (*buffer & 0x003FF);
	buffer++;

	dataVector_->push_back(data);
}
