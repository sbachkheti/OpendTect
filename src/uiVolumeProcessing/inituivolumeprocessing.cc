/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituivolumeprocessing.cc,v 1.12 2012-05-02 11:54:03 cvskris Exp $";


#include "moddepmgr.h"
#include "uivelocitygridder.h"
#include "uivolprochorinterfiller.h"
#include "uivolproclateralsmoother.h"
#include "uivolprocbodyfiller.h"
#include "uivolprocsmoother.h"
#include "uivolprocvolreader.h"


mDefModInitFn(uiVolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProc::uiHorInterFiller::initClass();
    VolProc::uiBodyFiller::initClass();
    VolProc::uiLateralSmoother::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiVelocityGridder::initClass();
    VolProc::uiVolumeReader::initClass();
}
