/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2008
-*/

static const char* mUnusedVar rcsID = "$Id: mousecursor.cc,v 1.6 2012-05-02 11:53:11 cvskris Exp $";

#include "mousecursor.h"


bool MouseCursor::operator==( const MouseCursor& mc ) const
{
    if ( mc.shape_!=shape_ )
	return false;

    if ( mc.shape_!=Bitmap )
	return true;

    return mc.filename_==filename_ && mc.hotx_==hotx_ && mc.hoty_==hoty_;
}


bool MouseCursor::operator!=( const MouseCursor& mc ) const
{ return !(*this==mc); }


MouseCursorManager* MouseCursorManager::mgr_ = 0;


void MouseCursorManager::setOverride( MouseCursor::Shape s, bool replace )
{
    MouseCursorManager* mgr = MouseCursorManager::mgr();
    if ( !mgr ) return;
    mgr->setOverrideShape( s, replace );
}


void MouseCursorManager::setOverride( const MouseCursor& mc, bool replace )
{
    MouseCursorManager* mgr = MouseCursorManager::mgr();
    if ( !mgr ) return;
    mgr->setOverrideCursor( mc, replace );
}


void MouseCursorManager::setOverride( const char* fnm, int hotx, int hoty,
				      bool replace )
{
    MouseCursorManager* mgr = MouseCursorManager::mgr();
    if ( !mgr ) return;
    mgr->setOverrideFile( fnm, hotx, hoty, replace );
}


void MouseCursorManager::restoreOverride()
{
    MouseCursorManager* mgr = MouseCursorManager::mgr();
    if ( !mgr ) return;

    mgr->restoreInternal();
}


MouseCursorManager* MouseCursorManager::mgr()
{ return MouseCursorManager::mgr_; }


void MouseCursorManager::setMgr( MouseCursorManager* mgr )
{
    MouseCursorManager::mgr_ = mgr;
}


MouseCursorChanger::MouseCursorChanger(const char* fnm, int hotx, int hoty)
    : active_( true )
{
    MouseCursorManager::setOverride(fnm,hotx,hoty);
}


MouseCursorChanger::MouseCursorChanger(MouseCursor::Shape cs)
    : active_( true )
{ MouseCursorManager::setOverride(cs); }


MouseCursorChanger::~MouseCursorChanger()
{ restore(); }


void MouseCursorChanger::restore()
{
    if ( active_ ) MouseCursorManager::restoreOverride();
    active_ = false;
}
