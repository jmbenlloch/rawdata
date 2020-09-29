#ifndef _PETALOEVENTREADER
#define _PETALOEVENTREADER
#endif

#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif

#include <vector>
#include <iostream>
#include <stdint.h>
#include <string.h>

namespace next {

  /// PetaloEventReader is a class to store the main methods to read data
  /// The aim of this class is to do the data reading framwork independent

  class PetaloEventReader
  {
  public:
    /// Default constructor
    PetaloEventReader(int verbose);
    /// Destructor
    ~PetaloEventReader();

    void FlipWords(int16_t* &, int16_t* &);
    void ReadCommonHeader(int16_t* &ptr);
	void readSeqCounter(int16_t* &ptr);
	void readFormatID(int16_t* &ptr);
	void readWordCount(int16_t* &ptr);
	void readEventID(int16_t* &ptr);
	void readCardID(int16_t* &ptr);

    unsigned int FWVersion() const;
    unsigned int SequenceCounter() const;
    unsigned int WordCounter() const;
    unsigned int CardID() const;
    int GetErrorBit() const;
    int TriggerCounter() const;

  private:
    unsigned int fFWVersion;
    unsigned int fFormatType;
    unsigned int fSequenceCounter;
    unsigned int fWordCounter;
	unsigned int fCardID;
    int fChannelMask;
    int fTriggerCounter;
    int fErrorBit;
	int verbose_;

	std::shared_ptr<spdlog::logger> _log;

  }; //class PetaloEventReader

  typedef std::vector<PetaloEventReader> PetaloEventReaderCollection;

  // INLINE METHODS //////////////////////////////////////////////////

  inline unsigned int PetaloEventReader::FWVersion() const {return fFWVersion;}
  inline unsigned int PetaloEventReader::SequenceCounter() const {return fSequenceCounter;}
  inline unsigned int PetaloEventReader::WordCounter() const {return fWordCounter;}
  inline unsigned int PetaloEventReader::CardID() const {return fCardID;}
  inline int PetaloEventReader::GetErrorBit() const {return fErrorBit;}
  inline int PetaloEventReader::TriggerCounter() const {return fTriggerCounter;}


}

int CheckBit(int, int);
