/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2008
-*/


#include "tableposprovider.h"
#include "keystrs.h"
#include "picksetmanager.h"
#include "od_istream.h"
#include "iopar.h"
#include "dbman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "survgeom.h"
#include <math.h>

#define mGetTableKey(k) IOPar::compKey(sKey::Table(),k)


Pos::TableProvider3D::TableProvider3D()
    : Pos::Provider3D()
    , bvs_(1,true)
{}


Pos::TableProvider3D::TableProvider3D( const IOObj& ioobj )
    : Pos::Provider3D()
    , bvs_(1,true)
{
    IOPar iop; iop.set( mGetTableKey("ID"), ioobj.key() );
    usePar( iop );
}


Pos::TableProvider3D::TableProvider3D( const char* fnm )
    : Pos::Provider3D()
    , bvs_(1,true)
{
    IOPar iop; iop.set( mGetTableKey(sKey::FileName()), fnm );
    usePar( iop );
}


Pos::TableProvider3D::TableProvider3D( const Pos::TableProvider3D& oth )
    : Pos::Provider3D(oth)
    , bvs_(1,true)
{
    *this = oth;
}


Pos::TableProvider3D& Pos::TableProvider3D::operator =(
					const TableProvider3D& oth )
{
    if ( &oth == this )
	return *this;

    Pos::Provider3D::operator = ( oth );

    bvs_ = oth.bvs_;
    pos_ = oth.pos_;

    return *this;
}


const char* Pos::TableProvider3D::type() const
{
    return sKey::Table();
}


bool Pos::TableProvider3D::includes( const BinID& bid, float z ) const
{
    BinIDValueSet::SPos pos = bvs_.find( bid );
    if ( !pos.isValid() ) return false;
    if ( mIsUdf(z) ) return true;

    while ( true )
    {
	const float* val = bvs_.getVals( pos );
	if ( !val ) return false;
	const float zdiff = *val - z;
	if ( mIsZero(zdiff,mDefEps) )
	    return true;
	if ( !bvs_.next(pos,false) )
	    break;
	else if ( bvs_.getBinID(pos) != bid )
	    return false;
    }
    return false;
}


void Pos::TableProvider3D::getBVSFromPar( const IOPar& iop, BinIDValueSet& bvs )
{
    const char* res = iop.find( mGetTableKey("ID") );
    if ( res && *res )
    {
	ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch(
					DBKey::getFromString(res) );
	if ( ps )
	{
	    Pick::SetIter psiter( *ps );
	    while ( psiter.next() )
	    {
		const Coord3 crd3 = psiter.get().pos();
		bvs.add( SI().transform(crd3.getXY()), (float)crd3.z_ );
	    }
	}
    }

    if ( bvs.isEmpty() )
    {
	res = iop.find( mGetTableKey(sKey::FileName()) );
	if ( res && *res )
	{
	    od_istream strm( res );
	    if ( strm.isOK() )
		bvs.getFrom( strm, Survey::GM().default3DSurvID() );
	    strm.close();
	    if ( !bvs.isEmpty() )
	    {
		float zfac = -1;
		if ( !SI().zIsTime() )
		    zfac = SI().depthsInFeet() ? mFromFeetFactorF : -1;
		else if ( bvs.nrVals() > 0 )
		{
		    const Interval<float> zrg( bvs.valRange(0) );
		    if ( !mIsUdf(zrg.start) )
		    {
			const Interval<float> sizrg( SI().zRange(false) );
			const float siwdth = sizrg.width();
			if ( zrg.start < sizrg.start - 10 * siwdth
			  || zrg.stop > sizrg.stop + 10 * siwdth )
			    zfac = 0.001;
		    }
		}
		if ( zfac > 0 )
		{
		    BinIDValueSet::SPos p;
		    while ( bvs.next(p) )
		    {
			float* val = bvs.getVals( p );
			if ( !val ) break;
			*val *= zfac;
		    }
		}
	    }
	}
    }

    if ( bvs.isEmpty() )
	bvs.usePar( iop, mGetTableKey("Data") );
}



void Pos::TableProvider3D::usePar( const IOPar& iop )
{
    getBVSFromPar( iop, bvs_ );
}


void Pos::TableProvider3D::fillPar( IOPar& iop ) const
{
    bvs_.fillPar( iop, "Data" );
}


void Pos::TableProvider3D::getSummary( BufferString& txt ) const
{
    const int sz = mCast( int, bvs_.totalSize() );
    if ( sz < 1 ) return;
    txt += sz; txt += " point"; if ( sz > 1 ) txt += "s";
    BinID start, stop;
    getExtent( start, stop );
    txt.add( " at " ).add( start.toString() );
    if ( start != stop )
	txt.add( "-" ).add( stop.toString() );
}


void Pos::TableProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    BinIDValueSet::SPos p; bvs_.next(p);
    if ( !p.isValid() )
	{ start = stop = BinID(0,0); return; }

    start = stop = bvs_.getBinID(p);
    while ( bvs_.next(p) )
    {
	const BinID bid( bvs_.getBinID(p) );
	if ( start.inl() > bid.inl() ) start.inl() = bid.inl();
	if ( stop.inl() < bid.inl() ) stop.inl() = bid.inl();
	if ( start.crl() > bid.crl() ) start.crl() = bid.crl();
	if ( stop.crl() < bid.crl() ) stop.crl() = bid.crl();
    }
}



void Pos::TableProvider3D::getZRange( Interval<float>& zrg ) const
{
    BinIDValueSet::SPos p; bvs_.next(p);
    if ( !p.isValid() )
	{ zrg.start = zrg.stop = 0; return; }

    const float* val = bvs_.getVals( p );
    if ( !val )
	{ zrg = SI().zRange(false); return; }

    zrg.start = zrg.stop = *val;
    while ( bvs_.next(p) )
    {
	const float z = *bvs_.getVals(p);
	if ( zrg.start > z ) zrg.start = z;
	if ( zrg.stop < z ) zrg.stop = z;
    }
}


void Pos::TableProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Table(),
						    tr("Table","data table") );
}
