////////////////////////////////////////////////////////////////////////
// RawDataInput
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

#include "navel/Digit.hh"
#include "navel/DATEEventHeader.hh"

#ifndef _HDF5WRITER
#include "writer/HDF5Writer.h"
#endif

#ifndef _EVENTREADER
#include "detail/EventReader.h"
#endif

#include "detail/event.h"

#include <stdint.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <numeric>
#include <ios>
#include <unistd.h>

#define NUM_FEC_SIPM 56
#define PMTS_PER_FEC 8
#define SIPMS_PER_FEB 64
#define NUMBER_OF_FEBS 28
#define MEMSIZE 8500000

#define NSIPMS 3584
#define NPMTS 168

namespace next {

class RawDataInput {

public:
  RawDataInput();
  RawDataInput(ReadConfig * config, HDF5Writer * writer);

  /// Open specified file.
  void readFile(std::string const & filename);
  std::FILE* openDATEFile(std::string const & filename);
  void countEvents(std::FILE* file, int * events, int * firstEvt);
  int loadNextEvent(std::FILE* file, unsigned char ** buffer);

  /// Read an event.
  bool readNext();

  ///Function to read DATE information
  bool ReadDATEEvent();
  void ReadHotelSipm(int16_t * buffer, unsigned int size);
  void ReadHotelPmt(int16_t * buffer, unsigned int size);
  void ReadIndiaJuliettPmt(int16_t * buffer, unsigned int size);
  void ReadHotelTrigger(int16_t * buffer, unsigned int size);
  void ReadIndiaTrigger(int16_t * buffer, unsigned int size);

  ///Fill PMT classes
  int setDualChannels(next::EventReader * reader);
  void computeNextFThm(int * nextFT, int * nextFThm, next::EventReader * reader);

  void decodeCharge(int16_t* &buffer, next::DigitCollection &digits, std::vector<int> &channelMaskVec, int *positions, int time);
  void decodeChargeHotelPmtZS(int16_t* &buffer, next::DigitCollection &digits, std::vector<int> &channelMaskVec, int *positions, int timeinmus);
  void decodeChargeIndiaPmtZS(int16_t* &buffer, next::DigitCollection &digits, std::vector<int> &channelMaskVec, int *positions, int timeinmus);
  void decodeChargeIndiaPmtCompressed(int16_t* &buffer, int *current_bit, next::DigitCollection &digits, Huffman * huffman, std::vector<int> &channelMaskVec, int *positions, int timeinmus);
  int computeSipmTime(int16_t * &ptr, next::EventReader * reader);
  int sipmChannelMask(int16_t * &ptr, std::vector<int> &channelMaskVec, int febId);
  int pmtsChannelMask(int16_t chmask, std::vector<int> &channelMaskVec, int fecId, int FWVersion);

  //function for tests
  Huffman* getHuffmanTree();

  void writeEvent();

  bool errors();

private:
  /// This is temporary (hope) to convert the FT times to us in the digit
  /// tdc.
  static double const CLOCK_TICK_;

  /// Retrieve DATE event header size stored in the raw data.
  /// 80 bytes for the newer DAQ (DATE event header format 3.14)
  unsigned int readHeaderSize(std::FILE* fptr) const;

  size_t run_;
  std::FILE* cfptr_; // current fptr
  std::FILE* fptr1_; // gdc1
  std::FILE* fptr2_; // gdc2
  int entriesThisFile_;
  eventHeaderStruct * event_;      // raw data super event
  int eventNo_;
  int skip_;
  int max_events_;
  unsigned char* buffer_;

  int fFecId; /// Number of the FEC
  int fFirstFT; /// Buffer position in the electronics
  int fPreTrgSamples; ///Number of samples in the pre-trigger
  int fMaxSample; /// Maximum samples in a circular buffer section (65536)

  //Sipm separate streams variables
  //char payloadSipm[5000000*NUM_FEC_SIPM];
  char * payloadSipm;
  bool sipmFec[NUM_FEC_SIPM]; //Store which sipm fec channels has been read
  int16_t * payloadSipmPtr[NUM_FEC_SIPM]; //Store pointers to the payload
  //To aid search of SiPM digits
  int sipmPosition[NSIPMS]; //Num FEBs * 64
  int pmtPosition[NPMTS];

  //Relation between real channels & BLR ones
  const std::vector<int> channelsRelation {2,3,0,1, 6,7,4,5, 10,11,8,9, 14,15,12,13, 18,19,16,17, 22,23,20,21, 26,27,24,25, 30,31,28,29};
  const std::vector<int> channelsRelationIndia {12,13,14,15, 16,17,18,19, 20,21,22,23, 0,1,2,3, 4,5,6,7, 8,9,10,11, 36,37,38,39, 40,41,42,43, 44,45,46,47, 24,25,26,27, 28,29,30,31, 32,33,34,35};

  const std::vector<int> channelsRelationJuliett {
	  12,13,14,15,16,17,18,19,20,21,22,23,
	  0,1,2,3,4,5,6,7,8,9,10,11,
	  36,37,38,39,40,41,42,43,44,45,46,47,
	  24,25,26,27,28,29,30,31,32,33,34,35,
	  60,61,62,63,64,65,66,67,68,69,70,71,
	  48,49,50,51,52,53,54,55,56,57,58,59,
	  84,85,86,87,88,89,90,91,92,93,94,95,
	  72,73,74,75,76,77,78,79,80,81,82,83,
	  108,109,110,111,112,113,114,115,116,117,118,119,
	  96,97,98,99,100,101,102,103,104,105,106,107,
	  132,133,134,135,136,137,138,139,140,141,142,143,
	  120,121,122,123,124,125,126,127,128,129,130,131,
	  156,157,158,159,160,161,162,163,164,165,166,167,
	  144,145,146,147,148,149,150,151,152,153,154,155};

  std::vector<int> dualChannels;

  // verbosity control
  unsigned int verbosity_;///< default 0 for quiet output.

  // To aid output.
  std::uint64_t eventTime_;
  int triggerType_;
  std::vector<int> triggerChans_;
  std::vector<std::pair<std::string, int> > trigOut_;
  std::unique_ptr<next::EventHeaderCollection> headOut_;
  std::unique_ptr<next::DigitCollection> pmtDgts_;
  std::unique_ptr<next::DigitCollection> sipmDgts_;

  ///New EventReader class
  next::EventReader *eventReader_;

  //Attributes to read from two files
  bool twoFiles_; // If true, gdc1 & gdc2 will be read
  bool nextGdc2_; // If true, next event will be read from gdc2

  //If true, discard FECs with ErrorBit true
  bool discard_;

  int externalTriggerCh_;

  std::shared_ptr<spdlog::logger> _log;
  std::shared_ptr<spdlog::logger> _logerr;

  next::HDF5Writer * _writer;

  int fwVersion;
  int fwVersionPmt;

  bool fileError_, eventError_;
  ReadConfig * config_;
  Huffman huffmanPmt_;
  Huffman huffmanSipm_;

};

inline bool RawDataInput::errors(){return fileError_;}

}

void flipWords(unsigned int size, int16_t* in, int16_t* out);
int computePmtElecID(int fecid, int channel, int version);
void buildSipmData(unsigned int size, int16_t* ptr, int16_t * ptrA, int16_t * ptrB);
bool isEventSelected(eventHeaderStruct& event);
void CreateSiPMs(next::DigitCollection * sipms, int * positions);
void CreatePMTs(next::DigitCollection * pmts, int * positions, std::vector<int> * elecIDs, int bufferSize, bool zs);
void freeWaveformMemory(next::DigitCollection * sensors);

void createWaveforms(next::DigitCollection * sensors, int bufferSamples);

void setActiveSensors(std::vector<int> * channelMaskVec, next::DigitCollection * pmts, int *positions);

void writePmtPedestals(next::EventReader * reader, next::DigitCollection * pmts, std::vector<int> * elecIDs, int * positions);
