/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visnodestate.h"

#include <osg/StateSet>

using namespace visBase;


NodeState::NodeState()
{}

NodeState::~NodeState()
{
    while ( statesets_.size() )
	detatchStateSet( statesets_[0]);
}


void NodeState::attachStateSet( osg::StateSet* ns )
{
    if ( statesets_.isPresent( ns ) )
	return;
    
    statesets_ += ns;
    ns->ref();
    
    for ( int idx=0; idx<attributes_.size(); idx++ )
	ns->setAttribute( attributes_[idx] );
}


void NodeState::detatchStateSet( osg::StateSet* ns )
{
    if ( statesets_.isPresent( ns ) )
	return;
    
    statesets_ -= ns;
    
    for ( int idx=0; idx<attributes_.size(); idx++ )
	ns->setAttribute( attributes_[idx] );
    
    ns->unref();
}



void NodeState::doAdd( osg::StateAttribute* as)
{
    for ( int idx=0; idx<statesets_.size(); idx++ )
	statesets_[idx]->setAttribute( as );
    
    attributes_ += as;
}


void NodeState::doRemove( osg::StateAttribute* as)
{
    for ( int idx=0; idx<statesets_.size(); idx++ )
	statesets_[idx]->removeAttribute( as );
    
    attributes_ -= as;
}
