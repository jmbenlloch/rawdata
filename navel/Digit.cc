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
	waveform_(0),
	active_(false)
{
}

next::Digit::Digit(unsigned int chID, digitType dtype, chanType chtype) :
	channelID_(chID),
	digType_(dtype),
	chanType_(chtype),
	pedestal_(0.),
	saturated_(false),
	febFailureBit_(false),
	waveform_(0),
	active_(false)
{
}
