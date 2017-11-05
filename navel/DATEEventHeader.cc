// -----------------------------------------------------------------------------
//  $Id$
//
//  Author: <sediaz@ific.uv.es>
//  Created: 2014
//
//  Copyright (c) 2010-2014 NEXT Collaboration. All rights reserved.
// -----------------------------------------------------------------------------

#include "navel/DATEEventHeader.hh"

next::DATEEventHeader::DATEEventHeader():
	fEventSize(0),
	fEventMagic(0),
	fEventHeadSize(0),
	fEventVersion(0),
	fEventType(0),
	fRunNb(0),
	fNbInRun(0),
	fBurstNb(0),
	fNbInBurst(0)
{
}

next::DATEEventHeader::~DATEEventHeader()
{
}
