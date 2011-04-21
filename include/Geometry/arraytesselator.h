#ifndef arraytesselator_h
#define arraytesselator_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          April 2011
RCS:           $Id: arraytesselator.h,v 1.3 2011-04-21 14:28:13 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "arraynd.h"


template <class T> class Array2D;

namespace Geometry
{

/*!Class to tesselate part of an array2D data, rrg/crg are given to set the 
  start, step and size of the tesselation. */

mClass ArrayTesselator : public ParallelTask
{
public:
    			ArrayTesselator(const float* data,
					int datarowsize,int datacolsize,
					const StepInterval<int>& rrg,
					const StepInterval<int>& crg);

    			ArrayTesselator(const Array2D<float>& data,
					const StepInterval<int>& rrg,
					const StepInterval<int>& crg);

    od_int64		nrIterations() const	
			{ return rowrange_.nrSteps()*colrange_.nrSteps(); }
    
    			/*<s= 0 for points, 1 for lines, 2 for triangles.*/
    TypeSet<int>	getIndices(char s=2) const
    			{ return s==2 ? stripcis_ : (s ? linecis_ : pointcis_);}
    
    virtual int		getCoordIndex(int row,int col )	{ return 0; }

protected:

    bool			doWork(od_int64 start,od_int64 stop,int);

    const float*		data_;
    int				datarowsize_;
    int				datacolsize_;
    const StepInterval<int>& 	rowrange_;
    const StepInterval<int>& 	colrange_;
   
    TypeSet<int>		pointcis_;
    TypeSet<int>		linecis_;
    TypeSet<int>		stripcis_;
};


ArrayTesselator::ArrayTesselator( const float* data, int rowsz, int colsz, 
	const StepInterval<int>& rrg, const StepInterval<int>& crg )
    : data_( data )
    , datarowsize_( rowsz )
    , datacolsize_( colsz )
    , rowrange_( rrg )
    , colrange_( crg )
{}		      


ArrayTesselator::ArrayTesselator( const Array2D<float>& data, 
	const StepInterval<int>& rrg, const StepInterval<int>& crg )
    : data_( data.getData() )
    , datarowsize_( data.info().getSize(0) )
    , datacolsize_( data.info().getSize(1) )
    , rowrange_( rrg )
    , colrange_( crg )
{}		      



#define mGlobleIdx(row,col) row*datacolsize_+col 

#define mAddTriangle( ci0, ci1, ci2 ) \
stripcis_ += ci0; \
stripcis_ += ci1; \
stripcis_ += ci2


bool ArrayTesselator::doWork( od_int64 start, od_int64 stop, int )
{
    const int glastrowidx = datarowsize_ - 1;
    const int glastcolidx = datacolsize_ - 1;
    const int colsz = colrange_.nrSteps();
    const int startidx = rowrange_.start * datacolsize_ + colrange_.start;

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int currow = idx / colsz + rowrange_.start;
	const int curcol = idx % colsz + colrange_.start;
	if ( currow > glastrowidx || curcol > glastcolidx )
	    continue;

	const bool islastrow = currow == glastrowidx;
	const bool islastcol = curcol == glastcolidx;
	const int nextrow = currow + datacolsize_;
	const int nextcol = curcol + 1;

	const int c11 = mGlobleIdx( currow, curcol );
	const int c12 = mGlobleIdx( currow, nextcol );
	const int c21 = mGlobleIdx( nextrow, curcol );
	const int c22 = mGlobleIdx( nextrow, nextcol );

	bool def11 = !mIsUdf( data_[c11] );
	bool def12 = islastcol ? false : !mIsUdf( data_[c12] );
	bool def21 = islastrow ? false : !mIsUdf( data_[c21] );
	bool def22 = (islastrow || islastcol) ? false : !mIsUdf( data_[c22] );
	const int nrdefined = def11 + def12 + def21 + def22;
	if ( !nrdefined )
	    continue;

	if ( nrdefined>2 )
	{
    	    if ( nrdefined==4 )
    	    {
    		mAddTriangle( c21, c22, c11 );
    		stripcis_ += c12;
	    }
	    else if ( !def11 )
	    {
		mAddTriangle( c21, c22, c12 );
	    }
	    else if ( !def12 )
	    {
		mAddTriangle( c21, c22, c11 );
	    }
	    else if ( !def21 )
	    {
		mAddTriangle( c11, c22, c12 );
	    }
	    else
	    {
		mAddTriangle( c11, c21, c12 );
	    }

	    stripcis_ += -1;
	}
	else if ( def11 )
	{
	    const int prerow = currow - datacolsize_;
	    const int precol = curcol - 1;

	    const int c01 = mGlobleIdx( prerow, curcol );
    	    const int c10 = mGlobleIdx( currow, precol );
	    bool def01 = prerow<0 ? false : !mIsUdf( data_[c01] );
	    bool def10 = precol<0 ? false : !mIsUdf( data_[c10] );
	    if ( nrdefined==1 )
	    {
		if ( !def01 && !def10 )
		    pointcis_ += c11;
	    }
	    else
	    {
		if ( def12 && !def01 )
		{
		    const int c02 = mGlobleIdx( prerow, nextcol );
		    bool def02 = prerow<0 || islastcol ? false 
						       : !mIsUdf( data_[c02] );
		    if ( !def02 )
		    {
			linecis_ += c11;
			linecis_ += c12;
			linecis_ += -1;
		    }
		}
		else if ( def21 && !def10 )
		{
		    const int c20 = mGlobleIdx( nextrow, precol );
		    bool def20 = islastrow || precol<0 ? false 
						       : !mIsUdf( data_[c20] );
		    if ( !def20 )
		    {
			linecis_ += c11;
			linecis_ += c21;
			linecis_ += -1;
		    }
		}
	    }
	}
    }

    return true;
} 


};

#endif
