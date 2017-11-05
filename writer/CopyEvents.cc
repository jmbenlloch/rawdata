////////////////////////////////////////////////////////////////////////
// CopyEvents
//
// Reads the files provided by the DAQ and converts them into ART
// Files containing Digit, DATEHeader and Trigger data Products.
//
////////////////////////////////////////////////////////////////////////

#include "CopyEvents.h"

namespace spd = spdlog;

next::CopyEvents::CopyEvents(ReadConfig * config) :
	run_(0),
	cfptr_(),
	entriesThisFile_(-1),
	event_(),
	eventNo_(0),
	skip_(config->skip()),
	max_events_(config->max_events()),
	buffer_(NULL),
	verbosity_(config->verbosity()),
	twoFiles_(config->two_files())
{
}


void next::CopyEvents::countEvents(std::FILE* file, int * nevents, int * firstEvt){
	*nevents = 0;
	*firstEvt = 0;
	unsigned char * buffer;

	*firstEvt = loadNextEvent(file, &buffer);
	free(buffer);
	int evt_number= *firstEvt;

	while(evt_number > 0){
		(*nevents)++;
		evt_number = loadNextEvent(file, &buffer);
//		free(buffer);
	}
}

//Loads one event from the file
int next::CopyEvents::loadNextEvent(std::FILE* file, unsigned char ** buffer){
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
			}
			evt_number = EVENT_ID_GET_NB_IN_RUN(header.eventId);
			//std::cout << "evt number: " << evt_number << std::endl;
			break;
		}
		bytes_read = fread(&header, 1, headerSize, file);
	}

	return evt_number;
}


void next::CopyEvents::readFile(std::string const & filename, std::string const & filename_out)
{

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
	}

	fout_ = std::fopen(filename_out.c_str(), "w");
	if ( !fout_ ){
		// TODO Failure to open file: must throw FileOpenError.
	}

	countEvents(file1, &nevents1, &firstEvtGDC1);

	if (twoFiles_){
		filename2.replace(filename2.find("gdc1"), 4, "gdc2");

		file2 = std::fopen(filename2.c_str(), "rb");
		if ( !file2 ){
			// TODO Failure to open file: must throw FileOpenError.
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
	}

	//Rewind back to the start of the file
	if (file1) fseek(file1, 0, SEEK_SET);
	if (file2) fseek(file2, 0, SEEK_SET);

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

bool next::CopyEvents::readNext()
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
				fwrite(buffer_, 1, event_->eventSize , fout_);
				free(buffer_);
			}
			//Unless error (missing event), we should read only one event at a time
			break;
		}
	}

	return eventNo_ < entriesThisFile_;
}

bool next::CopyEvents::isEventSelected(eventHeaderStruct& event) const
{
  if (event.eventType == PHYSICS_EVENT ||
		  event.eventType == CALIBRATION_EVENT){
    return true;
  }
  return false;
}

unsigned int next::CopyEvents::readHeaderSize(std::FILE* fptr) const
{
	unsigned char * buf = new unsigned char[4];
	eventHeadSizeType size;
	fseek(fptr, 8, SEEK_CUR);
	fread(buf, 1, 4, fptr);
	size = *buf;
	fseek(fptr, -12, SEEK_CUR);
	return (unsigned int)size;
}
