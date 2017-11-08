#ifndef navel_Products_Digit_hh
#define navel_Products_Digit_hh

#include "navel/pairUtilities.hh"

#include <map>
#include <vector>

namespace next
{

	///
	/// The class Digit represents the most basic Digitized information for next.
	/// It can be RAW, i.e. directly translated from machine code, RAWZERO, like RAW but
	/// with in DAQ zero suppression, CAL, prepared for calibration studies
	/// or PE, i.e. passed through zero suppression and translated into pe,
	/// In all cases it contains a readout trace of the indexed channel with timing
	/// and charge information (in ADC for RAW and PE for PE). In the case of ZEROSUP
	/// type, the Digit also contains information about baseline sigma.
	///

	// Definitions for Digit type and channel type.
	enum digitType {RAW,RAWZERO,CAL,PE};
	enum chanType {PMT,SIPM,NAI,PMAV,SIPMAV};

	class Digit
	{
		public:

			// Default constructor.
			Digit();

			// Constructor with the most basic required information.
			Digit(unsigned int chID, digitType dtype, chanType chtype);

			// Channel ID number.
			unsigned int chID() const;
			void setChID(unsigned int);

			// Digit type, RAW, RAWZERO, CAL or PE
			digitType digType() const;
			void setDigType(digitType dtype);

			// Channel type, PMT, SIPM, PMAV, SIPMAV; last two being averages over a type of sensor
			chanType chType() const;
			void setChType(chanType chtype);

			// The following setters/getters and their data members
			// are most relevant for digitType ZEROSUP
			float pedestal() const;
			void setPedestal(float ped);

			// Number of samples in Digit.
			unsigned int nSample() const;

			// Access to waveform.
			void newSample_end(float tdc, float Q); ///< Add new sample at end of waveform.

			bool isSaturated() const;
			void setSaturated(bool);

			// Failure bit for SiPMs, from fmwk::DAQDigit
			bool isFEBFailureBit() const;
			void setFEBFailureBit(bool s);

			std::map<float,float,util::key_f_comp> waveform();

		private:

			unsigned int channelID_; ///< ID of read-out channel.
			digitType digType_;    ///< Type of digit.
			chanType chanType_;   ///< Type of read-out channel.

			float pedestal_;        ///< The pedestal value which has been/will be subtracted.

			float clockTickWidth_;  ///< Width in us of each sample in waveform.

			bool saturated_;         ///< Saturation flag.

			bool febFailureBit_; ///< Value of error bit for SiPM FEBs.

			//    unsigned int resamplingFactor_; ///< Resampling from raw (necessary?)

		protected:

			std::map<float,float,util::key_f_comp> waveform_; ///< The charge distribution of the digit.


	}; //class Digit

	typedef std::vector<Digit> DigitCollection;

	/// INLINE METHODS

	inline unsigned int Digit::chID() const { return channelID_; }
	inline void Digit::setChID(unsigned int chanid) { channelID_ = chanid; }

	inline digitType Digit::digType() const { return digType_; }
	inline void Digit::setDigType(digitType dtype) { digType_ = dtype; }

	inline chanType Digit::chType() const { return chanType_; }
	inline void Digit::setChType(chanType chtype) { chanType_ = chtype; }

	inline float Digit::pedestal() const { return pedestal_; }
	inline void Digit::setPedestal(float ped) { pedestal_ = ped; }

	inline unsigned int Digit::nSample() const { return waveform_.size(); }

	inline bool Digit::isSaturated() const { return saturated_; }
	inline void Digit::setSaturated(bool sat) { saturated_ = sat; }

	inline bool Digit::isFEBFailureBit() const { return febFailureBit_; }
	inline void Digit::setFEBFailureBit(bool s) { febFailureBit_ = s; }

	inline std::map<float,float,util::key_f_comp> Digit::waveform() { return waveform_; }

} // namespace next

#endif
