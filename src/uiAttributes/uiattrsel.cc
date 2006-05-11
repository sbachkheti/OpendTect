/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uiattrsel.cc,v 1.12 2006-05-11 20:02:51 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrsel.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "hilbertattrib.h"
#include "attribstorprovider.h"

#include "uimsg.h"
#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "ptrman.h"
#include "seistrctr.h"
#include "linekey.h"
#include "cubesampling.h"
#include "survinfo.h"

#include "nlamodel.h"
#include "nladesign.h"

#include "uilistbox.h"
#include "uigeninput.h"
#include "datainpspec.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uicombobox.h"

using namespace Attrib;


uiAttrSelDlg::uiAttrSelDlg( uiParent* p, const char* seltxt,
			    const uiAttrSelData& atd, 
			    Pol2D pol2d, DescID ignoreid )
	: uiDialog(p,Setup("",0,atd.nlamodel?"":"101.1.1"))
	, attrdata_(atd)
	, selgrp_(0)
	, storfld_(0)
	, attrfld_(0)
	, attr2dfld_(0)
	, nlafld_(0)
	, nlaoutfld_(0)
	, in_action_(false)
{
    attrinf_ = new SelInfo( atd.attrset, atd.nlamodel, pol2d, ignoreid );
    if ( !attrinf_->ioobjnms.size() )
    {
	new uiLabel( this, "No seismic data available.\n"
			   "Please import data first" );
	setCancelText( "Ok" );
	setOkText( "" );
	return;
    }

    const bool havenlaouts = attrinf_->nlaoutnms.size();
    const bool haveattribs = attrinf_->attrnms.size();

    BufferString nm( "Select " ); nm += seltxt;
    setName( nm );
    setCaption( "Select" );
    setTitleText( nm );

    selgrp_ = new uiGroup( this, "Input selection" );

    srcfld_ = new uiRadioButton( selgrp_, "Stored" );
    srcfld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );
    srcfld_->setSensitive( attrdata_.shwcubes );

    calcfld_ = new uiRadioButton( selgrp_, "Attributes" );
    calcfld_->attach( alignedBelow, srcfld_ );
    calcfld_->setSensitive( haveattribs );
    calcfld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );

    if ( havenlaouts )
    {
	nlafld_ = new uiRadioButton( selgrp_, attrdata_.nlamodel->nlaType(false) );
	nlafld_->attach( alignedBelow, calcfld_ );
	nlafld_->setSensitive( havenlaouts );
	nlafld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );
    }

    if ( attrdata_.shwcubes )
    {
	storfld_ = new uiListBox( this, attrinf_->ioobjnms );
	storfld_->setHSzPol( uiObject::Wide );
	storfld_->selectionChanged.notify( mCB(this,uiAttrSelDlg,cubeSel) );
	storfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	storfld_->attach( rightOf, selgrp_ );
	attr2dfld_ = new uiGenInput( this, "Stored Attribute",
				    StringListInpSpec() );
	attr2dfld_->attach( alignedBelow, storfld_ );
	filtfld_ = new uiGenInput( this, "Filter", "*" );
	filtfld_->attach( alignedBelow, storfld_ );
	filtfld_->valuechanged.notify( mCB(this,uiAttrSelDlg,filtChg) );
    }

    if ( haveattribs )
    {
	attrfld_ = new uiListBox( this, attrinf_->attrnms );
	attrfld_->setHSzPol( uiObject::Wide );
	attrfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	attrfld_->attach( rightOf, selgrp_ );
    }

    if ( havenlaouts )
    {
	nlaoutfld_ = new uiListBox( this, attrinf_->nlaoutnms );
	nlaoutfld_->setHSzPol( uiObject::Wide );
	nlaoutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	nlaoutfld_->attach( rightOf, selgrp_ );
    }

    int seltyp = havenlaouts ? 2 : (haveattribs ? 1 : 0);
    int storcur = -1, attrcur = -1, nlacur = -1;
    if ( attrdata_.nlamodel && attrdata_.outputnr >= 0 )
    {
	seltyp = 2;
	nlacur = attrdata_.outputnr;
    }
    else
    {
	const Desc* desc = attrdata_.attribid < 0 ? 0 :
	    		attrdata_.attrset->getDesc( attrdata_.attribid );
	if ( desc )
	{
	    seltyp = desc->isStored() ? 0 : 1;
	    if ( seltyp == 1 )
		attrcur = attrinf_->attrnms.indexOf( desc->userRef() );
	    else if ( storfld_ )
		storcur = attrinf_->ioobjnms.indexOf( desc->userRef() );
	}
	else
	{
	    // Defaults are the last ones added to attrib set
	    for ( int idx=attrdata_.attrset->nrDescs()-1; idx!=-1; idx-- )
	    {
		const DescID attrid = attrdata_.attrset->getID( idx );
		const Desc& ad = *attrdata_.attrset->getDesc( attrid );
		if ( ad.isStored() && storcur == -1 )
		    storcur = attrinf_->ioobjnms.indexOf( ad.userRef() );
		else if ( !ad.isStored() && attrcur == -1 )
		    attrcur = attrinf_->attrnms.indexOf( ad.userRef() );
		if ( storcur != -1 && attrcur != -1 ) break;
	    }
	}
    }

    if ( storcur == -1 )		storcur = 0;
    if ( attrcur == -1 )		attrcur = attrinf_->attrnms.size()-1;
    if ( nlacur == -1 && havenlaouts )	nlacur = 0;

    if ( storfld_  )			storfld_->setCurrentItem( storcur );
    if ( attrfld_ && attrcur != -1 )	attrfld_->setCurrentItem( attrcur );
    if ( nlaoutfld_ && nlacur != -1 )	nlaoutfld_->setCurrentItem( nlacur );

    if ( seltyp == 0 )		srcfld_->setChecked(true);
    else if ( seltyp == 1 )	calcfld_->setChecked(true);
    else if ( nlafld_ )		nlafld_->setChecked(true);

    finaliseStart.notify( mCB( this,uiAttrSelDlg,doFinalise) );
}


uiAttrSelDlg::~uiAttrSelDlg()
{
    delete attrinf_;
}


void uiAttrSelDlg::doFinalise( CallBacker* )
{
    selDone(0);
    in_action_ = true;
}


int uiAttrSelDlg::selType() const
{
    if ( calcfld_->isChecked() )
	return 1;
    if ( nlafld_ && nlafld_->isChecked() )
	return 2;
    return 0;
}


void uiAttrSelDlg::selDone( CallBacker* c )
{
    if ( !selgrp_ ) return;

    mDynamicCastGet(uiRadioButton*,but,c);
   
    bool dosrc, docalc, donla; 
    if ( but == srcfld_ )
    { dosrc = true; docalc = donla = false; }
    else if ( but == calcfld_ )
    { docalc = true; dosrc = donla = false; }
    else if ( but == nlafld_ )
    { donla = true; docalc = dosrc = false; }

    if ( but )
    {
	srcfld_->setChecked( dosrc );
	calcfld_->setChecked( docalc );
	if ( nlafld_ ) nlafld_->setChecked( donla );
    }

    const int seltyp = selType();
    if ( attrfld_ ) attrfld_->display( seltyp == 1 );
    if ( nlaoutfld_ ) nlaoutfld_->display( seltyp == 2 );
    if ( storfld_ )
    {
	storfld_->display( seltyp == 0 );
	filtfld_->display( seltyp == 0 );
    }

    cubeSel(0);
}


void uiAttrSelDlg::filtChg( CallBacker* c )
{
    if ( !storfld_ || !filtfld_ ) return;

    attrinf_->fillStored( filtfld_->text() );
    storfld_->empty();
    storfld_->addItems( attrinf_->ioobjnms );
    if ( attrinf_->ioobjnms.size() )
	storfld_->setCurrentItem( 0 );

    cubeSel( c );
}


void uiAttrSelDlg::cubeSel( CallBacker* c )
{
    const int seltyp = selType();
    if ( seltyp )
    {
	attr2dfld_->display( false );
	return;
    }

    int selidx = storfld_->currentItem();
    bool is2d_ = false;
    BufferString ioobjkey;
    if ( selidx >= 0 )
    {
	ioobjkey = attrinf_->ioobjids.get( storfld_->currentItem() );
	is2d_ = SelInfo::is2D( ioobjkey.buf() );
    }
    attr2dfld_->display( is2d_ );
    filtfld_->display( !is2d_ );
    if ( is2d_ )
    {
	BufferStringSet nms;
	SelInfo::getAttrNames( ioobjkey.buf(), nms );
	attr2dfld_->newSpec( StringListInpSpec(nms), 0 );
    }
}


bool uiAttrSelDlg::getAttrData( bool needattrmatch )
{
    attrdata_.attribid = DescID::undef();
    attrdata_.outputnr = -1;
    if ( !selgrp_ || !in_action_ ) return true;

    const int seltyp = selType();
    int selidx = (seltyp ? (seltyp == 2 ? nlaoutfld_ : attrfld_) : storfld_)
		    ->currentItem();
    if ( selidx < 0 )
	return false;

    if ( seltyp == 1 )
	attrdata_.attribid = attrinf_->attrids[selidx];
    else if ( seltyp == 2 )
	attrdata_.outputnr = selidx;
    else
    {
	DescSet& as = const_cast<DescSet&>( *attrdata_.attrset );

	const char* ioobjkey = attrinf_->ioobjids.get(selidx);
	LineKey linekey( ioobjkey );
	if ( SelInfo::is2D(ioobjkey) )
	{
	    attrdata_.outputnr = attr2dfld_->getIntValue();
	    BufferStringSet nms;
	    SelInfo::getAttrNames( ioobjkey, nms );
	    const char* attrnm = attrdata_.outputnr >= nms.size() ? 0
				    : nms.get(attrdata_.outputnr).buf();
	    if ( needattrmatch )
		linekey.setAttrName( attrnm );
	}

	attrdata_.attribid = as.getStoredID( linekey, 0, true );
	if ( needattrmatch && attrdata_.attribid < 0 )
	{
	    BufferString msg( "Could not find the seismic data " );
	    msg += attrdata_.attribid == DescID::undef() ? "in object manager"
							: "on disk";	
	    uiMSG().error( msg );
	    return false;
	}
    }

    return true;
}


bool uiAttrSelDlg::acceptOK( CallBacker* )
{
    return getAttrData(true);
}


uiAttrSel::uiAttrSel( uiParent* p, const DescSet* ads,
		      const char* txt, DescID curid )
    : uiIOSelect(p,mCB(this,uiAttrSel,doSel),txt?txt:"Input Data")
    , attrdata_(ads)
    , ignoreid(DescID::undef())
    , is2d_(false)
{
    attrdata_.attribid = curid;
    updateInput();
}


uiAttrSel::uiAttrSel( uiParent* p, const char* txt, const uiAttrSelData& ad )
    : uiIOSelect(p,mCB(this,uiAttrSel,doSel),txt?txt:"Input Data")
    , attrdata_(ad)
    , ignoreid(DescID::undef())
    , is2d_(false)
{
    updateInput();
}


void uiAttrSel::setDescSet( const DescSet* ads )
{
    attrdata_.attrset = ads;
    update2D();
}


void uiAttrSel::setNLAModel( const NLAModel* mdl )
{
    attrdata_.nlamodel = mdl;
}


void uiAttrSel::setDesc( const Desc* ad )
{
    attrdata_.attrset = ad ? ad->descSet() : 0;
    if ( !ad ) return;

    const char* inp = ad->userRef();
    if ( inp[0] == '_' || (ad->isStored() && ad->dataType() == Seis::Dip) )
	return;

    attrdata_.attribid = !attrdata_.attrset ? DescID::undef() : ad->id();
    updateInput();
    update2D();
}


void uiAttrSel::setIgnoreDesc( const Desc* ad )
{
    ignoreid = DescID::undef();
    if ( !ad ) return;
    if ( !attrdata_.attrset ) attrdata_.attrset = ad->descSet();
    ignoreid = ad->id();
}


void uiAttrSel::updateInput()
{
    BufferString bs;
    bs = attrdata_.attribid.asInt();
    bs += ":";
    bs += attrdata_.outputnr;
    setInput( bs );
}


const char* uiAttrSel::userNameFromKey( const char* txt ) const
{
    if ( !attrdata_.attrset || !txt || !*txt ) return "";

    BufferString buf( txt );
    char* outnrstr = strchr( buf.buf(), ':' );
    if ( outnrstr ) *outnrstr++ = '\0';
    const DescID attrid( atoi(buf.buf()), true );
    const int outnr = outnrstr ? atoi( outnrstr ) : 0;
    if ( attrid < 0 )
    {
	if ( !attrdata_.nlamodel || outnr < 0 )
	    return "";
	if ( outnr >= attrdata_.nlamodel->design().outputs.size() )
	    return "<error>";

	const char* nm = attrdata_.nlamodel->design().outputs[outnr]->buf();
	return IOObj::isKey(nm) ? IOM().nameOf(nm) : nm;
    }

    const Desc* ad = attrdata_.attrset->getDesc( attrid );
    usrnm = ad ? ad->userRef() : "";
    return usrnm.buf();
}


void uiAttrSel::getHistory( const IOPar& iopar )
{
    uiIOSelect::getHistory( iopar );
    updateInput();
}


bool uiAttrSel::getRanges( CubeSampling& cs ) const
{
    if ( attrdata_.attribid < 0 || !attrdata_.attrset )
	return false;

    const Desc* desc = attrdata_.attrset->getDesc( attrdata_.attribid );
    if ( !desc->isStored() ) return false;

    const ValParam* keypar = 
		(ValParam*)desc->getParam( StorageProvider::keyStr() );
    const MultiID mid( keypar->getStringValue() );
    return SeisTrcTranslator::getRanges( mid, cs,
					 desc->is2D() ? getInput() : 0 );
}


void uiAttrSel::doSel( CallBacker* )
{
    if ( !attrdata_.attrset ) return;

    uiAttrSelDlg dlg( this, inp_->label()->text(), attrdata_, 
	    	      is2d_ ? Only2D : Both2DAnd3D, ignoreid );
    if ( dlg.go() )
    {
	attrdata_.attribid = dlg.attribID();
	attrdata_.outputnr = dlg.outputNr();
	updateInput();
	update2D();
	selok_ = true;
    }
}


void uiAttrSel::processInput()
{
    if ( !attrdata_.attrset ) return;

    BufferString inp = getInput();
    char* attr2d = strchr( inp, '|' );
    DescSet& as = const_cast<DescSet&>( *attrdata_.attrset );
    attrdata_.attribid = as.getID( inp, true );
    attrdata_.outputnr = -1;
    if ( attrdata_.attribid >= 0 && attr2d )
    {
	const Desc& ad = *attrdata_.attrset->getDesc(attrdata_.attribid);
	BufferString defstr; ad.getDefStr( defstr );
	BufferStringSet nms;
	SelInfo::getAttrNames( defstr, nms );
	attrdata_.outputnr = nms.indexOf( attr2d );
    }
    else if ( attrdata_.attribid < 0 && attrdata_.nlamodel )
    {
	const BufferStringSet& outnms( attrdata_.nlamodel->design().outputs );
	const BufferString nodenm = IOObj::isKey(inp) ? IOM().nameOf(inp)
	    						: inp.buf();
	for ( int idx=0; idx<outnms.size(); idx++ )
	{
	    const BufferString& outstr = *outnms[idx];
	    const char* desnm = IOObj::isKey(outstr) ? IOM().nameOf(outstr)
						     : outstr.buf();
	    if ( nodenm == desnm )
		{ attrdata_.outputnr = idx; break; }
	}
    }

    updateInput();
}


void uiAttrSel::fillSelSpec( SelSpec& as ) const
{
    const bool isnla = attrdata_.attribid < 0 && attrdata_.outputnr >= 0;
    if ( isnla )
	as.set( 0, DescID(attrdata_.outputnr,true), true, "" );
    else
	as.set( 0, attrdata_.attribid, false, "" );

    if ( isnla && attrdata_.nlamodel )
	as.setRefFromID( *attrdata_.nlamodel );
    else
	as.setRefFromID( *attrdata_.attrset );
}


const char* uiAttrSel::getAttrName() const
{
    static BufferString ret;

    ret = getInput();
    if ( is2d_ )
    {
	const Desc* ad = attrdata_.attrset->getDesc( attrdata_.attribid );
	if ( (ad && ad->isStored()) || strchr(ret.buf(),'|') )
	    ret = LineKey( ret ).attrName();
    }

    return ret.buf();
}


bool uiAttrSel::checkOutput( const IOObj& ioobj ) const
{
    if ( attrdata_.attribid < 0 && attrdata_.outputnr < 0 )
    {
	uiMSG().error( "Please select the input" );
	return false;
    }

    if ( is2d_ && !SeisTrcTranslator::is2D(ioobj) )
    {
	uiMSG().error( "Can only store this in a 2D line set" );
	return false;
    }

    //TODO this is pretty difficult to get right
    if ( attrdata_.attribid < 0 )
	return true;

    const Desc& ad = *attrdata_.attrset->getDesc( attrdata_.attribid );
    bool isdep = false;
/*
    if ( !is2d_ )
	isdep = ad.isDependentOn(ioobj,0);
    else
    {
	// .. and this too
	if ( ad.isStored() )
	{
	    LineKey lk( ad.defStr() );
	    isdep = ioobj.key() == lk.lineName();
	}
    }
    if ( isdep )
    {
	uiMSG().error( "Cannot output to an input" );
	return false;
    }
*/

    return true;
}


void uiAttrSel::update2D()
{
    processInput();
    if ( attrdata_.attrset && attrdata_.attrset->nrDescs() > 0 )
	is2d_ = attrdata_.attrset->is2D();
}


// **** uiImagAttrSel ****

DescID uiImagAttrSel::imagID() const
{
    if ( !attrdata_.attrset )
    {
	pErrMsg( "No attribdescset set");
	return DescID::undef();
    }

    const DescID selattrid = attribID();
    TypeSet<DescID> attribids;
    attrdata_.attrset->getIds( attribids );
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	const Desc* desc = attrdata_.attrset->getDesc( attribids[idx] );

	if ( strcmp(desc->attribName(),Hilbert::attribName()) )
	    continue;

	const Desc* inputdesc = desc->getInput( 0 );
	if ( !inputdesc || inputdesc->id() != selattrid )
	    continue;

	return attribids[idx];
    }

    DescSet* descset = const_cast<DescSet*>(attrdata_.attrset);

    Desc* inpdesc = descset->getDesc( selattrid );
    Desc* newdesc = PF().createDescCopy( Hilbert::attribName() );
    if ( !newdesc || !inpdesc ) return DescID::undef();

    newdesc->selectOutput( 0 );
    newdesc->setInput( 0, inpdesc );
    newdesc->setHidden( true );

    BufferString usrref = "_"; usrref += inpdesc->userRef(); usrref += "_imag";
    newdesc->setUserRef( usrref );

    return descset->addDesc( newdesc );
}
