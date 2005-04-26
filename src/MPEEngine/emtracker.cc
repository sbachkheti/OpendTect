/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emtracker.cc,v 1.9 2005-04-26 20:38:01 cvsnanne Exp $";

#include "emtracker.h"

#include "emhistory.h"
#include "emmanager.h"
#include "emobject.h"
#include "sectionextender.h"
#include "sectionselector.h"
#include "sectiontracker.h"
#include "consistencychecker.h"
#include "trackplane.h"
#include "iopar.h"

namespace MPE 
{

EMTracker::EMTracker( EM::EMObject* emo )
    : isenabled (true)
    , emobject(emo)
{}


EMTracker::~EMTracker()
{
    deepErase( sectiontrackers );
}


BufferString EMTracker::objectName() const
{ return emobject ? emobject->name() : 0; }



EM::ObjectID EMTracker::objectID() const
{ return emobject ? emobject->id() : -1; }


bool EMTracker::trackSections( const TrackPlane& plane )
{
    if ( !emobject || !isenabled ) return true;

    const int initialhistnr = EM::EMM().history().currentEventNr();
    ConsistencyChecker* consistencychecker = getConsistencyChecker();
    if ( consistencychecker ) consistencychecker->reset();

    for ( int idx=0; idx<emobject->nrSections(); idx++ )
    {
	const EM::SectionID sectionid = emobject->sectionID(idx);
	SectionTracker* sectiontracker = getSectionTracker(sectionid,true);
	if ( !sectiontracker )
	    continue;

	sectiontracker->reset();
	sectiontracker->selector()->setTrackPlane( plane );

	if ( plane.getTrackMode() == TrackPlane::Erase )
	{
	    erasePositions( sectionid,
		    	    sectiontracker->selector()->selectedPositions() );
	    continue;
	}
	
	sectiontracker->extender()->setDirection( plane.motion() );
	sectiontracker->select();

	const bool hasextended = sectiontracker->extend();
	bool hasadjusted = true;
	if ( hasextended && sectiontracker->adjusterUsed() )
	    hasadjusted = sectiontracker->adjust();

	if ( !hasextended || !hasadjusted )
	{
	    while ( EM::EMM().history().canUnDo() &&
		    EM::EMM().history().currentEventNr()!=initialhistnr )
	    {
		bool res = EM::EMM().history().unDo(1);
		if ( !res ) break;
	    }

	    EM::EMM().history().setCurrentEventAsLast();

	    errmsg = sectiontracker->errMsg();
	    return false;;
	}

	EM::PosID posid( emobject->id(), sectionid );
	if ( consistencychecker )
	{
	    const TypeSet<EM::SubID>& addedpos = 
		sectiontracker->extender()->getAddedPositions();
	    for ( int posidx=0; posidx<addedpos.size(); posidx++ )
	    {
		posid.setSubID( addedpos[posidx] );
		consistencychecker->addNodeToCheck( posid );
	    }
	}
    }

    if ( consistencychecker )
	consistencychecker->nextStep();

    return true;
}


bool EMTracker::trackIntersections( const TrackPlane& )
{ return true; }


const char* EMTracker::errMsg() const
{ return errmsg[0] ? (const char*) errmsg : 0; }


SectionTracker* EMTracker::getSectionTracker( EM::SectionID sid, bool create )
{
    for ( int idx=0; idx<sectiontrackers.size(); idx++ )
    {
	if ( sectiontrackers[idx]->sectionID()==sid )
	    return sectiontrackers[idx];
    }

    if ( !create ) return 0;

    SectionTracker* sectiontracker = createSectionTracker( sid );
    if ( !sectiontracker || !sectiontracker->init() )
    {
	delete sectiontracker;
	return 0;
    }

    if ( sectiontrackers.size() )
    {
	IOPar par;
	sectiontrackers[0]->fillPar( par );
	sectiontracker->usePar( par );
    }

    sectiontrackers += sectiontracker;
    return sectiontracker;
}


void EMTracker::erasePositions( EM::SectionID sectionid,
				const TypeSet<EM::SubID>& pos )
{
    EM::PosID posid( emobject->id(), sectionid );
    for ( int idx=0; idx<pos.size(); idx++ )
    {
	posid.setSubID( pos[idx] );
	emobject->unSetPos( posid, true );
    }
}


// TrackerFactory +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TrackerFactory::TrackerFactory( const char* emtype, EMTrackerCreationFunc func )
    : type( emtype )
    , createfunc( func )
{}


const char* TrackerFactory::emObjectType() const
{ return type; } 


EMTracker* TrackerFactory::create( EM::EMObject* emobj ) const
{ return createfunc( emobj ); }


}; // namespace MPE
