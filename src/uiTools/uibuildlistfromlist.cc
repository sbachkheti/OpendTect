/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: uibuildlistfromlist.cc,v 1.7 2012-05-02 11:53:56 cvskris Exp $";

#include "uibuildlistfromlist.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uilabel.h"
#include "uimsg.h"


static void chckYPlural( BufferString& str )
{
    const int len = str.size();
    if ( len < 3 ) return;
    char* ptr = str.buf() + len - 2;
    if ( *ptr == 'y' )
    {
	*ptr = '\0';
	str += "ies";
    }
}


uiBuildListFromList::Setup::Setup( bool mv, const char* avitmtp,
				   const char* defitmtp )
    : movable_(mv)
    , avitemtype_(avitmtp)
    , defitemtype_(defitmtp)
    , withio_(true)
    , singleuse_(false)
    , withtitles_(false)
{
    if ( avitemtype_.isEmpty() )
	avitemtype_ = "ingredient";
    if ( defitemtype_.isEmpty() )
	defitemtype_ = "definition";
    addtt_.add( "Add " ).add( defitemtype_ );
    edtt_.add( "Edit " ).add( defitemtype_ );
    rmtt_.add( "Remove " ).add( defitemtype_ );
    avtitle_.add( "Available " ).add( avitemtype_ ).add( "s" );
    deftitle_.add( "Defined " ).add( defitemtype_ ).add( "s" );
    chckYPlural( avtitle_ ); chckYPlural( deftitle_ );
}


uiBuildListFromList::uiBuildListFromList( uiParent* p,
			const uiBuildListFromList::Setup& su, const char* nm )
    : uiGroup(p,nm?nm:"List-from-list build group")
    , setup_(su)
    , usrchg_(false)
    , savebut_(0)
    , movedownbut_(0)
{
    avfld_ = new uiListBox( this, setup_.avtitle_ );
    avfld_->doubleClicked.notify( mCB(this,uiBuildListFromList,addCB) );
    if ( setup_.withtitles_ && !setup_.avtitle_.isEmpty() )
    {
	uiLabel* lbl = new uiLabel( this, setup_.avtitle_ );
	lbl->attach( centeredAbove, avfld_ );
    }

    uiToolButton* addbut = new uiToolButton( this, uiToolButton::RightArrow,
		    setup_.addtt_, mCB(this,uiBuildListFromList,addCB) );
    addbut->attach( centeredRightOf, avfld_ );

    deffld_ = new uiListBox( this, setup_.deftitle_ );
    deffld_->attach( rightTo, avfld_ );
    deffld_->attach( ensureRightOf, addbut );
    deffld_->selectionChanged.notify( mCB(this,uiBuildListFromList,defSelCB) );
    deffld_->doubleClicked.notify( mCB(this,uiBuildListFromList,edCB) );
    if ( setup_.withtitles_ && !setup_.deftitle_.isEmpty() )
    {
	uiLabel* lbl = new uiLabel( this, setup_.deftitle_ );
	lbl->attach( centeredAbove, deffld_ );
    }

    edbut_ = new uiToolButton( this, "edit.png",
		    setup_.edtt_, mCB(this,uiBuildListFromList,edCB) );
    edbut_->attach( rightOf, deffld_ );
    rmbut_ = new uiToolButton( this, "trashcan.png",
		    setup_.rmtt_, mCB(this,uiBuildListFromList,rmCB) );
    rmbut_->attach( alignedBelow, edbut_ );

    if ( setup_.withio_ )
    {
	uiToolButton* openbut = new uiToolButton( this, "openset.png",
				"Open stored set",
				mCB(this,uiBuildListFromList,openCB) );
	openbut->attach( alignedBelow, rmbut_ );
	savebut_ = new uiToolButton( this, "save.png", "Save set",
		mCB(this,uiBuildListFromList,saveCB) );
	savebut_->attach( alignedBelow, openbut );
    }

    if ( setup_.movable_ )
    {
	moveupbut_ = new uiToolButton( this, uiToolButton::UpArrow,
			"Move up", mCB(this,uiBuildListFromList,moveCB) );
	moveupbut_->attach( alignedBelow, savebut_ ? savebut_ : rmbut_ );
	movedownbut_ = new uiToolButton( this, uiToolButton::DownArrow,
			"Move down", mCB(this,uiBuildListFromList,moveCB) );
	movedownbut_->attach( alignedBelow, moveupbut_ );
    }

    setHAlignObj( deffld_ );
    defSelChg();
}


void uiBuildListFromList::setAvailable( const BufferStringSet& avnms )
{
    avfld_->setEmpty();
    avfld_->addItems( avnms );
}


void uiBuildListFromList::defSelChg()
{
    const int selidx = deffld_->currentItem();
    if ( !setup_.singleuse_ && selidx >= 0 )
	avfld_->setCurrentItem( avFromDef(deffld_->getText()) );

    const bool havesel = selidx >= 0;
    edbut_->setSensitive( havesel );
    rmbut_->setSensitive( havesel );
    if ( savebut_ )
	savebut_->setSensitive( havesel );
    if ( movedownbut_ )
    {
	const int sz = deffld_->size();
	const bool canmove = sz > 1;
	moveupbut_->setSensitive( canmove && selidx > 0 );
	movedownbut_->setSensitive( canmove && selidx < sz - 1 );
    }
}


void uiBuildListFromList::rmItm( int itmidx, bool dosignals )
{
    if ( itmidx < 0 || itmidx >= deffld_->size() )
	return;
    if ( setup_.singleuse_ )
	avfld_->insertItem( avFromDef(deffld_->textOfItem(itmidx)), 0 );

    deffld_->removeItem( itmidx );
    usrchg_ = true;

    if ( !dosignals )
	return;

    if ( itmidx >= deffld_->size() )
	itmidx = deffld_->size() - 1;
    if ( itmidx >= 0 )
	deffld_->setCurrentItem( itmidx );

    defSelChg(); // explicit because the UI selection hasn't changed
}


void uiBuildListFromList::removeItem()
{
    rmItm( deffld_->currentItem(), true );
}

void uiBuildListFromList::removeAll()
{
    while ( true )
    {
	const int sz = deffld_->size();
	const bool islast = sz < 2;
	rmItm( 0, islast );
	if ( islast )
	    break;
    }
}


void uiBuildListFromList::setItemName( const char* newnm )
{
    const int itmidx = deffld_->currentItem();
    if ( itmidx < 0 ) return;
    const BufferString orgnm( deffld_->textOfItem(itmidx) );
    if ( orgnm != newnm )
    {
	deffld_->setItemText( itmidx, newnm );
	usrchg_ = true;
    }
}


void uiBuildListFromList::addItem( const char* itmnm )
{
    deffld_->addItem( itmnm );
    const int itmidx = deffld_->size() - 1;
    if ( setup_.singleuse_ )
    {
	const int avidx = avfld_->indexOf( avFromDef(itmnm) );
	avfld_->removeItem( avidx );
    }
    deffld_->setCurrentItem( itmidx );
    usrchg_ = true;
}


void uiBuildListFromList::openCB( CallBacker* )
{
    if ( usrchg_ && !uiMSG().askGoOn("Current work not saved. Continue?") )
	return;

    if ( ioReq(false) )
	usrchg_ = false;
}


void uiBuildListFromList::saveCB( CallBacker* )
{
    if ( ioReq(true) )
	usrchg_ = false;
}


void uiBuildListFromList::moveCB( CallBacker* cb )
{
    const int sz = deffld_->size();
    if ( sz < 2 ) return;

    const int fromidx = deffld_->currentItem();
    const int toidx = cb == movedownbut_ ? fromidx + 1 : fromidx - 1;
    if ( toidx < 0 || toidx >= sz ) return;

    const BufferString fromtxt( deffld_->textOfItem(fromidx) );
    const BufferString totxt( deffld_->textOfItem(toidx) );
    deffld_->setItemText( fromidx, totxt );
    deffld_->setItemText( toidx, fromtxt );

    itemSwitch( fromtxt, totxt );
}



uiToolButton* uiBuildListFromList::lowestStdBut()
{
    return movedownbut_ ? movedownbut_ : (savebut_ ? savebut_ : rmbut_);
}


const char* uiBuildListFromList::curAvSel() const
{
    const int itmidx = avfld_->currentItem();
    return itmidx < 0 ? 0 : avfld_->textOfItem(itmidx);
}


const char* uiBuildListFromList::curDefSel() const
{
    const int itmidx = deffld_->currentItem();
    return itmidx < 0 ? 0 : deffld_->textOfItem(itmidx);
}


void uiBuildListFromList::setCurDefSel( const char* nm )
{
    deffld_->setCurrentItem( nm );
}
