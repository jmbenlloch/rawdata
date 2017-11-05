#include "navel/Trigger.hh"

#include <algorithm>

next::Trigger::Trigger() :
	trigger_(-1),
	DAQbufferSize_(-1.e9),
	preTrigger_(-1.e9),
	fecID_(-1),
	bufferPos_(-1)
{
	trigChan_.reserve( 100 );
}

next::Trigger::Trigger(int trigger) :
	trigger_(trigger),
	DAQbufferSize_(-1.e9),
	preTrigger_(-1.e9),
	fecID_(-1),
	bufferPos_(-1)
{
	trigChan_.reserve( 100 );
}

void next::Trigger::setTrigChan(const std::vector<int>& trCh)
{
	this->trigChan_.assign( trCh.begin(), trCh.end() );
}

void next::Trigger::addTrigChan(int trCh)
{
	auto trIt = std::find( this->trigChan_.begin(),
			this->trigChan_.end(), trCh );
	if ( trIt == this->trigChan_.end() )
		trigChan_.push_back( trCh );
}

void next::Trigger::getTrigChans(std::vector<int>& trCh) const
{
	trCh.assign( this->trigChan_.begin(), this->trigChan_.end() );
}

