/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attriboutput.cc,v 1.46 2006-07-05 15:27:49 cvshelene Exp $";

#include "attriboutput.h"
#include "attribdataholder.h"
#include "attribdatacubes.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "seisbuf.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "linekey.h"
#include "scaler.h"
#include "keystrs.h"


namespace Attrib
{

const char* Output::outputstr = "Output";
const char* Output::cubekey = "Cube";
const char* Output::surfkey = "Surface";
const char* Output::tskey = "Trace Selection";
const char* Output::scalekey = "Scale";

const char* SeisTrcStorOutput::seisidkey = "Seismic ID";
const char* SeisTrcStorOutput::attribkey = "Attributes";
const char* SeisTrcStorOutput::inlrangekey = "In-line range";
const char* SeisTrcStorOutput::crlrangekey = "Cross-line range";
const char* SeisTrcStorOutput::depthrangekey = "Depth range";

const char* LocationOutput::filenamekey = "Output Filename";
const char* LocationOutput::locationkey = "Locations";
const char* LocationOutput::attribkey = "Attribute";
const char* LocationOutput::surfidkey = "Surface ID";


Output::Output()
    : seldata_(*new SeisSelData)
{
    mRefCountConstructor;
}


Output::~Output()
{
    delete &seldata_;
}


const LineKey& Output::curLineKey() const
{
    return seldata_.linekey_;
}


DataCubesOutput::DataCubesOutput( const CubeSampling& cs )
    : desiredvolume_(cs)
    , datacubes_(0)
    , udfval_(mUdf(float))
{
}


DataCubesOutput::~DataCubesOutput()
{ if ( datacubes_ ) datacubes_->unRef(); }


bool DataCubesOutput::getDesiredVolume( CubeSampling& cs ) const
{ cs=desiredvolume_; return true; }


bool DataCubesOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume_.hrg.includes(bid); }


TypeSet< Interval<int> > DataCubesOutput::getLocalZRange( const BinID&,
							  float zstep ) const
{
    if ( sampleinterval_.size() ==0 )
    {
	Interval<int> interval( mNINT( desiredvolume_.zrg.start / zstep ),
				mNINT( desiredvolume_.zrg.stop / zstep ) );
	const_cast<DataCubesOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_; 
}


#define mGetSz(dir)\
	dir##sz = (desiredvolume_.hrg.stop.dir - desiredvolume_.hrg.start.dir)\
		  /desiredvolume_.hrg.step.dir + 1;\

#define mGetZSz()\
	zsz = mNINT( ( desiredvolume_.zrg.stop - desiredvolume_.zrg.start )\
	      /refstep + 1 );

void DataCubesOutput::collectData( const DataHolder& data, float refstep, 
				  const SeisTrcInfo& info )
{
    if ( !datacubes_ )
    {
	datacubes_ = new Attrib::DataCubes;
	datacubes_->ref();
	int step = desiredvolume_.hrg.step.inl>SI().inlStep() ? 
		   SI().inlStep() : desiredvolume_.hrg.step.inl;
	datacubes_->inlsampling= StepInterval<int>(desiredvolume_.hrg.start.inl,
						   desiredvolume_.hrg.stop.inl,
						   step);
	step = desiredvolume_.hrg.step.crl>SI().crlStep() ?
	       SI().crlStep() : desiredvolume_.hrg.step.crl;
	datacubes_->crlsampling= StepInterval<int>(desiredvolume_.hrg.start.crl,
						   desiredvolume_.hrg.stop.crl,
						   step);
	datacubes_->z0 = mNINT(desiredvolume_.zrg.start/refstep);
	datacubes_->zstep = refstep;
	int inlsz, crlsz, zsz;
	mGetSz(inl); mGetSz(crl); mGetZSz();
	datacubes_->setSize( inlsz, crlsz, zsz );
    }
		
    const int totalnrcubes = desoutputs_.size();
    while ( datacubes_->nrCubes() < totalnrcubes )
	datacubes_->addCube(mUdf(float));

    if ( !datacubes_->includes(info.binid) )
	return;
    
    int zsz; 
    mGetZSz();
    Interval<int> dataidxrg( data.z0_, data.z0_+data.nrsamples_ - 1 );
    for ( int desout=0; desout<desoutputs_.size(); desout++ )
    {
	for ( int idx=0; idx<zsz; idx++)
	{
	    const int dataidx = datacubes_->z0 + idx;
	    float val = udfval_;
	    if (dataidxrg.includes(dataidx) && data.series(desoutputs_[desout]))
		val = data.series(desoutputs_[desout])->value(dataidx-data.z0_);

	    const int inlidx =
		datacubes_->inlsampling.nearestIndex(info.binid.inl);
	    const int crlidx =
		datacubes_->crlsampling.nearestIndex(info.binid.crl);

	    if ( mIsUdf(val) ) continue;
	    datacubes_->setValue( desout, inlidx, crlidx, idx, val);
	}
    }
}


const DataCubes* DataCubesOutput::getDataCubes() const
{
    return datacubes_;
}


void DataCubesOutput::setGeometry( const CubeSampling& cs )
{
    if ( cs.isEmpty() ) return;
    seldata_.copyFrom(cs);
}


SeisTrcStorOutput::SeisTrcStorOutput( const CubeSampling& cs,
				      const LineKey& lk )
    : desiredvolume_(cs)
    , auxpars_(0)
    , storid_(*new MultiID)
    , writer_(0)
    , trc_(0)
    , prevpos_(-1,-1)
    , storinited_(0)
    , errmsg_(0)
    , scaler_(0)
{
    seldata_.linekey_ = lk;
    attribname_ = lk.attrName();
}


bool SeisTrcStorOutput::getDesiredVolume( CubeSampling& cs ) const
{
    cs = desiredvolume_;
    return true;
}


bool SeisTrcStorOutput::wantsOutput( const BinID& bid ) const
{
    return desiredvolume_.hrg.includes(bid);
}


bool SeisTrcStorOutput::setStorageID( const MultiID& storid )
{
    if ( *((const char*)storid) )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid );
	if ( !ioseisout )
	{
	    errmsg_ = "Cannot find seismic data with ID: "; errmsg_ += storid;
	    return false;
	}
    }

    storid_ = storid;
    return true;
}


void SeisTrcStorOutput::setGeometry( const CubeSampling& cs )
{
    if ( cs.isEmpty() ) return;
    seldata_.copyFrom(cs);
}


SeisTrcStorOutput::~SeisTrcStorOutput()
{
    delete writer_;
    delete &storid_;
    delete auxpars_;
    delete scaler_;
}


bool SeisTrcStorOutput::doUsePar( const IOPar& pars )
{
    errmsg_ = "";
    PtrMan<IOPar> outppar = pars.subselect("Output.1");
    const char* storid = outppar->find("Seismic ID");
    if ( !setStorageID( storid ) )
    {
        errmsg_ = "Could not find output ID: "; errmsg_ += storid;
        return false;
    }

    const char* res = outppar->find( scalekey );
    if ( res )
    {
	scaler_ = new LinScaler;
	scaler_->fromString( res );
	if ( scaler_->isEmpty() )
	    { delete scaler_; scaler_ = 0; }
    }
    
    auxpars_ = pars.subselect("Aux");
    doInit();
    return true;
}//warning, only a small part of the old taken, see if some more is required


bool SeisTrcStorOutput::doInit()
{
    if ( *storid_.buf() )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid_ );
	if ( !ioseisout )
	{
	    errmsg_ = "Cannot find seismic data with ID: "; errmsg_ += storid_;
	    return false;
	}

	writer_ = new SeisTrcWriter( ioseisout );
	is2d_ = writer_->is2D();
	if ( auxpars_ )
	{
	    writer_->lineAuxPars().merge( *auxpars_ );
	    delete auxpars_; auxpars_ = 0;
	}
    }

    desiredvolume_.normalise();
    seldata_.linekey_.setAttrName( "" );
    if ( seldata_.type_ != Seis::Range )
	seldata_.type_ = Seis::Range;

    if ( !is2d_ )
    {
	if ( seldata_.inlrg_.start > desiredvolume_.hrg.start.inl )
	    desiredvolume_.hrg.start.inl = seldata_.inlrg_.start;
	if ( seldata_.inlrg_.stop < desiredvolume_.hrg.stop.inl )
	    desiredvolume_.hrg.stop.inl = seldata_.inlrg_.stop;
	if ( seldata_.crlrg_.start > desiredvolume_.hrg.start.crl )
	    desiredvolume_.hrg.start.crl = seldata_.crlrg_.start;
	if ( seldata_.crlrg_.stop < desiredvolume_.hrg.stop.crl )
	    desiredvolume_.hrg.stop.crl = seldata_.crlrg_.stop;
	if ( seldata_.zrg_.start > desiredvolume_.zrg.start )
	    desiredvolume_.zrg.start = seldata_.zrg_.start;
	if ( seldata_.zrg_.stop < desiredvolume_.zrg.stop )
	    desiredvolume_.zrg.stop = seldata_.zrg_.stop;
    }

    return true;
}


class COLineKeyProvider : public LineKeyProvider
{
public:

COLineKeyProvider( SeisTrcStorOutput& c, const char* a, const char* lk )
	: co_(c) , attrnm_(a) , linename_(lk) {}

LineKey lineKey() const
{
    LineKey lk(linename_,attrnm_);
    return lk;
}
    SeisTrcStorOutput&   co_;
    BufferString        attrnm_;
    BufferString 	linename_;

};


void SeisTrcStorOutput::collectData( const DataHolder& data, float refstep, 
				     const SeisTrcInfo& info )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs_.size())
	return;

    const int sz = data.nrsamples_;
    DataCharacteristics dc;

    if ( !trc_ )
    {
	trc_ = new SeisTrc( sz, dc );
	trc_->info() = info;
	trc_->info().sampling.step = refstep;
	trc_->info().sampling.start = data.z0_*refstep;
	for ( int idx=1; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( sz, dc, false );
    }
    else if ( trc_->info().binid != info.binid )
    {
	errmsg_ = "merge components of two different traces!";
	return;	    
    }
    else
    {
	for ( int idx=0; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( sz, dc, false );
    }

    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    float val = data.series(desoutputs_[comp])->value(idx);
	    trc_->set(idx, val, comp);
	}
    }
    
    if ( scaler_ )
    {
	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		float val = trc_->get( idx, icomp );
		val = scaler_->scale( val );
		trc_->set( idx, val, icomp );
	    }
	}
    }
}


void SeisTrcStorOutput::writeTrc()
{
    if ( !trc_ ) return;
    
    if ( !storinited_ )
    {
	if ( writer_->is2D() )
	{
	    if ( attribname_ == "inl_dip" || attribname_ == "crl_dip" )
		attribname_ = sKey::Steering;
	    else if ( IOObj::isKey(attribname_) )
		attribname_ = IOM().nameOf(attribname_);

	    writer_->setLineKeyProvider( 
		new COLineKeyProvider( *this, attribname_, 
		    		       curLineKey().lineName()) );
	}

	if ( !writer_->prepareWork(*trc_) )
	    { errmsg_ = writer_->errMsg(); return; }

	SeisTrcTranslator* transl = writer_->seisTranslator();
	if ( transl && !writer_->is2D() )
	{
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& cis
		             = transl->componentInfo();
	    for ( int idx=0; idx<cis.size(); idx++ )
		cis[idx]->datatype = outptypes_.size() ? outptypes_[idx] : 
		    					Seis::UnknowData;
	}

	storinited_ = true;
    }
    
    if ( !writer_->put(*trc_) )
	{ errmsg_ = writer_->errMsg(); }

    delete trc_;
    trc_ = 0;
}


TypeSet< Interval<int> > SeisTrcStorOutput::getLocalZRange( const BinID& bid,
							    float zstep ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	Interval<int> interval( mNINT(desiredvolume_.zrg.start/zstep), 
				mNINT(desiredvolume_.zrg.stop/zstep) );
	const_cast<SeisTrcStorOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


TwoDOutput::TwoDOutput( const Interval<int>& trg, const Interval<float>& zrg,
			const LineKey& lk )
    : errmsg_(0)
    , output_( 0 )
{
    seldata_.linekey_ = lk;
    setGeometry( trg, zrg );
}


TwoDOutput::~TwoDOutput()
{
    if ( output_ ) output_->unRef();
}


bool TwoDOutput::wantsOutput( const BinID& bid ) const
{
    return seldata_.crlrg_.includes(bid.crl);
} 
 

void TwoDOutput::setGeometry( const Interval<int>& trg,
			      const Interval<float>& zrg )
{
    seldata_.zrg_ = zrg;
    assign( seldata_.crlrg_, trg );
    seldata_.type_ = Seis::Range;
}


bool TwoDOutput::getDesiredVolume( CubeSampling& cs ) const
{
    cs.hrg.start.crl = seldata_.crlrg_.start;
    cs.hrg.stop.crl = seldata_.crlrg_.stop;
    cs.zrg = StepInterval<float>( seldata_.zrg_.start, seldata_.zrg_.stop,
	    			  SI().zStep() );
    cs.hrg.start.inl = cs.hrg.stop.inl = 0;
    return true;
}


bool TwoDOutput::doInit()
{
    seldata_.linekey_.setAttrName( "" );
    if ( seldata_.crlrg_.start <= 0 && Values::isUdf(seldata_.crlrg_.stop) )
	seldata_.type_ = Seis::All;

    return true;
}


void TwoDOutput::collectData( const DataHolder& data, float refstep,
			      const SeisTrcInfo& info )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs_.size() )
	return;

    if ( !output_ ) return;

    output_->dataset_ += data.clone();

    SeisTrcInfo* trcinfo = new SeisTrcInfo(info);
    trcinfo->sampling.step = refstep;
    trcinfo->sampling.start = data.z0_*refstep;
    output_->trcinfoset_ += trcinfo;
}


void TwoDOutput::setOutput( Data2DHolder& no )
{
    if ( output_ ) output_->unRef();
    output_ = &no;
    output_->ref();
}


TypeSet< Interval<int> > TwoDOutput::getLocalZRange( const BinID& bid,
						     float zstep ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	Interval<int> interval( mNINT(seldata_.zrg_.start/zstep), 
				mNINT(seldata_.zrg_.stop/zstep) );
	const_cast<TwoDOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


LocationOutput::LocationOutput( BinIDValueSet& bidvalset )
    : bidvalset_(bidvalset)
{
    seldata_.all_ = false;
    seldata_.type_ = Seis::Table;
    seldata_.table_.allowDuplicateBids( true );
    seldata_.table_ = bidvalset;
}


void LocationOutput::collectData( const DataHolder& data, float refstep,
				  const SeisTrcInfo& info )
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( info.binid );
    if ( !pos.valid() ) return;

    const int desnrvals = desoutputs_.size()+1;
    if ( bidvalset_.nrVals() < desnrvals )
	bidvalset_.setNrVals( desnrvals );

    while ( true )
    {
	float* vals = bidvalset_.getVals( pos );
	const int zidx = mNINT(vals[0]/refstep);
	if ( data.z0_ == zidx )
	{
	    for ( int comp=0; comp<desoutputs_.size(); comp++ )
		vals[comp+1] = data.series(desoutputs_[comp])->value(0);
	}

	bidvalset_.next( pos );
	if ( info.binid != bidvalset_.getBinID(pos) )
	    break;
    }
}


bool LocationOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    return pos.valid();
}


TypeSet< Interval<int> > LocationOutput::getLocalZRange( const BinID& bid,
							 float zstep ) const
{
    TypeSet< Interval<int> > sampleinterval;

    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    while ( pos.valid() )
    {
	const float* vals = bidvalset_.getVals( pos );
	Interval<int> interval( mNINT(vals[0]/zstep), mNINT(vals[0]/zstep) );
	sampleinterval += interval;
	bidvalset_.next( pos );
	if ( bid != bidvalset_.getBinID(pos) )
	    break;
    }

    return sampleinterval;
}


TrcSelectionOutput::TrcSelectionOutput( const BinIDValueSet& bidvalset,
					float outval )
    : bidvalset_(bidvalset)
    , outpbuf_(0)
    , outval_(outval)
{
    seldata_.all_ = false;
    seldata_.type_ = Seis::Table;
    seldata_.table_.allowDuplicateBids( bidvalset.totalSize()<2 );
    seldata_.table_.setNrVals( 1 );

    const int nrinterv = bidvalset.nrVals()/2;
    float zmin = mUdf(float);
    float zmax = -mUdf(float);
    for ( int idx=0; idx<nrinterv; idx+=2 )
    {
	float val = bidvalset.valRange(idx).start;
	if ( val < zmin ) zmin = val;
	val = bidvalset.valRange(idx+1).stop;
	if ( val > zmax ) zmax = val;
    }

    BinIDValueSet::Pos pos;
    bidvalset.next( pos );
    seldata_.table_.add( bidvalset.getBinID(pos), zmin );
    while ( bidvalset.next(pos) )
	seldata_.table_.add( bidvalset.getBinID(pos), zmax );

    stdtrcsz_ = zmax - zmin;
    stdstarttime_ = zmin;
}


TrcSelectionOutput::~TrcSelectionOutput()
{}


void TrcSelectionOutput::collectData( const DataHolder& data, float refstep,
				      const SeisTrcInfo& info )
{
    const int nrcomp = data.nrSeries();
    if ( !outpbuf_ || !nrcomp || nrcomp < desoutputs_.size() )
	return;

    const int trcsz = mNINT(stdtrcsz_/refstep) + 1;
    const float globalsttime = stdstarttime_;
    const float trcstarttime = ( (int)(globalsttime/refstep) +1 ) * refstep;
    const int startidx = data.z0_ - mNINT(trcstarttime/refstep);
    const int index = outpbuf_->find( info.binid );

    SeisTrc* trc;
    if ( index == -1 )
    {
	const DataCharacteristics dc;
	trc = new SeisTrc( trcsz, dc );
	for ( int idx=trc->data().nrComponents(); idx<desoutputs_.size(); idx++)
	    trc->data().addComponent( trcsz, dc, false );

	trc->info() = info;
	trc->info().sampling.start = trcstarttime;
	trc->info().sampling.step = refstep;
    }
    else
	trc = outpbuf_->get( index );

    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	for ( int idx=0; idx<trcsz; idx++ )
	{
	    if ( idx < startidx || idx>=startidx+data.nrsamples_ )
		trc->set( idx, outval_, comp );
	    else  
	    {
		const float val = 
		    data.series(desoutputs_[comp])->value(idx-startidx);
		trc->set( idx, val, comp );
	    }
	}
    }

    if ( index == -1 )
	outpbuf_->add( trc );
}


bool TrcSelectionOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    return pos.valid();
}


void TrcSelectionOutput::setOutput( SeisTrcBuf* outp_ )
{
    outpbuf_ = outp_;
    if ( outpbuf_ )
	outpbuf_->erase();
}


void TrcSelectionOutput::setTrcsBounds( Interval<float> intv )
{
    stdstarttime_ = intv.start;
    stdtrcsz_ = intv.stop - intv.start;
}


TypeSet< Interval<int> > TrcSelectionOutput::getLocalZRange( const BinID& bid,
							     float zstep ) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    BinID binid;
    TypeSet<float> values;
    bidvalset_.get( pos, binid, values );
    TypeSet< Interval<int> > sampleinterval;
    for ( int idx=0; idx<values.size()/2; idx+=2 )
    {
	Interval<int> interval( mNINT(values[idx]/zstep), 
				mNINT(values[idx+1]/zstep) );
	sampleinterval += interval;
    }
 
    return sampleinterval;
}

} // namespace Attrib
