////////////////////////////////////////////////////////////////////////
// CopyEvents
//
// Reads the files provided by the DAQ and converts them into ART
// Files containing Digit, DATEHeader and Trigger data Products.
//
////////////////////////////////////////////////////////////////////////

#ifndef _READCONFIG
#include "config/ReadConfig.h"
#endif

#include "navel/DATEEventHeader.hh"

#include "detail/event.h"
#include <stdint.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <numeric>

namespace next {

class CopyEvents {

public:
  CopyEvents();
  CopyEvents(ReadConfig * config);

  /// Open specified file.
  void readFile(std::string const & filename, std::string const & filename_out);
  void countEvents(std::FILE* file, int * events, int * firstEvt);
  int loadNextEvent(std::FILE* file, unsigned char ** buffer);

  /// Read an event.
  bool readNext();

private:
  /// DATE stuff: check if event of desired type.
  bool isEventSelected(eventHeaderStruct& event) const;

  /// Retrieve DATE event header size stored in the raw data.
  /// 80 bytes for the newer DAQ (DATE event header format 3.14)
  unsigned int readHeaderSize(std::FILE* fptr) const;

  size_t run_;
  std::FILE* cfptr_; // current fptr
  std::FILE* fptr1_; // gdc1
  std::FILE* fptr2_; // gdc2
  std::FILE* fout_; // gdc2
  int entriesThisFile_;
  eventHeaderStruct * event_;      // raw data super event
  int eventNo_;
  int skip_;
  int max_events_;
  unsigned char* buffer_;

  // verbosity control
  unsigned int verbosity_;///< default 0 for quiet output.

  //Attributes to read from two files
  bool twoFiles_; // If true, gdc1 & gdc2 will be read
  bool nextGdc2_; // If true, next event will be read from gdc2

};

}
