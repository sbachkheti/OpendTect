/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:		$Id: uiseismmproc.cc,v 1.26 2002-08-02 10:27:23 bert Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "seismmjobman.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uiprogressbar.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uiseparator.h"
#include "uifilebrowser.h"
#include "uiiosel.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uislider.h"
#include "uigeninput.h"
#include "hostdata.h"
#include "iopar.h"
#include "timefun.h"
#include "filegen.h"
#include <stdlib.h>


uiSeisMMProc::uiSeisMMProc( uiParent* p, const char* prognm, const IOPar& iop )
	: uiExecutor(p,getFirstJM(prognm,iop),true)
    	, running(false)
    	, finished(false)
    	, jmfinished(false)
	, logvwer(0)
{
    setCancelText( "Abort" );
    setOkText( "Finish Now" );
    delay = 500;

    const char* res = iop.find( "Target value" );
    BufferString txt( "Manage processing" );
    if ( res && *res )
	{ txt += ": "; txt += res; }
    setTitleText( txt );

    tmpstordirfld = new uiIOFileSelect( this, "Temporary storage directory",
	    				false, jm->tempStorageDir() );
    tmpstordirfld->usePar( uiIOFileSelect::tmpstoragehistory );
    tmpstordirfld->selectDirectory( true );

    uiSeparator* sep = new uiSeparator( this, "Hor sep 1", true );
    sep->attach( stretchedBelow, tmpstordirfld );

    machgrp = new uiGroup( this, "Machine handling" );

    HostDataList hdl;
    rshcomm = hdl.rshComm();
    avmachfld = new uiLabeledListBox( machgrp, "Available hosts", true,
				      uiLabeledListBox::AboveMid );
    for ( int idx=0; idx<hdl.size(); idx++ )
    {
	const HostData& hd = *hdl[idx];
	BufferString nm( hd.name() );
	const int nraliases = hd.nrAliases();
	for ( int idx=0; idx<nraliases; idx++ )
	    { nm += " / "; nm += hd.alias(idx); }
	avmachfld->box()->addItem( nm );
    }

    addbut = new uiPushButton( machgrp, ">> Add >>" );
    addbut->activated.notify( mCB(this,uiSeisMMProc,addPush) );
    addbut->attach( rightOf, avmachfld );

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Machine handling" );
    usedmachfld = new uiLabeledListBox( usedmachgrp, "Used hosts", false,
				        uiLabeledListBox::AboveMid );
    stopbut = new uiPushButton( usedmachgrp, "Stop" );
    stopbut->activated.notify( mCB(this,uiSeisMMProc,stopPush) );
    stopbut->attach( alignedBelow, usedmachfld );
    vwlogbut = new uiPushButton( usedmachgrp, "View log" );
    vwlogbut->activated.notify( mCB(this,uiSeisMMProc,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld );

    usedmachgrp->attach( rightOf, addbut );
    machgrp->setHAlignObj( addbut );
    machgrp->attach( alignedBelow, tmpstordirfld );
    machgrp->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "Hor sep 2", true );
    sep->attach( stretchedBelow, machgrp );
    uiLabel* lbl = new uiLabel( this, "Progress" );
    lbl->attach( alignedBelow, sep );
    nicefld = new uiSlider( this, "Nice level" );
    nicefld->attach( ensureBelow, sep );
    nicefld->attach( rightBorder );
    nicefld->setMinValue( -0.5 ); nicefld->setMaxValue( 19.5 );
    nicefld->setValue( hdl.defNiceLevel() );
    nicefld->valueChanged.notify( mCB(this,uiSeisMMProc,niceValChg) );
    uiLabel* nicelbl = new uiLabel( this, "'Nice' level (0-19)", nicefld );
    progrfld = new uiTextEdit( this, "Processing progress", true );
    progrfld->attach( alignedBelow, lbl );
    progrfld->attach( widthSameAs, sep );
    progrfld->setPrefHeightInChar( 7 );
}


Executor& uiSeisMMProc::getFirstJM( const char* prognm, const IOPar& iopar )
{
    const char* res = iopar.find( "Output Seismics Key" );
    BufferString seisoutkey( res ? res : "Output.1.Seismic ID" );
    res = iopar.find( "Inline Range Key" );
    BufferString ilrgkey( res ? res : "Output.1.In-line range" );
    jm = new SeisMMJobMan( prognm, iopar, seisoutkey, ilrgkey );
    newJM();
    return *jm;
}


uiSeisMMProc::~uiSeisMMProc()
{
    delete logvwer;
    delete jm;
}


void uiSeisMMProc::newJM()
{
    if ( !jm ) return;
    jm->poststep.notify( mCB(this,uiSeisMMProc,postStep) );
}


void uiSeisMMProc::doFinalise()
{
    progbar->attach( widthSameAs, machgrp );
    progbar->attach( alignedBelow, progrfld );

    // But we start processing when at least one machine is added.
}


void uiSeisMMProc::postStep( CallBacker* )
{
    const char* txt = jm->progressMessage();
    if ( *txt ) progrfld->append( txt );
    updateCurMachs();
}


void uiSeisMMProc::niceValChg( CallBacker* )
{
    if ( !jm ) return;
    int v = nicefld->getIntValue();
    if ( v > 19 ) v = 19;
    if ( v < 0 ) v = 0;
    jm->setNiceNess( v );
}


void uiSeisMMProc::setDataTransferrer( SeisMMJobMan* newjm )
{
    delete newjm; newjm = 0;
    jmfinished = true;
    task_ = jm->dataTransferrer();
    delay = 0;
    progrfld->append( "Starting data transfer" );
}


void uiSeisMMProc::execFinished()
{
    if ( jmfinished )
    {
	Time_sleep( 5 );
	if ( !jm->removeTempSeis() )
	    ErrMsg( "Could not remove all temporary seismics" );
	progrfld->append( "Data transferred" );
	statusBar()->message( "Finished", 0 );
	finished = true;
	setOkText( "Quit" ); setCancelText( "Quit" );
    }
    else
    {
	stopRunningJobs();
	updateCurMachs();
	SeisMMJobMan* newjm = new SeisMMJobMan( *jm );
	const int nrlines = newjm->totalNr();
	if ( nrlines < 1 )
	    setDataTransferrer( newjm );
	else
	{
	    BufferString msg( "The following inlines were not calculated.\n" );
	    msg += "This may be due to gaps or an unexpected error.\n";
	    for ( int idx=0; idx<nrlines; idx++ )
	    {
		msg += newjm->lineToDo(idx);
		if ( idx != nrlines-1 ) msg += " ";
	    }
	    msg += "\nDo you want to try to calculate these lines?";
	    int res = uiMSG().askGoOnAfter( msg, "Quit program" );
	    if ( res == 2 )
		reject(this);
	    else if ( res == 1 )
		setDataTransferrer( newjm );
	    else
	    {
		uiMSG().message( "Please select the hosts to perform"
				 " the remaining calculations" );
		delete jm; jm = newjm;
		task_ = newjm;
		newJM();
	    }
	}
	first_time = true;
	timerTick(0);
    }
}


void uiSeisMMProc::updateCurMachs()
{
    ObjectSet<BufferString> machs;
    jm->getActiveMachines( machs );
    sort( machs );
    const int oldsz = usedmachfld->box()->size();

    const int newsz = machs.size();
    bool chgd = newsz != oldsz;
    if ( !chgd )
    {
	// Check in detail
	for ( int idx=0; idx<oldsz; idx++ )
	    if ( *machs[idx] != usedmachfld->box()->textOfItem(idx) )
		{ chgd = true; break; }
    }

    if ( !chgd ) return;

    int curit = oldsz ? usedmachfld->box()->currentItem() : -1;
    usedmachfld->box()->empty();
    if ( newsz )
    {
	usedmachfld->box()->addItems( machs );
	deepErase( machs );
	if ( curit >= usedmachfld->box()->size() )
	    curit = usedmachfld->box()->size() - 1;
	usedmachfld->box()->setCurrentItem(curit);
    }
    stopbut->setSensitive( newsz );
    vwlogbut->setSensitive( newsz );
}


bool uiSeisMMProc::rejectOK( CallBacker* )
{
    BufferString msg;

    if ( !running ) return true;

    int res = 0;
    if ( !finished )
    {
	msg = "This will stop all processing!\n\n";
	msg += "Do you want to remove already processed data?";
	res = uiMSG().askGoOnAfter( msg );
    }

    if ( res == 2 )
	return false;

    stopRunningJobs();

    if ( res == 0 )
    {
	if ( !jm->removeTempSeis() )
	    ErrMsg( "Could not remove all temporary seismics" );
	jm->cleanup();
    }

    return true;
}


void uiSeisMMProc::stopRunningJobs()
{
    const int nrleft = usedmachfld->box()->size();
    if ( nrleft )
    {
	statusBar()->message( "Stopping running jobs" );
	for ( int idx=0; idx<nrleft; idx++ )
	{
	    usedmachfld->box()->setCurrentItem(0);
	    stopPush( 0 );
	}
	statusBar()->message( "" );
    }
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisMMProc::addPush( CallBacker* )
{
    for( int idx=0; idx<avmachfld->box()->size(); idx++ )
    {
	if ( avmachfld->box()->isSelected(idx) )
	{
	    BufferString hnm( avmachfld->box()->textOfItem(idx) );
	    char* ptr = strchr( hnm.buf(), '/' );
	    if ( ptr ) *(--ptr) = '\0';
	    jm->addHost( hnm );
	}
    }

    if ( !running && jm->nrHostsInQueue() )
    {
	tmpstordirfld->setSensitive(false);
	jm->setTempStorageDir( tmpstordirfld->getInput() );
	jm->setRemExec( rshcomm );
	running = true;
	prepareNextCycle(0);
    }
}


bool uiSeisMMProc::getCurMach( BufferString& mach ) const
{
    int curit = usedmachfld->box()->currentItem();
    if ( curit < 0 ) return false;

    mach = usedmachfld->box()->textOfItem(curit);
    return true;
}


void uiSeisMMProc::stopPush( CallBacker* )
{
    BufferString mach;
    if ( !getCurMach(mach) ) { pErrMsg("Can't find machine"); return; }
    jm->removeHost( mach );
    updateCurMachs();
}


void uiSeisMMProc::vwLogPush( CallBacker* )
{
    BufferString mach;
    if ( !getCurMach(mach) ) return;

    BufferString fname;
    if ( !jm->getLogFileName(mach,fname) )
	mErrRet("Cannot find log file")

    delete logvwer;
    logvwer = new uiFileBrowser( this, fname );
    logvwer->go();
}


#include "seissingtrcproc.h"
#include <fstream.h>

bool uiSeisMMProc::acceptOK(CallBacker*)
{
    if ( finished )
	return true;
    if ( jmfinished ) // Transferring data!
	return false;

    int res = 0;
    if ( usedmachfld->box()->size() )
    {
	const char* msg = "This will stop processing and start data transfer"
	    		  " now.\n\nDo you want to continue?";
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

    bool mkdump = true;
    if ( mkdump )
    {

	// Stop during operation. Create a dump
	BufferString dumpfname( GetDataDir() );
	dumpfname = File_getFullPath( dumpfname, "Proc" );
	dumpfname = File_getFullPath( dumpfname, "mmbatch_dump.txt" );
	ofstream ostrm( dumpfname );
	ostream* strm = &cerr;
	if ( ostrm.fail() )
	    cerr << "Cannot open dump file '" << dumpfname << "'" << endl;
	else
	{
	    cerr << "Writing to dump file '" << dumpfname << "'" << endl;
	    strm = &ostrm;
	}

	*strm << "Multi-machine-batch dump at " << Time_getLocalString() << endl;
	if ( !jm )
	{
	    *strm << "No Job Manager. Therefore, data transfer is busy, or "
		      "should have already finished" << endl;
	    if ( !task_ )
		{ *strm << "No task_ either. Huh?" << endl; return false; }
	    mDynamicCastGet(SeisSingleTraceProc*,stp,task_)
	    if ( !stp )
		*strm << "Huh? task_ should really be a SeisSingleTraceProc!\n"; 
	    else
		*strm << "SeisSingleTraceProc:\n"
		       << stp->nrDone() << "/" << stp->totalNr() << endl
		       << stp->message() << endl;
	    return false;
	}

	if ( task_ != jm )
	    *strm << "task_ != jm . Why?" << endl;

	jm->dump( *strm );
    }

    execFinished();
    return false;
}
