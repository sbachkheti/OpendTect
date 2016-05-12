/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vishorizon2ddisplay.h"

#include "bendpointfinder.h"
#include "emhorizon2d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "keystrs.h"
#include "rowcolsurface.h"
#include "survinfo.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vispointset.h"
#include "visseis2ddisplay.h"
#include "viscoord.h"
#include "vistransform.h"
#include "zaxistransform.h"
#include "seisioobjinfo.h"
#include "geom2dintersections.h"
#include "hiddenparam.h"

namespace visSurvey
{
static HiddenParam< Horizon2DDisplay,visBase::MarkerSet* > intersectmkset_(0);
static HiddenParam< Horizon2DDisplay,char > updateintsectmarkers_(true);
static HiddenParam< Horizon2DDisplay,int > nr2dlines_(0);
static HiddenParam< Horizon2DDisplay,Line2DInterSectionSet* > ln2dset_(0);

Horizon2DDisplay::Horizon2DDisplay()
{
    intersectmkset_.setParam( this, visBase::MarkerSet::create() );
    updateintsectmarkers_.setParam( this, true );
    nr2dlines_.setParam( this, 0 );
    ln2dset_.setParam( this, 0 );
    
    points_.allowNull(true);
    EMObjectDisplay::setLineStyle( LineStyle(LineStyle::Solid,5 ) );
    visBase::MarkerSet* mkset = intersectmkset_.getParam(this);
    mkset->ref();
    addChild( mkset->osgNode() );
    mkset->setMaterial( new visBase::Material );
    mkset->setMarkerStyle( MarkerStyle3D::Sphere );
    mkset->setScreenSize( 4.0 );
}


Horizon2DDisplay::~Horizon2DDisplay()
{
    setZAxisTransform( 0, 0 );

    for ( int idx=0; idx<sids_.size(); idx++ )
	removeSectionDisplay( sids_[idx] );

    removeEMStuff();
    intersectmkset_.getParam(this)->unRef();
    intersectmkset_.removeParam( this );
    updateintsectmarkers_.removeParam( this );

    if ( ln2dset_.getParam( this ) )
	delete ln2dset_.getParam( this );
    ln2dset_.removeParam( this );

    nr2dlines_.removeParam( this );
}


void Horizon2DDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    EMObjectDisplay::setDisplayTransformation( nt );

    for ( int idx=0; idx<lines_.size(); idx++ )
	lines_[idx]->setDisplayTransformation( transformation_ );

    for ( int idx=0; idx<points_.size(); idx++ )
    {
	if( points_[idx] )
	    points_[idx]->setDisplayTransformation(transformation_);
    }

    intersectmkset_.getParam(this)->setDisplayTransformation( transformation_ );
}


void Horizon2DDisplay::getMousePosInfo(const visBase::EventInfo& eventinfo,
				       Coord3& mousepos,
				       BufferString& val,
				       BufferString& info) const
{
    EMObjectDisplay::getMousePosInfo( eventinfo, mousepos, val, info );
    const EM::SectionID sid =
		    EMObjectDisplay::getSectionID( &eventinfo.pickedobjids );
    if ( sid<0 ) return;

    mDynamicCastGet( const Geometry::RowColSurface*, rcs,
		     emobject_->sectionGeometry(sid));
    if ( !rcs ) return;

    const StepInterval<int> rowrg = rcs->rowRange();
    RowCol rc;
    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
    {
	const StepInterval<int> colrg = rcs->colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    const Coord3 pos = emobject_->getPos( sid, rc.toInt64() );
	    if ( pos.sqDistTo(mousepos) < mDefEps )
	    {
		mDynamicCastGet( const EM::Horizon2D*, h2d, emobject_ );
		info += ", Linename: ";
		info += h2d->geometry().lineName( rc.row() );
		return;
	    }

	}
    }
}


EM::SectionID Horizon2DDisplay::getSectionID(int visid) const
{
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	if ( lines_[idx]->id()==visid ||
	     (points_[idx] && points_[idx]->id()==visid) )
	    return sids_[idx];
    }

    return -1;
}


const visBase::PolyLine3D* Horizon2DDisplay::getLine(
	const EM::SectionID& sid ) const
{
    for ( int idx=0; idx<sids_.size(); idx++ )
	if ( sids_[idx]==sid ) return lines_[idx];

    return 0;
}


const visBase::PointSet* Horizon2DDisplay::getPointSet(
	const EM::SectionID& sid ) const
{
    for ( int idx=0; idx<sids_.size(); idx++ )
	if ( sids_[idx]==sid ) return points_[idx];

    return 0;
}

void Horizon2DDisplay::setLineStyle( const LineStyle& lst )
{
    // TODO: set the draw style correctly after properly implementing
    // different line styles. Only SOLID is supported now.
    //EMObjectDisplay::drawstyle_->setDrawStyle( visBase::DrawStyle::Lines );

    EMObjectDisplay::setLineStyle( lst );
    for ( int idx=0; idx<lines_.size(); idx++ )
	lines_[idx]->setLineStyle( lst );

    drawstyle_->setLineStyle( lst );
}


bool Horizon2DDisplay::addSection( const EM::SectionID& sid, TaskRunner* taskr )
{
    visBase::PolyLine3D* pl = visBase::PolyLine3D::create();
    pl->ref();
    pl->setDisplayTransformation( transformation_ );
    pl->setName( toUiString("PolyLine3D") );
    pl->setLineStyle( drawstyle_->lineStyle() );
    addChild( pl->osgNode() );
    lines_ += pl;
    points_ += 0;
    sids_ += sid;

    updateSection( sids_.size()-1 );

    return true;
}


void Horizon2DDisplay::removeSectionDisplay( const EM::SectionID& sid )
{
    const int idx = sids_.indexOf( sid );
    if ( idx<0 ) return;

    removeChild( lines_[idx]->osgNode() );
    lines_[idx]->unRef();
    if ( points_[idx] )
    {
	removeChild( points_[idx]->osgNode() );
	points_[idx]->unRef();
    }

    points_.removeSingle( idx );
    lines_.removeSingle( idx );
    sids_.removeSingle( idx );
}


bool Horizon2DDisplay::withinRanges( const RowCol& rc, float z,
				     const LineRanges& linergs )
{
    if ( rc.row()<linergs.trcrgs.size() && rc.row()<linergs.zrgs.size() )
    {
	for ( int idx=0; idx<linergs.trcrgs[rc.row()].size(); idx++ )
	{
	    if ( idx<linergs.zrgs[rc.row()].size() &&
		 linergs.zrgs[rc.row()][idx].includes(z,true) &&
		 linergs.trcrgs[rc.row()][idx].includes(rc.col(),true) )
		return true;
	}
    }
    return false;
}


class Horizon2DDisplayUpdater : public ParallelTask
{
public:
Horizon2DDisplayUpdater( const Geometry::RowColSurface* rcs,
		const Horizon2DDisplay::LineRanges* lr,
		visBase::VertexShape* shape, visBase::PointSet* points,
		ZAxisTransform* zaxt, const TypeSet<Pos::GeomID>& geomids,
		TypeSet<int>& volumeofinterestids)
    : surf_( rcs )
    , lines_( shape )
    , points_( points )
    , lineranges_( lr )
    , scale_( 1, 1, SI().zScale() )
    , zaxt_( zaxt )
    , geomids_(geomids)
    , crdidx_(0)
    , volumeofinterestids_( volumeofinterestids )
{
    eps_ = mMIN(SI().inlDistance(),SI().crlDistance());
    eps_ = (float) mMIN(eps_,SI().zRange(true).step*scale_.z )/4;

    rowrg_ = surf_->rowRange();
    nriter_ = rowrg_.isRev() ? 0 : rowrg_.nrSteps()+1;
}


od_int64 nrIterations() const { return nriter_; }


int getNextRow()
{
    Threads::MutexLocker lock( lock_ );
    if ( curidx_>=nriter_ )
	return mUdf(int);

    const int res = rowrg_.atIndex( curidx_++ );
    return res;
}


void prepareForTransform( int rowidx, Pos::GeomID geomid,
			  const StepInterval<int>& colrg )
{
    Threads::MutexLocker lock( lock_ );
    TrcKeyZSampling cs;
    cs.hsamp_.start_ = BinID( geomid, colrg.start );
    cs.hsamp_.step_ = BinID( colrg.step, colrg.step );
    cs.hsamp_.stop_ = BinID( geomid , colrg.stop );

    int& voiid = volumeofinterestids_[rowidx];
    if ( voiid==-1 && zaxt_->needsVolumeOfInterest() )
	voiid = zaxt_->addVolumeOfInterest( cs );
    else if ( voiid>=0 )
	zaxt_->setVolumeOfInterest( voiid, cs );

    zaxt_->loadDataIfMissing( voiid );
}


bool doPrepare( int nrthreads )
{
    curidx_ = 0;
    nrthreads_ = nrthreads;
    points_->removeAllPoints();
    lines_->removeAllPrimitiveSets();
    return true;
}


bool doFinish( bool res )
{
    return res;
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    RowCol rc;
    while ( true )
    {
	const int rowidx = getNextRow();
	if ( mIsUdf(rowidx) )
	    break;

	rc.row() = rowidx;
	const Pos::GeomID geomid =
	    geomids_.validIdx(rowidx) ? geomids_[rowidx] : Pos::GeomID();

	TypeSet<Coord3> positions;
	const StepInterval<int> colrg = surf_->colRange( rowidx );

	if ( zaxt_ )
	    prepareForTransform( rowidx, geomid, colrg );

	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    Coord3 pos = surf_->getKnot( rc );
	    const float zval = mCast(float,pos.z);

	    if ( !pos.isDefined() || (lineranges_ &&
		 !Horizon2DDisplay::withinRanges(rc,zval,*lineranges_)) )
	    {
		if ( positions.size() )
		    sendPositions( positions );
	    }
	    else
	    {
		if ( zaxt_ )
		    pos.z = zaxt_->transform( pos );

		if ( !mIsUdf(pos.z) )
		    positions += pos;
		else if ( positions.size() )
		    sendPositions( positions );
	    }
	}

	sendPositions( positions );
    }

    return true;
}

void sendPositions( TypeSet<Coord3>& positions )
{
    if ( !positions.size() )
	return;

    if ( positions.size()==1 )
    {
	if ( !positions[0].isDefined() )
	    return;
	points_->addPoint( positions[0] );
    }
    else
    {
	BendPointFinder3D finder( positions, scale_, eps_ );
	finder.executeParallel(nrthreads_<2);
	const TypeSet<int>& bendpoints = finder.bendPoints();
	const int nrbendpoints = bendpoints.size();
	if ( nrbendpoints )
	{
	    TypeSet<int> indices;
	    lock_.lock();
	    for ( int idy=0; idy<nrbendpoints; idy++ )
	    {
		const Coord3& pos = positions[ bendpoints[idy] ];
		if ( !pos.isDefined() )
		    continue;

		lines_->getCoordinates()->setPos( crdidx_, pos );
		indices += crdidx_++;
	    }

	    Geometry::IndexedPrimitiveSet* lineprimitiveset =
				Geometry::IndexedPrimitiveSet::create( true );
	    lineprimitiveset->ref();
	    lineprimitiveset->set( indices.arr(), indices.size() );
	    lines_->addPrimitiveSet( lineprimitiveset );
	    lineprimitiveset->unRef();
	    lock_.unLock();
	}
    }

    positions.erase();
}

protected:
    const Geometry::RowColSurface*	surf_;
    const Horizon2DDisplay::LineRanges*	lineranges_;
    visBase::VertexShape*		lines_;
    visBase::PointSet*			points_;
    ZAxisTransform*			zaxt_;
    const TypeSet<Pos::GeomID>&		geomids_;
    Threads::Mutex			lock_;
    int					nrthreads_;
    const Coord3			scale_;
    float				eps_;
    StepInterval<int>			rowrg_;
    int					nriter_;
    int					curidx_;
    int					crdidx_;
    TypeSet<int>&			volumeofinterestids_;
};


void Horizon2DDisplay::updateSection( int idx, const LineRanges* lineranges )
{
    if ( !emobject_ ) return;

    visBase::PointSet* ps = points_.validIdx(idx) ? points_[idx] : 0;
    if ( !ps )
    {
	ps = visBase::PointSet::create();
	ps->ref();
	ps->removeAllPoints();
	ps->setDisplayTransformation( transformation_ );
	points_.replace( idx, ps );
	addChild( ps->osgNode() );
    }

    TypeSet<Pos::GeomID> geomids;
    EM::IOObjInfo info( emobject_->multiID() );
    info.getGeomIDs( geomids );

    LineRanges linergs;
    mDynamicCastGet(const EM::Horizon2D*,h2d,emobject_);
    const bool redo = h2d && zaxistransform_ && geomids.isEmpty();
    if ( redo )
    {
	const EM::Horizon2DGeometry& emgeo = h2d->geometry();
	for ( int lnidx=0; lnidx<emgeo.nrLines(); lnidx++ )
	{
	    const Pos::GeomID geomid = emgeo.geomID( lnidx );
	    geomids += geomid;

	    for ( int idy=0; idy<h2d->nrSections(); idy++ )
	    {
		const Geometry::Horizon2DLine* ghl =
		    emgeo.sectionGeometry( h2d->sectionID(idy) );
		if ( ghl )
		{
		    linergs.trcrgs += TypeSet<Interval<int> >();
		    linergs.zrgs += TypeSet<Interval<float> >();
		    const int ridx = linergs.trcrgs.size()-1;

		    linergs.trcrgs[ridx] += ghl->colRange( geomid );
		    linergs.zrgs[ridx] += ghl->zRange( geomid );
		}
	    }
	}
    }

    const EM::SectionID sid = emobject_->sectionID( idx );
    mDynamicCastGet(const Geometry::RowColSurface*,rcs,
		    emobject_->sectionGeometry(sid));
    const LineRanges* lrgs = redo ? &linergs : lineranges;
    visBase::PolyLine3D* pl = lines_.validIdx(idx) ? lines_[idx] : 0;

    if ( volumeofinterestids_.isEmpty() )
	volumeofinterestids_.setSize( geomids.size(), -1 );

    if ( !rcs || !pl )
	return;

    Horizon2DDisplayUpdater updater( rcs, lrgs, pl, ps,
			zaxistransform_, geomids, volumeofinterestids_ );
    updater.execute();
}


bool Horizon2DDisplay::shouldDisplayIntersections( 
    const Seis2DDisplay& seisdisp )
{
    for ( int idx=0; idx<seisdisp.nrAttribs(); idx++ )
    {
	const bool hasattribenable = seisdisp.isAttribEnabled(idx);
	const DataPack::ID dpid = seisdisp.getDataPackID(idx);

	if ( hasattribenable && dpid )
	    return true;
    }
    return false;
}


void Horizon2DDisplay::emChangeCB( CallBacker* cb )
{
    updateintsectmarkers_.setParam( this, true );
    EMObjectDisplay::emChangeCB( cb );
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	getMaterial()->setColor( emobject_->preferredColor() );
	setLineStyle( emobject_->preferredLineStyle() );
    }
}


void Horizon2DDisplay::updateLinesOnSections(
			const ObjectSet<const Seis2DDisplay>& seis2dlist )
{
    if ( !displayonlyatsections_ )
    {
	for ( int sidx=0; sidx<sids_.size(); sidx++ )
	    updateSection( sidx );
	intersectmkset_.getParam(this)->turnOn( displayonlyatsections_ );
	return;
    }

    mDynamicCastGet(const EM::Horizon2D*,h2d,emobject_)
    if ( !h2d ) return;

    LineRanges linergs;
    for ( int lnidx=0; lnidx<h2d->geometry().nrLines(); lnidx++ )
    {
	const Pos::GeomID geomid = h2d->geometry().geomID( lnidx );
	linergs.trcrgs += TypeSet<Interval<int> >();
	linergs.zrgs += TypeSet<Interval<float> >();
	for ( int idx=0; idx<seis2dlist.size(); idx++ )
	{
	    Interval<int> trcrg = h2d->geometry().colRange( geomid );
	    trcrg.limitTo( seis2dlist[idx]->getTraceNrRange() );
	    if ( geomid != seis2dlist[idx]->getGeomID() )
	    {
		const Coord sp0 = seis2dlist[idx]->getCoord( trcrg.start );
		const Coord sp1 = seis2dlist[idx]->getCoord( trcrg.stop );
		if ( !trcrg.width() || !sp0.isDefined() || !sp1.isDefined() )
		    continue;

		const Coord hp0 = h2d->getPos( 0, geomid, trcrg.start );
		const Coord hp1 = h2d->getPos( 0, geomid, trcrg.stop );
		if ( !hp0.isDefined() || !hp1.isDefined() )
		    continue;

		const float maxdist =
			(float) ( 0.1 * sp0.distTo(sp1) / trcrg.width() );
		if ( hp0.distTo(sp0)>maxdist || hp1.distTo(sp1)>maxdist )
		    continue;
	    }

	    if ( !seis2dlist[idx]->isPanelShown() ||
		 !seis2dlist[idx]->isAnyAttribEnabled() )
		trcrg.setUdf();

	    linergs.trcrgs[lnidx] += trcrg;
	    linergs.zrgs[lnidx] += seis2dlist[idx]->getZRange( true );
	}
    }

    for ( int sidx=0; sidx<sids_.size(); sidx++ )
	updateSection( sidx, &linergs );

    if ( updateintsectmarkers_.getParam(this) )
	updateIntersectionMarkers( seis2dlist );

    intersectmkset_.getParam(this)->turnOn( displayonlyatsections_ );
}




void Horizon2DDisplay::updateIntersectionMarkers( 
    const ObjectSet<const Seis2DDisplay>& seis2dlist )
{
    intersectmkset_.getParam(this)->clearMarkers();
    calcLine2DInterSectionSet();

    const Line2DInterSectionSet* ln2dset = ln2dset_.getParam(this);
    if ( !ln2dset )
	return;

    mDynamicCastGet( const EM::Horizon2D*, hor2d, emobject_ )
    if ( !hor2d ) return;

    TypeSet<Pos::GeomID> geomids;
    const int nrlns = hor2d->geometry().nrLines();
    for ( int idx=0; idx<nrlns; idx++ )
	geomids += hor2d->geometry().geomID(idx);

    for ( int idx=0; idx<ln2dset->size(); idx++ )
    {
	const Line2DInterSection* intsect = (*ln2dset)[idx];
	if ( !intsect )  continue;

	for ( int idy=0; idy<seis2dlist.size(); idy++ )
	{
	    if ( !shouldDisplayIntersections(*seis2dlist[idy]) )
		continue;

	    if ( intsect->geomID() != seis2dlist[idy]->getGeomID() )
		    continue;
	    for ( int idz=0; idz<geomids.size(); idz++ )
		updateIntersectionPoint( 
		geomids[idz], seis2dlist[idy]->getGeomID(), intsect );
	}

    }

    updateintsectmarkers_.setParam( this, false );
}


void Horizon2DDisplay::updateIntersectionPoint( const Pos::GeomID lngid, 
    const Pos::GeomID seisgid, const Line2DInterSection* intsect )
{
    mDynamicCastGet(EM::Horizon2D*,hor2d,emobject_)
    if ( !hor2d ) return;

    TypeSet<Coord3> intsectpnts;
    for ( int idx=0; idx<intsect->size(); idx++ )
    {
	const Line2DInterSection::Point& intpoint = intsect->getPoint(idx);
	
	if ( lngid != seisgid && intpoint.line != lngid )   
	    continue;

	for ( int idy=0; idy<sids_.size(); idy++ )
	{
	    const int trcnr =
		lngid != seisgid ? intpoint.linetrcnr : intpoint.mytrcnr;
	    const Coord3 crd = hor2d->getPos( sids_[idy], lngid, trcnr );
	    if ( crd.isDefined() )
		intsectpnts += crd;
	}

    }

    if ( intsectpnts.size()==1 )
    {
	visBase::MarkerSet* mkset = intersectmkset_.getParam(this);
	const int mid = mkset->addPos( intsectpnts[0] );
	mkset->getMaterial()->setColor( hor2d->preferredColor(), mid );
	const int sz =
		hor2d->getPosAttrMarkerStyle(EM::EMObject::sSeedNode()).size_;
	mkset->setScreenSize( mCast(float,sz) );
    }
}



bool Horizon2DDisplay::calcLine2DIntersections( 
    const TypeSet<Pos::GeomID>& geom2dids, 
    Line2DInterSectionSet& intsectset )
{
    BendPointFinder2DGeomSet bpfinder( geom2dids );
    bpfinder.execute();
    intsectset.erase();
    Line2DInterSectionFinder intfinder( bpfinder.bendPoints(), intsectset );
    intfinder.execute();

    return intsectset.size()>0;
}


void Horizon2DDisplay::calcLine2DInterSectionSet()
{
    const MultiID mid( IOObjContext::getStdDirData(IOObjContext::Geom)->id_ );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    const bool needcalc = 
	nr2dlines_.getParam(this) !=ioobjs.size() ? true : false;
    nr2dlines_.setParam( this, ioobjs.size() );

    if ( needcalc )
    {
	BufferStringSet lnms;
	TypeSet<Pos::GeomID> geom2dids;
	SeisIOObjInfo::getLinesWithData( lnms, geom2dids );
	Line2DInterSectionSet* ln2dset = ln2dset_.getParam(this);
	if ( ln2dset )
	    delete ln2dset_.getParam(this);	
	ln2dset_.setParam( this, new Line2DInterSectionSet );
	calcLine2DIntersections( geom2dids, *ln2dset_.getParam(this) );
    }
    
}


void Horizon2DDisplay::updateSeedsOnSections(
			const ObjectSet<const Seis2DDisplay>& seis2dlist )
{
    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
    {
	visBase::MarkerSet* markerset = posattribmarkers_[idx];
	if ( !displayonlyatsections_ )
	{
	    markerset->turnAllMarkersOn( true );
	    continue;
	}
	else
	{
	    markerset->turnAllMarkersOn( false );
	}

	for ( int idy=0; idy<markerset->getCoordinates()->size(); idy++ )
	{
	    markerset->turnMarkerOn( idy, !displayonlyatsections_ );
	    const visBase::Coordinates* markercoords =
		markerset->getCoordinates();
	    if ( markercoords->isEmpty() )
		continue;

	    Coord3 markerpos = markercoords->getPos( idy, true );
	    if ( zaxistransform_ )
		markerpos.z = zaxistransform_->transform( markerpos );
	    for ( int idz=0; idz<seis2dlist.size(); idz++ )
	    {
		const Seis2DDisplay* s2dd = seis2dlist[idz];
		const Survey::Geometry* geom2d =
			Survey::GM().getGeometry( s2dd->getGeomID() );
		const float max = geom2d ? geom2d->averageTrcDist()
					 : s2dd->maxDist();
		const float dist = s2dd->calcDist( markerpos );
		if ( dist < max )
		{
		    markerset->turnMarkerOn( idy, true );
		    break;
		}
	    }
	}
    }
}


void Horizon2DDisplay::otherObjectsMoved(
		    const ObjectSet<const SurveyObject>& objs, int movedobj )
{
    if ( burstalertison_ ) return;

    bool refresh = movedobj==-1 || movedobj==id();
    ObjectSet<const Seis2DDisplay> seis2dlist;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet(const Seis2DDisplay*,seis2d,objs[idx]);
	if ( seis2d )
	{
	    seis2dlist += seis2d;
	    if ( movedobj==seis2d->id() )
		refresh = true;
	}
    }

    if ( !refresh ) return;

    updateintsectmarkers_.setParam( this, true);
    updateLinesOnSections( seis2dlist );
    updateSeedsOnSections( seis2dlist );
}


bool Horizon2DDisplay::setEMObject( const EM::ObjectID& newid,
				    TaskRunner* taskr )
{
    if ( !EMObjectDisplay::setEMObject( newid, taskr ) )
	return false;

    getMaterial()->setColor( emobject_->preferredColor() );
    setLineStyle( emobject_->preferredLineStyle() );
    return true;
}


bool Horizon2DDisplay::setZAxisTransform( ZAxisTransform* zat,
					  TaskRunner* taskr )
{
    CallBack cb = mCB(this,Horizon2DDisplay,zAxisTransformChg);
    if ( zaxistransform_ )
    {
	removeVolumesOfInterest();

	if ( zaxistransform_->changeNotifier() )
	    zaxistransform_->changeNotifier()->remove( cb );
	zaxistransform_->unRef();
	zaxistransform_ = 0;
    }

    zaxistransform_ = zat;
    if ( zaxistransform_ )
    {
	zaxistransform_->ref();
	if ( zaxistransform_->changeNotifier() )
	    zaxistransform_->changeNotifier()->notify( cb );
    }

    return true;
}


void Horizon2DDisplay::zAxisTransformChg( CallBacker* )
{
    for ( int sidx=0; sidx<sids_.size(); sidx++ )
	updateSection( sidx );
}


void Horizon2DDisplay::fillPar( IOPar& par ) const
{
    visSurvey::EMObjectDisplay::fillPar( par );
}


bool Horizon2DDisplay::usePar( const IOPar& par )
{
    return visSurvey::EMObjectDisplay::usePar( par );
}


void Horizon2DDisplay::doOtherObjectsMoved(
	            const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    otherObjectsMoved( objs, whichobj );
}


void Horizon2DDisplay::setPixelDensity( float dpi )
{
    EMObjectDisplay::setPixelDensity( dpi );

    for ( int idx =0; idx<lines_.size(); idx++ )
	lines_[idx]->setPixelDensity( dpi );

    for ( int idx=0; idx<points_.size(); idx++ )
	points_[idx]->setPixelDensity( dpi );
}


void Horizon2DDisplay::removeVolumesOfInterest()
{
    if ( !zaxistransform_ ) return;

    for ( int idx=0; idx<volumeofinterestids_.size();idx++ )
	zaxistransform_->removeVolumeOfInterest( volumeofinterestids_[idx] );

    volumeofinterestids_.setAll( -1 );
}

} // namespace visSurvey
