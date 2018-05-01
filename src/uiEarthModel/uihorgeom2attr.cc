/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2011
________________________________________________________________________

-*/

#include "uihorgeom2attr.h"

#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "uihorsavefieldgrp.h"
#include "uiseparator.h"
#include "uistrings.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "emsurfaceauxdata.h"
#include "emmanager.h"
#include "emioobjinfo.h"
#include "survinfo.h"
#include "od_helpids.h"

#define mAddMSFld(txt,att) \
    if ( SI().zIsTime() ) \
    { \
	msfld_ = new uiGenInput( this, txt, BoolInpSpec(true, \
			uiStrings::sMSec(false),uiStrings::sSec(false)) ); \
	msfld_->attach( alignedBelow, att ); \
    }
#define mGetZFac(valifms) \
    const float zfac = (float) (msfld_ && msfld_->getBoolValue() ? valifms : 1)


uiHorGeom2Attr::uiHorGeom2Attr( uiParent* p, EM::Horizon3D& hor )
    : uiGetObjectName( p, Setup(tr("Store Z values as attribute"),
			       getItems(hor)).inptxt(uiStrings::sAttribName()) )
    , hor_(hor)
    , msfld_(0)
{
    hor_.ref();
    setHelpKey( mODHelpKey(mHorGeom2AttrHelpID) );

    mAddMSFld(tr("Store in"),inpfld_)
}


uiHorGeom2Attr::~uiHorGeom2Attr()
{
    delete itmnms_;
    hor_.unRef();
}


BufferStringSet& uiHorGeom2Attr::getItems( const EM::Horizon3D& hor )
{
    itmnms_ = new BufferStringSet;
    EM::IOObjInfo eminfo( hor.id() );
    eminfo.getAttribNames( *itmnms_ );
    return *itmnms_;
}

#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }


bool uiHorGeom2Attr::acceptOK()
{
    if ( !uiGetObjectName::acceptOK() )
	return false;

    int auxidx = hor_.auxdata.auxDataIndex( text() );
    if ( auxidx >= 0 )
	hor_.auxdata.removeAuxData( auxidx );
    auxidx = hor_.auxdata.addAuxData( text() );

    mGetZFac( 1000 );

    PtrMan<EM::ObjectIterator> iter = hor_.createIterator();
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.isInvalid() )
	    break;
	if ( !hor_.geometry().isNodeOK(pid) )
	    continue;

	const float zval = (float) ( hor_.getPos(pid).z_ * zfac );
	hor_.auxdata.setAuxDataVal( auxidx, pid, zval );
    }

    PtrMan<Executor> saver = hor_.auxdata.auxDataSaver( auxidx, true );
    if ( !saver )
	return false;

    uiTaskRunner taskrunner( this );
    return TaskRunner::execute( &taskrunner, *saver );
}


uiHorAttr2Geom::uiHorAttr2Geom( uiParent* p, EM::Horizon3D& hor,
				const DataPointSet& dps, int colid )
    : uiDialog(p, Setup(tr("Set horizon Z values"),(tr("Set Z values from '%1'")
			.arg(toUiString(dps.dataSet().colDef(colid).name_)))
			,mODHelpKey(mHorAttr2GeomHelpID)) )
    , hor_(hor)
    , dps_(dps)
    , colid_(colid-dps.nrFixedCols())
    , msfld_(0)
{
    hor_.ref();

    isdeltafld_ = new uiGenInput( this, tr("Values are"),
		    BoolInpSpec(false,tr("Relative (deltas)"),
				uiStrings::sAbsolute()) );
    mAddMSFld( uiStrings::sUnit(mPlural), isdeltafld_ )

    uiSeparator* sep = new uiSeparator( this, "HSep" );
    if ( msfld_ ) sep->attach( stretchedBelow, msfld_ );
    else sep->attach( stretchedBelow, isdeltafld_ );

    savefldgrp_ = new uiHorSaveFieldGrp( this, &hor_, false );
    savefldgrp_->setSaveFieldName( "Save modified horizon" );
    savefldgrp_->attach( alignedBelow, isdeltafld_ );
    savefldgrp_->attach( ensureBelow, sep );
}


uiHorAttr2Geom::~uiHorAttr2Geom()
{
    hor_.unRef();
}


class uiHorAttr2GeomExec : public Executor
{ mODTextTranslationClass(uiHorAttr2GeomExec)
public:

uiHorAttr2GeomExec( EM::Horizon3D& h, const DataPointSet& dps,
		    int colid, float zfac, bool isdel )
    : Executor("Horizon geometry from attribute")
    , hor_(h)
    , dps_(dps)
    , it_(h.createIterator(0))
    , stepnr_(0)
    , colid_(colid)
    , isdelta_(isdel)
    , zfac_(zfac)
    , horarray_(0)
    , uimsg_(tr("Setting Z values"))
{
    totnr_ = it_->approximateSize();

    hortks_ = hor_.range();
    mDeclareAndTryAlloc( Array2D<float>*, arr,
	    Array2DImpl<float>( hortks_.nrInl(), hortks_.nrCrl() ) );
    if ( arr && !arr->isEmpty() )
    {
	arr->setAll( mUdf(float) );
	horarray_ = arr;
    }

    hor_.enableGeometryChecks( false );
}

~uiHorAttr2GeomExec()
{
    delete it_;
    delete horarray_;
}

uiString message() const	{ return uimsg_; }
uiString nrDoneText() const	{ return tr("Nodes done"); }
od_int64 nrDone() const		{ return stepnr_ * 1000; }
od_int64 totalNr() const	{ return totnr_; }

int nextStep()
{
    if ( !horarray_ )
    {
	uimsg_ = uiStrings::phrCannotCreate( toUiString("array") );
	return ErrorOccurred();
    }

    for ( int idx=0; idx<1000; idx++ )
    {
	const EM::PosID pid = it_->next();
	if ( pid.isInvalid() )
	{
	    fillHorizonArray();
	    return Finished();
	}

	const BinID bid = pid.getRowCol();
	DataPointSet::RowID rid = dps_.findFirst( bid );
	Coord3 crd = hor_.getPos( pid );
	if ( rid < 0 )
	{
	    if ( !isdelta_ )
		crd.z_ = mUdf(float);
	}
	else
	{
	    float newz = dps_.value( colid_, rid );
	    if ( mIsUdf(newz) && isdelta_ )
		newz = 0;

	    if ( mIsUdf(newz) )
		crd.z_ = newz;
	    else
	    {
		newz *= zfac_;
		if ( isdelta_ )
		    crd.z_ += newz;
		else
		    crd.z_ = newz;
	    }
	}

	const int inlidx = hortks_.inlIdx( bid.inl() );
	const int crlidx = hortks_.crlIdx( bid.crl() );
	if ( !horarray_->info().validPos(inlidx,crlidx) )
	    continue;
	horarray_->set( inlidx, crlidx, mCast(float,crd.z_) );
    }
    stepnr_++;
    return MoreToDo();
}


void fillHorizonArray()
{
    Geometry::BinIDSurface* geom = hor_.geometry().geometryElement();
    geom->setArray( hortks_.start_, hortks_.step_, horarray_, true );
    horarray_ = 0;
    hor_.enableGeometryChecks( true );
}

    EM::Horizon3D&		hor_;
    const DataPointSet&		dps_;
    EM::ObjectIterator*	it_;
    const int			colid_;
    od_int64			stepnr_;
    od_int64			totnr_;
    bool			isdelta_;
    const float			zfac_;
    Array2D<float>*		horarray_;
    TrcKeySampling		hortks_;
    uiString			uimsg_;
};


bool uiHorAttr2Geom::acceptOK()
{
    mGetZFac( 0.001f );
    const bool isdelta = isdeltafld_->getBoolValue();

    if ( !savefldgrp_->acceptOK() )
	return false;

    EM::Horizon3D* usedhor = &hor_;
    if ( !savefldgrp_->overwriteHorizon() )
	mDynamicCast( EM::Horizon3D*, usedhor, savefldgrp_->getNewHorizon() );

    if ( !usedhor ) return false;
    uiHorAttr2GeomExec exec( *usedhor, dps_, colid_, zfac, isdelta );
    uiTaskRunner taskrunner( this );
    const bool res = TaskRunner::execute( &taskrunner, exec )
	&& savefldgrp_->saveHorizon();
    return res;
}
