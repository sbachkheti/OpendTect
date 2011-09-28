/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2011
-*/


#include "fourierinterpol.h"

#include "arrayndinfo.h"
#include "fourier.h"



FourierInterpol1D::FourierInterpol1D( const TypeSet<Point>& pts,
			    const StepInterval<float>& sampling)
    : fft_( new Fourier::CC )
    , sampling_(sampling)
    , pts_(pts)
{
    sz_ = sampling_.nrSteps()+1;
}


FourierInterpol1D::~FourierInterpol1D()
{
    deepErase( arrs_ );
}


bool FourierInterpol1D::doPrepare( int nrthreads )
{
    for ( int idthread=0; idthread<nrthreads; idthread++ )
	arrs_ += new Array1DImpl<float_complex>( sz_ );

    return true;
}


bool FourierInterpol1D::doWork( od_int64 start ,od_int64 stop, int thread )
{
    if ( !sz_ )
	return false;

    const float df = Fourier::CC::getDf( sampling_.step, sz_ );
    const float nyqfreq = Fourier::CC::getNyqvist( sampling_.step  );

    Array1D<float_complex>& interpvals = *arrs_[thread];

    for ( int idpt=start; idpt<=stop; idpt++ )
    {
	float_complex cplxval = pts_[idpt].val_;
	if ( mIsUdf( cplxval ) ) 
	    cplxval = float_complex( 0, 0 );

	const float time = pts_[idpt].pos_; 
	const float anglesampling = -time * df;

	for ( int idx=0; idx<sz_; idx++ )
	{
	    const float angle = 2*M_PI *anglesampling*idx;
	    const float freq = df * idx;
	    const float_complex cexp = float_complex( cos(angle), sin(angle) );
	    const float_complex cplxref = cexp*cplxval;
	    float_complex outpval = interpvals.get( idx );
	    outpval += cplxref; 
	    interpvals.set( idx, outpval );
	}
    }
    return true;
}


bool FourierInterpol1D::doFinish( bool success )
{
    if ( !success || arrs_.isEmpty() )
	return false;

    while ( arrs_.size() > 1 )
    {
	Array1D<float_complex>& arr = *arrs_.remove(1);
	for ( int idx=0; idx<sz_; idx++ )
	{
	    float_complex val = arrs_[0]->get( idx );
	    val += arr.get( idx );
	    arrs_[0]->set( idx, val );
	}
	delete &arr;
    }

    fft_->setInputInfo( Array1DInfoImpl(sz_ ) );
    fft_->setDir( false );
    fft_->setNormalization( true );
    fft_->setInput( arrs_[0]->getData() );
    fft_->setOutput( arrs_[0]->getData() );
    fft_->run( true );

    return true;
}




FourierInterpol2D::FourierInterpol2D( const TypeSet<Point>& pts,
			    const StepInterval<float>& xsampling,
			    const StepInterval<float>& ysampling )
    : fft_( new Fourier::CC )
    , xsampling_(xsampling)
    , ysampling_(ysampling)
    , pts_(pts)
{
    szx_ = xsampling_.nrSteps()+1;
    szy_ = ysampling_.nrSteps()+1;
}


FourierInterpol2D::~FourierInterpol2D()
{
    deepErase( arrs_ );
}


bool FourierInterpol2D::doPrepare( int nrthreads )
{
    for ( int idthread=0; idthread<nrthreads; idthread++ )
	arrs_ += new Array2DImpl<float_complex>( szx_, szy_ );

    return true;
}


bool FourierInterpol2D::doWork( od_int64 start ,od_int64 stop, int thread )
{
    if ( !szx_  || !szy_ )
	return false;

    const float dfx = Fourier::CC::getDf( xsampling_.step, szx_ );
    const float dfy = Fourier::CC::getDf( ysampling_.step, szy_ );

    const float nyqxfreq = Fourier::CC::getNyqvist( xsampling_.step  );
    const float nyqyfreq = Fourier::CC::getNyqvist( ysampling_.step );

    Array2D<float_complex>& interpvals = *arrs_[thread];

    for ( int idpt=start; idpt<=stop; idpt++ )
    {
	float_complex cplxval = pts_[idpt].val_;
	if ( mIsUdf( cplxval ) ) 
	    cplxval = float_complex( 0, 0 );

	const float timex = pts_[idpt].xpos_; 
	const float timey = pts_[idpt].ypos_; 

	const float xanglesampling = -timex * dfx;
	const float yanglesampling = -timey * dfy;

	for ( int idx=0; idx<szx_; idx++ )
	{
	    const float anglex = 2*M_PI *xanglesampling*idx;
	    const float freqx = dfx * idx;
	    const float_complex cxexp = float_complex(cos(anglex),sin(anglex));

	    for ( int idy=0; idy<szy_; idy++ )
	    {
		const float angley = 2*M_PI *yanglesampling*idy;
		const float freqy = dfy * idy;
		const float_complex cyexp = float_complex( cos(angley), 
							    sin(angley) );
		const float_complex cplxref = cxexp*cyexp*cplxval;
		float_complex outpval = interpvals.get( idx, idy );
		outpval += cplxref;
		interpvals.set( idx, idy, outpval );
	    }
	}
    }
    return true;
}


bool FourierInterpol2D::doFinish( bool success )
{
    if ( !success || arrs_.isEmpty() )
	return false;

    while ( arrs_.size() > 1 )
    {
	Array2D<float_complex>& arr = *arrs_.remove(1);
	for ( int idx=0; idx<szx_; idx++ )
	{
	    for ( int idy=0; idy<szy_; idy++ )
	    {
		float_complex val = arrs_[0]->get( idx, idy );
		val += arr.get( idx, idy );
		arrs_[0]->set( idx, idy, val );
	    }
	}
	delete &arr;
    }

    fft_->setInputInfo( Array2DInfoImpl(szx_,szy_) );
    fft_->setDir( false );
    fft_->setNormalization( true );
    fft_->setInput( arrs_[0]->getData() );
    fft_->setOutput( arrs_[0]->getData() );
    fft_->run( true );

    return true;
}




FourierInterpol3D::FourierInterpol3D( const TypeSet<Point>& pts,
			    const StepInterval<float>& xsampling,
			    const StepInterval<float>& ysampling,
			    const StepInterval<float>& zsampling )
    : fft_( new Fourier::CC )
    , xsampling_(xsampling)
    , ysampling_(ysampling)
    , zsampling_(zsampling)
    , pts_(pts)
{
    szx_ = xsampling_.nrSteps()+1;
    szy_ = ysampling_.nrSteps()+1;
    szz_ = zsampling_.nrSteps()+1;
}


FourierInterpol3D::~FourierInterpol3D()
{
    deepErase( arrs_ );
}


bool FourierInterpol3D::doPrepare( int nrthreads )
{
    for ( int idthread=0; idthread<nrthreads; idthread++ )
	arrs_ += new Array3DImpl<float_complex>( szx_, szy_, szz_ );

    return true;
}


bool FourierInterpol3D::doWork( od_int64 start ,od_int64 stop, int thread )
{
    if ( !szx_  || !szy_ || !szz_ )
	return false;

    const float dfx = Fourier::CC::getDf( xsampling_.step, szx_ );
    const float dfy = Fourier::CC::getDf( ysampling_.step, szy_ );
    const float dfz = Fourier::CC::getDf( zsampling_.step, szz_ );

    const float nyqxfreq = Fourier::CC::getNyqvist( xsampling_.step  );
    const float nyqyfreq = Fourier::CC::getNyqvist( ysampling_.step );
    const float nyqzfreq = Fourier::CC::getNyqvist( zsampling_.step );

    Array3DImpl<float_complex>& interpvals = *arrs_[thread];

    for ( int idpt=start; idpt<=stop; idpt++ )
    {
	float_complex cplxval = pts_[idpt].val_;
	if ( mIsUdf( cplxval ) ) 
	    cplxval = float_complex( 0, 0 );

	const float timex = pts_[idpt].xpos_; 
	const float timey = pts_[idpt].ypos_; 
	const float timez = pts_[idpt].zpos_;

	const float xanglesampling = -timex * dfx;
	const float yanglesampling = -timey * dfy;
	const float zanglesampling = -timez * dfz;

	for ( int idx=0; idx<szx_; idx++ )
	{
	    const float anglex = 2*M_PI *xanglesampling*idx;
	    const float freqx = dfx * idx;
	    const float_complex cxexp = float_complex(cos(anglex),sin(anglex));

	    for ( int idy=0; idy<szy_; idy++ )
	    {
		const float angley = 2*M_PI *yanglesampling*idy;
		const float freqy = dfy * idy;
		const float_complex cyexp = float_complex( cos(angley), 
							    sin(angley) );
		for ( int idz=0; idz<szz_; idz++ )
		{
		    const float anglez = 2*M_PI *zanglesampling*idz;
		    const float freqz = dfz * idz;
		    const float_complex czexp = float_complex( cos(anglez), 
			    					sin(anglez) );

		    const float_complex cplxref = cxexp*cyexp*czexp*cplxval;

		    float_complex outpval = interpvals.get( idx, idy, idz );
		    outpval += cplxref; //freqz > nyqzfreq ? 0 : cplxref; 
		    interpvals.set( idx, idy, idz, outpval );
		}
	    }
	}
    }
    return true;
}


bool FourierInterpol3D::doFinish( bool success )
{
    if ( !success || arrs_.isEmpty() )
	return false;

    while ( arrs_.size() > 1 )
    {
	Array3DImpl<float_complex>& arr = *arrs_.remove(1);
	for ( int idx=0; idx<szx_; idx++ )
	{
	    for ( int idy=0; idy<szy_; idy++ )
	    {
		for ( int idz=0; idz<szz_; idz++ )
		{
		    float_complex val = arrs_[0]->get( idx, idy, idz );
		    val += arr.get( idx, idy, idz );
		    arrs_[0]->set( idx, idy, idz, val );
		}
	    }
	}
	delete &arr;
    }

    fft_->setInputInfo( Array3DInfoImpl(szx_,szy_,szz_) );
    fft_->setDir( false );
    fft_->setNormalization( true );
    fft_->setInput( arrs_[0]->getData() );
    fft_->setOutput( arrs_[0]->getData() );
    fft_->run( true );

    return true;
}
