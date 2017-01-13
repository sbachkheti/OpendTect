
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : May 2007
-*/


#include "uituthortools.h"
#include "tuthortools.h"

#include "ioobjctxt.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "ioobj.h"
#include "transl.h"
#include "keystrs.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uistrings.h"

uiTutHorTools::uiTutHorTools( uiParent* p )
	: uiDialog( p, Setup( tr("Tut Horizon tools"),
			      tr("Specify process parameters"),
			      HelpKey("tut","hor") ) )
{
    taskfld_= new uiGenInput( this, tr("Task"),
			BoolInpSpec(true,tr("Thickness between two horizons"),
					 tr("Smooth a horizon")) );
    taskfld_->valuechanged.notify( mCB(this,uiTutHorTools,choiceSel) );

    IOObjContext ctxt = mIOObjContext(EMHorizon3D);
    inpfld_ = new uiIOObjSel( this, ctxt, tr("Input Horizon") );
    inpfld_->attach( alignedBelow, taskfld_ );

    // For thickness calculation
    inpfld2_ = new uiIOObjSel( this, ctxt, uiStrings::sBottomHor() );
    inpfld2_->attach( alignedBelow, inpfld_ );

    selfld_= new uiGenInput( this, tr("Add Result as an Attribute to "),
			BoolInpSpec(true, uiStrings::sTopHor(),
				    uiStrings::sBottomHor()) );
    selfld_->attach( alignedBelow, inpfld2_ );

    attribnamefld_ = new uiGenInput( this, uiStrings::sAttribName(),
			StringInpSpec( sKey::Thickness() ) );
    attribnamefld_->attach( alignedBelow, selfld_ );

    // For smoothing
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			    uiStrings::phrOutput( uiStrings::sHorizon() ) );
    outfld_->attach( alignedBelow, inpfld_ );

    strengthfld_ = new uiGenInput( this, tr("Filter Strength"),
			BoolInpSpec(true, tr("Low"), tr("High")) );
    strengthfld_->attach( alignedBelow, attribnamefld_ );

    listboxfld1_ = new uiListBox( this, "Random ListBox",
			OD::ChoiceMode::ChooseAtLeastOne );
    listboxfld1_->attach( alignedBelow, strengthfld_ );
    uiStringSet uistr;
    uistr.add(uiStrings::sNo());
    uistr.add(uiStrings::sAbort());
    uistr.add(uiStrings::sBelow());
    uistr.add(uiStrings::sCannotWrite());
    uistr.add(uiStrings::sCoefficient());
    uistr.add(uiStrings::s2DPlane());
    uistr.add(uiStrings::sZip());
    uistr.add(uiStrings::sZ());

    listboxfld2_ = new uiListBox( this, "Random ListBox",
			OD::ChoiceMode::ChooseAtLeastOne );
    listboxfld2_->attach( alignedBelow, listboxfld1_ );

    listboxfld1_->addItems(uistr);
    uistr.sort(true, true);
    listboxfld2_->addItems(uistr);
    postFinalise().notify( mCB(this,uiTutHorTools,choiceSel) );
}


uiTutHorTools::~uiTutHorTools()
{
}


void uiTutHorTools::choiceSel( CallBacker* )
{
    const bool isthick = taskfld_->getBoolValue();
    inpfld_->setLabelText( isthick ? uiStrings::sTopHor()
				   : tr("Input Horizon") );
    inpfld2_->display( isthick );
    selfld_->display( isthick );
    attribnamefld_->display( isthick );
    outfld_->display( !isthick );
    strengthfld_->display( !isthick );
}


bool uiTutHorTools::checkAttribName() const
{
    int attridx = -1;
    BufferString attrnm = attribnamefld_->text();
    const bool top = selfld_->getBoolValue();
    const DBKey key = top ? inpfld_->key() : inpfld2_->key();
    EM::SurfaceIOData sd;
    uiString errmsg;
    if ( !EM::EMM().getSurfaceData(key,sd,errmsg) )
	return false;

    for ( int idx=0; idx<sd.valnames.size(); idx++ )
    {
	if ( attrnm != sd.valnames.get(idx) )
	    continue;

	attridx = idx;
	break;
    }

    if ( attridx < 0 )
	return true;

    uiString msg = tr("This surface already has an attribute called:\n%1"
		      "\nDo you wish to overwrite this data?")
		 .arg(sd.valnames.get(attridx));
    return uiMSG().askOverwrite( msg );
}


bool uiTutHorTools::acceptOK()
{
    if ( !inpfld_->ioobj() ) return false;

    const bool isthick = taskfld_->getBoolValue();
    return isthick ? doThicknessCalc() : doSmoother();
}


#define mGetHor(varnm,fld) \
    RefMan<EM::EMObject> varnm##_emobj = \
	EM::EMM().loadIfNotFullyLoaded( (fld)->key(), &taskrunner ); \
    mDynamicCastGet(EM::Horizon3D*,varnm,varnm##_emobj.ptr()) \
    if ( !varnm ) return false;

bool uiTutHorTools::doThicknessCalc()
{
    if ( inpfld_->key() == inpfld2_->key()
	    && !uiMSG().askGoOn(tr("Input horizon same as Output. Continue?")) )
	return false;

    if ( !inpfld2_->ioobj() )
	return false;

    const bool cont = checkAttribName();
    if ( !cont ) return false;

    uiTaskRunner taskrunner( this );
    Tut::ThicknessCalculator* calc = new Tut::ThicknessCalculator;
    const bool top = selfld_->getBoolValue();
    mGetHor( hor1, top ? inpfld_ : inpfld2_ );
    mGetHor( hor2, top ? inpfld2_ : inpfld_ );
    calc->setHorizons( hor1, hor2 );
    calc->init( attribnamefld_->text() );

    if ( !taskrunner.execute(*calc) )
	return false;

    PtrMan<Executor> saver = calc->dataSaver();
    return taskrunner.execute( *saver );
}


bool uiTutHorTools::doSmoother()
{
    if ( !outfld_->ioobj() )
	return false;

    uiTaskRunner taskrunner( this );
    Tut::HorSmoother* calc = new Tut::HorSmoother;
    mGetHor( hor, inpfld_ );
    calc->setHorizons( hor );
    calc->setWeak( strengthfld_->getBoolValue() );

    if ( !taskrunner.execute(*calc) )
	return false;

    PtrMan<Executor> saver = calc->dataSaver( outfld_->key() );
    return taskrunner.execute( *saver );
}
