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
	saturated_(false),
	febFailureBit_(false),
	waveform_()
{
}

void next::Digit::newSample_end(float tdc, float Q)
{
	//Will need some protection for if key exists
	waveform_.emplace_hint( this->waveform_.end(), tdc, Q );
}
