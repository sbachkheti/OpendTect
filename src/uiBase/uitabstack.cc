/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/01/2002
________________________________________________________________________

-*/

#include "uitabstack.h"
#include "uitabbar.h"
#include "uiobjbody.h"
#include "objectset.h"

#include <QFrame>

mUseQtnamespace

uiTabStack::uiTabStack( uiParent* parnt, const char* nm, bool mnge )
    : uiGroup( parnt, nm, mnge )
    , tabbar_( 0 )
    , tabgrp_( 0 )
{
    // Don't change the order of these constuctions!
    tabgrp_ = new uiGroup( this, nm );
    tabbar_ = new uiTabBar( this, nm );

    tabgrp_->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
    tabgrp_->setBorder(10);
    tabgrp_->attach( stretchedBelow, tabbar_, 0 );

    tabbar_->selected.notify( mCB(this,uiTabStack,tabSel) );
    tabbar_->tabToBeClosed.notify( mCB(this,uiTabStack,tabCloseCB) );
}


NotifierAccess& uiTabStack::selChange()
{ return tabbar_->selected; }


void uiTabStack::tabSel( CallBacker* cb )
{
    int id = tabbar_->currentTabId();
    uiGroup* selgrp = page( id );
    ObjectSet<uiTab>& tabs = tabbar_->tabs_;

    for ( int idx=0; idx<tabs.size(); idx++ )
    {
	uiGroup* grp = &tabs[idx]->group();
	const bool disp = grp == selgrp;
	grp->display( disp );
    }
}


void uiTabStack::addTab( uiGroup* grp, const uiString& txt )
{
    if ( !grp ) return;

    const uiString tabcaption = !txt.isEmpty() ? txt
						: toUiString(grp->name());
    uiTab* tab = new uiTab( *grp, tabcaption );
    tabbar_->addTab( tab );

    if ( !hAlignObj() )
	setHAlignObj( grp );
}


int uiTabStack::insertTab( uiGroup* grp, int index, const uiString& txt )
{
    if ( !grp ) return -1;

    const uiString tabcaption = !txt.isEmpty() ? txt
						: toUiString(grp->name());
    uiTab* tab = new uiTab( *grp, tabcaption );
    if ( !hAlignObj() )
	setHAlignObj( grp );

    return tabbar_->insertTab( tab, index );
}


void uiTabStack::removeTab( uiGroup* grp )
{ tabbar_->removeTab( grp ); }


void uiTabStack::setTabEnabled( uiGroup* grp, bool yn )
{
    int id = indexOf( grp );
    tabbar_->setTabEnabled( id, yn );
}


bool uiTabStack::isTabEnabled( uiGroup* grp ) const
{
    int id = indexOf( grp );
    return tabbar_->isTabEnabled( id );
}


int uiTabStack::indexOf( uiGroup* grp ) const
{ return tabbar_->indexOf( grp ); }

int uiTabStack::size() const
{ return tabbar_->size(); }


void uiTabStack::setCurrentPage( int id )
{
    tabbar_->setCurrentTab( id );
    tabSel(0);
}


void uiTabStack::setCurrentPage( uiGroup* grp )
{
    if( !grp ) return;
    setCurrentPage( indexOf(grp) );
}


void uiTabStack::setCurrentPage( const char* grpnm )
{
    for ( int idx=0; grpnm && idx<size(); idx++ )
    {
	if ( FixedString(page(idx)->name()) == grpnm )
	{
	    setCurrentPage( idx );
	    return;
	}
    }
}


void uiTabStack::setTabIcon( int idx, const char* icnnm )
{
    tabbar_->setTabIcon( idx, icnnm );
}


void uiTabStack::setTabsClosable( bool closable )
{
    tabbar_->setTabsClosable( closable );
}


void uiTabStack::tabCloseCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,tabid,cb);
    uiGroup* tabgrp = tabbar_->page( tabid );
    removeTab( tabgrp );
}


void uiTabStack::setTabIcon( uiGroup* grp, const char* icnnm )
{
    const int idx = indexOf( grp );
    setTabIcon( idx, icnnm );
}


uiGroup* uiTabStack::currentPage() const
{ return page( currentPageId() ); }

uiGroup* uiTabStack::page( int id ) const
{ return tabbar_->page( id ); }

int uiTabStack::currentPageId() const
{ return tabbar_->currentTabId(); }
