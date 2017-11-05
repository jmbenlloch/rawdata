#include "navel/Digit.hh"

#include <iostream>
#include <numeric>
#include <iterator>
#include <cmath>

next::Digit::Digit() :
	channelID_(0),
	digType_(digitType::PE),
	chanType_(chanType::PMT),
	pedestal_(0.),
	clockTickWidth_(0.01),
	saturated_(false),
	febFailureBit_(false),
	waveform_()
{
}

next::Digit::Digit(unsigned int chID, digitType dtype, chanType chtype) :
	channelID_(chID),
	digType_(dtype),
	chanType_(chtype),
	pedestal_(0.),
	clockTickWidth_(0.01),
	saturated_(false),
	febFailureBit_(false),
	waveform_()
{
}

float next::Digit::TDC(unsigned int i) const
{
	if ( i > this->waveform_.size() ){
		std::cout << "Sample index out of range i = " << i;
	} else {
		auto mIt = waveform_.begin();
		for ( unsigned int iAdv = 0;
				iAdv < i && mIt != waveform_.end(); ++iAdv )
			++mIt;

		return (*mIt).first;
	}
}

void next::Digit::newSample_end(float tdc, float Q)
{
	//Will need some protection for if key exists
	waveform_.emplace_hint( this->waveform_.end(), tdc, Q );
}
