/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiveldesc.cc,v 1.36 2010-03-18 19:46:05 cvskris Exp $";

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emsurfaceiodata.h"
#include "ioman.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "separstr.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uistaticsdesc.h"
#include "uitaskrunner.h"
#include "unitofmeasure.h"
#include "zdomain.h"

static const char* sKeyDefVelCube = "Default.Cube.Velocity";


uiVelocityDesc::uiVelocityDesc( uiParent* p, const uiVelocityDesc::Setup* vsu )
    : uiGroup( p, "Velocity type selector" )
{
    typefld_ = new uiGenInput( this, "Velocity type",
			StringListInpSpec(VelocityDesc::TypeNames()) );
    typefld_->valuechanged.notify( mCB(this,uiVelocityDesc,updateFlds) );

    uiGroup* vigrp = new uiGroup( this, "Vel info grp" );
    hasstaticsfld_ = new uiGenInput( vigrp, "Has statics", BoolInpSpec(true) );
    hasstaticsfld_->valuechanged.notify(mCB(this,uiVelocityDesc,updateFlds));
    staticsfld_ = new uiStaticsDesc( vigrp, 0 );
    staticsfld_->attach( alignedBelow, hasstaticsfld_ );
    vigrp->setHAlignObj( hasstaticsfld_ );
    vigrp->attach( alignedBelow, typefld_ );

    setdefbox_ = new uiCheckBox( this, "Set as default" );
    setdefbox_->attach( alignedBelow, vigrp );

    setHAlignObj( typefld_ );

    set( vsu ? vsu->desc_ : VelocityDesc() );
}


void uiVelocityDesc::updateFlds( CallBacker* )
{
    VelocityDesc::Type type = (VelocityDesc::Type) typefld_->getIntValue();
    if ( type!=VelocityDesc::RMS )
    {
	hasstaticsfld_->display( false );
	staticsfld_->display( false );
	return;
    }

    hasstaticsfld_->display( true );
    staticsfld_->display( hasstaticsfld_->getBoolValue() );
}


void uiVelocityDesc::set( const VelocityDesc& desc )
{
    typefld_->setValue( desc.type_ );
    hasstaticsfld_->setValue( !desc.statics_.horizon_.isEmpty() );
    staticsfld_->set( desc.statics_ );
    updateFlds( 0 ); 
}


bool uiVelocityDesc::get( VelocityDesc& res, bool disperr ) const
{
    res.type_ = (VelocityDesc::Type) typefld_->getIntValue();
    if ( res.type_!=VelocityDesc::RMS || !hasstaticsfld_->getBoolValue() )
    {
	res.statics_.horizon_.setEmpty();
	res.statics_.vel_ = mUdf(float);
	res.statics_.velattrib_.setEmpty();
    }
    else
    {
	if ( !staticsfld_->get( res.statics_, disperr ) )
	    return false;
    }

    return true;
}


bool uiVelocityDesc::updateAndCommit( IOObj& ioobj, bool disperr )
{
    VelocityDesc desc;
    if ( !get( desc, disperr ) )
	return false;

    if ( desc.type_ != VelocityDesc::Unknown )
	desc.fillPar( ioobj.pars() );
    else
	desc.removePars( ioobj.pars() );
    
    if ( !IOM().commitChanges(ioobj) )
    {
	if ( disperr ) uiMSG().error("Cannot write velocity information");
	return false;
    }

    if ( setdefbox_->isChecked() )
    {
	SI().getPars().set( sKeyDefVelCube, ioobj.key() );
	SI().savePars();
    }

    return true;
}


uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p, const IOObj* sel,
				      const uiVelocityDesc::Setup* vsu )
    : uiDialog( this, uiDialog::Setup("Edit velocity information",0,"103.6.7") )
{
    uiSeisSel::Setup ssu( Seis::Vol ); ssu.seltxt( "Velocity cube" );
    volselfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,true),
	    			ssu );
    if ( sel ) volselfld_->setInput( *sel );
    volselfld_->selectionDone.notify(mCB(this,uiVelocityDescDlg,volSelChange) );

    veldescfld_ = new uiVelocityDesc( this, vsu );
    veldescfld_->attach( alignedBelow, volselfld_ );

    BufferString str( "Vavg at top " );
    str += VelocityDesc::getVelUnit( true );

    topavgvelfld_ =
	new uiGenInput( this, str.buf(), FloatInpIntervalSpec(false) );
    topavgvelfld_->attach( alignedBelow, veldescfld_ );

    scanavgvel_ = new uiPushButton( this, "Scan",
			    mCB(this,uiVelocityDescDlg, scanAvgVelCB ), false );
    scanavgvel_->attach( rightOf, topavgvelfld_ );

    str = "Vavg at bottom ";
    str += VelocityDesc::getVelUnit( true );

    botavgvelfld_ =
	new uiGenInput( this, str.buf(), FloatInpIntervalSpec(false) );
    botavgvelfld_->attach( alignedBelow, topavgvelfld_ );

    volSelChange( 0 );
}


uiVelocityDescDlg::~uiVelocityDescDlg()
{ }


IOObj* uiVelocityDescDlg::getSelection() const
{
    return volselfld_->getIOObj(true);
}


void uiVelocityDescDlg::volSelChange(CallBacker*)
{
    const IOObj* ioobj = volselfld_->ioobj( true );
    scanavgvel_->setSensitive( ioobj );

    VelocityDesc vd;
    Interval<float> topavgvel = Time2DepthStretcher::getDefaultVAvg();
    Interval<float> botavgvel = Time2DepthStretcher::getDefaultVAvg();

    if ( ioobj )
    {
	vd.usePar( ioobj->pars() );
	ioobj->pars().get( VelocityStretcher::sKeyTopVavg(), topavgvel );
	ioobj->pars().get( VelocityStretcher::sKeyBotVavg(), botavgvel );
    }

    veldescfld_->set( vd );
    topavgvelfld_->setValue( topavgvel );
    botavgvelfld_->setValue( botavgvel );
}


void uiVelocityDescDlg::scanAvgVelCB( CallBacker* )
{
    const IOObj* ioobj = volselfld_->ioobj( true );
    if ( !ioobj )
	return;

    VelocityDesc desc;
    if ( !veldescfld_->get( desc, true ) )
	return;

    VelocityModelScanner scanner( *ioobj, desc );
    uiTaskRunner tr( this );
    if ( tr.execute( scanner ) )
    {
	topavgvelfld_->setValue( scanner.getTopVAvg() );
	botavgvelfld_->setValue( scanner.getBotVAvg() );
    }
}


bool uiVelocityDescDlg::acceptOK(CallBacker*)
{
    volselfld_->commitInput();
    PtrMan<IOObj> ioobj = volselfld_->getIOObj( false );
    if ( !ioobj )
	return false;

    ioobj->pars().set( VelocityStretcher::sKeyTopVavg(),
	    	       topavgvelfld_->getFInterval(0) );
    ioobj->pars().set( VelocityStretcher::sKeyBotVavg(),
	    	       botavgvelfld_->getFInterval(0) );
    return veldescfld_->updateAndCommit( *ioobj, true );
}


uiVelSel::uiVelSel( uiParent* p, IOObjContext& ctxt,
		    const uiSeisSel::Setup& setup )
    : uiSeisSel( p, ctxt, setup )
{
    editcubebutt_ = new uiPushButton( this, "",
	    mCB(this,uiVelSel,editCB), false );
    editcubebutt_->attach( rightOf, selbut_ );
    updateEditButton( 0 );
    selectionDone.notify( mCB(this,uiVelSel,updateEditButton) );

    const char* res = SI().pars().find( sKeyDefVelCube );
    if ( res && *res && IOObj::isKey(res) )
	setInput( MultiID(res) );
}


const IOObjContext& uiVelSel::ioContext()
{
    static PtrMan<IOObjContext> velctxt = 0;
    if ( !velctxt )
    {
	velctxt = new IOObjContext( SeisTrcTranslatorGroup::ioContext() );
	velctxt->deftransl = "CBVS";
	velctxt->parconstraints.setYN( VelocityDesc::sKeyIsVelocity(), true );
	velctxt->includeconstraints = true;
	velctxt->allowcnstrsabsent = false;
    }

    return *velctxt;
}


void uiVelSel::editCB(CallBacker*)
{
    uiVelocityDescDlg dlg( this, workctio_.ioobj );
    if ( dlg.go() )
	workctio_.setObj( dlg.getSelection() );

    updateEditButton( 0 );
}


void uiVelSel::setInput( const MultiID& mid )
{
    uiIOObjSel::setInput( mid );
    updateEditButton( 0 );
}


void uiVelSel::updateEditButton(CallBacker*)
{
    editcubebutt_->setText( ioobj(true) ? "Edit ..." : "Create ..." );
}

uiTimeDepthBase::uiTimeDepthBase( uiParent* p, bool t2d )
    : uiZAxisTransform( p )
    , transform_ ( 0 )
    , t2d_( t2d )
{
    usevelfld_ = new uiGenInput(this, "Use velocity model", BoolInpSpec(true) );
    usevelfld_->valuechanged.notify( mCB(this,uiTimeDepthBase,useVelChangeCB) );

    IOObjContext ctxt = uiVelSel::ioContext();
    ctxt.forread = true;
    uiSeisSel::Setup su( false, false ); su.seltxt("Velocity model");
    velsel_ = new uiVelSel( this, ctxt, su );
    velsel_->attach( alignedBelow, usevelfld_ );

    BufferString str = t2d ? sKey::Depth.str() : sKey::Time.str();
    str += " range ";
    str += UnitOfMeasure::surveyDefDepthUnitAnnot( true, true );

    rangefld_ = new uiGenInput(this, str.buf(),
	    		       FloatInpIntervalSpec(true) );
    rangefld_->attach( alignedBelow, usevelfld_ );
    StepInterval<float> zrange;
    getDefaultZRange( zrange );
    rangefld_->setValue( zrange );

    useVelChangeCB( 0 );
}


uiTimeDepthBase::~uiTimeDepthBase()
{
    if ( transform_ ) transform_->unRef();
}


ZAxisTransform* uiTimeDepthBase::getSelection()
{
    return transform_;
}


StepInterval<float> uiTimeDepthBase::getZRange() const
{ return rangefld_->getFStepInterval(); } 


void uiTimeDepthBase::getDefaultZRange( StepInterval<float>& rg ) const
{
    const Interval<float> velrg = Time2DepthStretcher::getDefaultVAvg();
    rg = SI().zRange( true );
    if ( t2d_ && SI().zIsTime() )
    {
	rg.start *= velrg.start / 2;
	rg.stop *= velrg.stop / 2;
	rg.step *= velrg.center() / 2;
    }
    else if ( !t2d_ && !SI().zIsTime() )
    {
	rg.start /= velrg.start*2;
	rg.stop /= velrg.stop*2;
	rg.step /= velrg.center()*2;
    }
}


const char* uiTimeDepthBase::selName() const
{ return selname_.buf(); }

#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiTimeDepthBase::acceptOK()
{
    if ( transform_ ) transform_->unRef();
    transform_ = 0;

    const IOObj* ioobj = 0;
    if ( !usevelfld_->getBoolValue() )
    {
	const StepInterval<float> zrg = rangefld_->getFStepInterval();
	if ( mIsUdf(zrg.start) || mIsUdf(zrg.stop) || mIsUdf(zrg.step) ||
	     zrg.isRev() || zrg.step<=0 )
	{
	    uiMSG().error( "Z Range must be defined, "
		    "start value must be less than stop value, "
		    "and step must be more than zero." );
	    return false;
	}

	return true;
    }

    ioobj = velsel_->ioobj( false );
    if ( !ioobj )
	return false;

    VelocityDesc desc;
    if ( !desc.usePar( ioobj->pars() ) )
	mErrRet("Cannot read velocity information for selected model");

    BufferString zdomain = ioobj->pars().find( ZDomain::sKey() ).str();
    if ( zdomain.isEmpty() )
	zdomain = SI().getZDomainString();

    if ( zdomain==ZDomain::sKeyTWT() )
    {
	if ( desc.type_ != VelocityDesc::Interval &&
	     desc.type_ != VelocityDesc::RMS )
	    mErrRet("Only RMS and Interval allowed for time based models");
    }
    else if ( zdomain==ZDomain::sKeyDepth() )
    {
	if ( desc.type_ != VelocityDesc::Interval )
	    mErrRet("Only Interval velocity allowed for time based models");
    }
    else
    {
	mErrRet( "Velocity model must be in either time or depth");
    }

    if ( t2d_ )
    {
	mTryAlloc( transform_, Time2DepthStretcher() );
    }
    else
    {
	mTryAlloc( transform_, Depth2TimeStretcher() );
    }

    if ( !transform_ )
	mErrRet("Could not allocate memory");

    transform_->ref();
    if ( !transform_->setVelData( ioobj->key()  ) || !transform_->isOK() )
    {
	FileMultiString fms("Internal: Could not initialize transform" );
	fms += transform_->errMsg();
	uiMSG().errorWithDetails( fms );
	return false;
    }

    selname_ = ioobj->name();

    return true;
}


void uiTimeDepthBase::useVelChangeCB(CallBacker*)
{
    velsel_->display( usevelfld_->getBoolValue() );
    rangefld_->display( !usevelfld_->getBoolValue() );
}


FixedString uiTimeDepthBase::getZDomain() const
{
    return t2d_ ? ZDomain::sKeyDepth() : ZDomain::sKeyTWT();
}


void uiTime2Depth::initClass()
{
    uiZAxisTransform::factory().addCreator( create,
		    Time2DepthStretcher::sName(), "Time to depth" );
}


uiZAxisTransform* uiTime2Depth::create( uiParent* p, const char* fromdomain )
{
    if ( fromdomain!=ZDomain::sKeyTWT() )
	return 0;

    return new uiTime2Depth( p );
}


uiTime2Depth::uiTime2Depth( uiParent* p )
    : uiTimeDepthBase( p, true )
{}


void uiDepth2Time::initClass()
{
    uiZAxisTransform::factory().addCreator( create,
		    Depth2TimeStretcher::sName(), "Depth to Time" );
}


uiZAxisTransform* uiDepth2Time::create( uiParent* p, const char* fromdomain )
{
    if ( fromdomain!=ZDomain::sKeyDepth() )
	return 0;

    return new uiDepth2Time( p );
}


uiDepth2Time::uiDepth2Time( uiParent* p )
    : uiTimeDepthBase( p, false )
{}
