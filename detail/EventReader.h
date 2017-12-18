#ifndef SPDLOG_VERSION
#include "spdlog/spdlog.h"
#endif

#include <vector>
#include <iostream>
#include <stdint.h>
#include <string.h>

namespace next {

  /// EventReader is a class to store the main methods to read data
  /// The aim of this class is to do the data reading framwork independent

  class EventReader
  {
  public:
    /// Default constructor
    EventReader(int verbose);
    /// Destructor
    ~EventReader();

    void FlipWords(int16_t* &, int16_t* &);
    void ReadCommonHeader(int16_t* &ptr);
	void readSeqCounter(int16_t* &ptr);
	void readFormatID(int16_t* &ptr);
	void readWordCount(int16_t* &ptr);
	void readEventID(int16_t* &ptr);
	void readEventConf(int16_t* &ptr);
	void readFecID(int16_t* &ptr);
	void readIndiaFecID(int16_t* &ptr);
	void readCTandFTh(int16_t* &ptr);
	void readFTl(int16_t* &ptr);
	void readHotelBaselines(int16_t* &ptr);
	void readIndiaBaselines(int16_t* &ptr);

    unsigned int FWVersion() const;
    unsigned int FecType() const;
    unsigned int FecId() const;
    unsigned int SequenceCounter() const;
    unsigned int WordCounter() const;
    uint64_t TimeStamp() const;
    bool Baseline() const;
    bool ZeroSuppression() const;
    int ChannelMask() const;
    void SetChannelMask(int mask); //This one is need now fot tests
    void SetDualModeBit(int dual); //This one is need now fot tests
    unsigned int NumberOfChannels() const;
    int PreTriggerSamples() const;
    int BufferSamples() const;
    int TriggerFT() const;
    int Pedestal(int);
    int GetFTBit() const;
    int GetErrorBit() const;
    int GetDualModeBit() const;
    int DualModeMask() const;
    int TriggerCounter() const;
    int TriggerType() const;

    void SetPreTriggerSamples(int PreTriggerSamples); //Needed for tests
    void SetBufferSamples(int BufferSamples); //Needed for tests
    void SetFTBit(int FTBit); //Needed for tests
    void SetTriggerFT(int TriggerFT); //Needed for tests

  private:
    unsigned int fFWVersion;
    unsigned int fFecType;
    unsigned int fFecId;
    unsigned int fSequenceCounter;
    unsigned int fWordCounter;
	uint64_t fTimestamp;
    unsigned int fNumberOfChannels;
    bool fBaseline;
    bool fZeroSuppression;
    int fChannelMask;
    std::vector <int> peds;
    int fPreTriggerSamples;
    int fBufferSamples;
    int fTriggerFT;
    int fTriggerType;
    int fTriggerCounter;
    int fFTBit;
    int fErrorBit;
    int fDualModeBit;
    int fDualModeMask;
	int verbose_;

	std::shared_ptr<spdlog::logger> _log;

  }; //class EventReader

  typedef std::vector<EventReader> EventReaderCollection;

  // INLINE METHODS //////////////////////////////////////////////////

  inline unsigned int EventReader::FWVersion() const {return fFWVersion;}
  inline unsigned int EventReader::FecType() const {return fFecType;}
  inline unsigned int EventReader::FecId() const {return fFecId;}
  inline unsigned int EventReader::SequenceCounter() const {return fSequenceCounter;}
  inline unsigned int EventReader::WordCounter() const {return fWordCounter;}
  inline uint64_t EventReader::TimeStamp() const {return fTimestamp;}
  inline bool EventReader::Baseline() const {return fBaseline;}
  inline bool EventReader::ZeroSuppression() const {return fZeroSuppression;}
  inline int EventReader::ChannelMask() const {return fChannelMask;}
  inline unsigned int EventReader::NumberOfChannels() const {return fNumberOfChannels;}
  inline int EventReader::PreTriggerSamples() const {return fPreTriggerSamples;}
  inline int EventReader::BufferSamples() const  {return fBufferSamples;}
  inline int EventReader::TriggerFT() const {return fTriggerFT;}
  inline int EventReader::GetFTBit() const { return fFTBit; }
  inline int EventReader::GetErrorBit() const {return fErrorBit;}
  inline int EventReader::GetDualModeBit() const {return fDualModeBit;}
  inline int EventReader::DualModeMask() const {return fDualModeMask;}
  inline int EventReader::TriggerCounter() const {return fTriggerCounter;}
  inline int EventReader::TriggerType() const {return fTriggerType;}
  inline void EventReader::SetChannelMask(int mask) {fChannelMask = mask;}
  inline void EventReader::SetDualModeBit(int dual) {fDualModeBit = dual;}

  inline void EventReader::SetPreTriggerSamples(int PreTriggerSamples) {fPreTriggerSamples = PreTriggerSamples;}
  inline void EventReader::SetBufferSamples(int BufferSamples) {fBufferSamples = BufferSamples;}
  inline void EventReader::SetFTBit(int FTBit) {fFTBit = FTBit;}
  inline void EventReader::SetTriggerFT(int TriggerFT) {fTriggerFT = TriggerFT;}

}

int CheckBit(int, int);
