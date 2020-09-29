////////////////////////////////////////////////////////////////////////
// RawDataPetalo
//
// Reads the files provided by the DAQ
//
////////////////////////////////////////////////////////////////////////

#ifndef _READCONFIG
#include "config/ReadConfig.h"
#endif

#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif

#include "navel/DATEEventHeader.hh"

#ifndef _PETALOWRITER
#include "writer/PetaloWriter.h"
#endif

#ifndef _PETALOEVENTREADER
#include "detail/PetaloEventReader.h"
#endif

#include "detail/event.h"

#include <stdint.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <numeric>
#include <ios>
#include <unistd.h>

#define MEMSIZE 8500000

namespace next {

class RawDataPetalo {

public:
  RawDataPetalo();
  RawDataPetalo(ReadConfig * config, PetaloWriter * writer);

  /// Open specified file.
  void readFile(std::string const & filename);
  std::FILE* openDATEFile(std::string const & filename);
  void countEvents(std::FILE* file, int * events, int * firstEvt);
  int loadNextEvent(std::FILE* file, unsigned char ** buffer);

  /// Read an event.
  bool readNext();

  ///Function to read DATE information
  bool ReadDATEEvent();

  ///Fill PMT classes
  void writeEvent();

  bool errors();

private:
  /// Retrieve DATE event header size stored in the raw data.
  /// 80 bytes for the newer DAQ (DATE event header format 3.14)
  unsigned int readHeaderSize(std::FILE* fptr) const;

  std::unique_ptr<next::EventHeaderCollection> headOut_;

  size_t run_;
  std::FILE* fptr_; // current fptr
  int entriesThisFile_;
  eventHeaderStruct * event_;      // raw data super event
  int eventNo_;
  int skip_;
  int max_events_;
  unsigned char* buffer_;

  // verbosity control
  unsigned int verbosity_;///< default 0 for quiet output.

  ///New PetaloEventReader class
  next::PetaloEventReader *eventReader_;

  //If true, discard FECs with ErrorBit true
  bool discard_;

  std::shared_ptr<spdlog::logger> _log;
  std::shared_ptr<spdlog::logger> _logerr;

  next::PetaloWriter * _writer;

  int fwVersion;
  int fwVersionPmt;

  bool fileError_, eventError_;
  ReadConfig * config_;

};

inline bool RawDataPetalo::errors(){return fileError_;}

}

void flipWords(unsigned int size, int16_t* in, int16_t* out);
bool isEventSelected(eventHeaderStruct& event);
