/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segyfiledef.cc,v 1.4 2008-10-02 14:39:49 cvsbert Exp $";

#include "segyfiledef.h"
#include "segytr.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "separstr.h"
#include "survinfo.h"
#include "datapointset.h"

const char* SEGY::FileSpec::sKeyFileNrs = "File numbers";


const char* SEGY::FileSpec::getFileName( int nr ) const
{
    if ( !isMultiFile() )
	return fname_.buf();

    BufferString numbstr( nrs_.atIndex(nr) );
    BufferString replstr;
    if ( zeropad_ < 2 )
	replstr = numbstr;
    else
    {
	const int numblen = numbstr.size();
	while ( numblen + replstr.size() < zeropad_ )
	    replstr += "0";
	replstr += numbstr;
    }

    static FileNameString ret( fname_ );
    replaceString( ret.buf(), "*", replstr.buf() );
    return ret.buf();
}


IOObj* SEGY::FileSpec::getIOObj( bool tmp ) const
{
    IOStream* iostrm;
    if ( tmp )
    {
	MultiID tmpid( "100010." ); tmpid += BufferString(IOObj::tmpID);
	iostrm = new IOStream( fname_, tmpid.buf() );
    }
    else
    {
	iostrm = new IOStream( fname_ );
	iostrm->acquireNewKey();
    }
    iostrm->setFileName( fname_ );
    iostrm->setGroup( "Seismic Data" );
    iostrm->setTranslator( "SEG-Y" );
    const bool ismulti = !mIsUdf(nrs_.start);
    if ( ismulti )
    {   
	iostrm->fileNumbers() = nrs_;
	iostrm->setZeroPadding( zeropad_ );
    }

    return iostrm;
}


void SEGY::FileSpec::fillPar( IOPar& iop ) const
{
    iop.set( sKey::FileName, fname_ );
    if ( mIsUdf(nrs_.start) )
	iop.removeWithKey( sKeyFileNrs );
    else
    {
	FileMultiString fms;
	fms += nrs_.start; fms += nrs_.stop; fms += nrs_.step;
	if ( zeropad_ )
	    fms += zeropad_;
	iop.set( sKeyFileNrs, fms );
    }
}


void SEGY::FileSpec::usePar( const IOPar& iop )
{
    iop.get( sKey::FileName, fname_ );
    getMultiFromString( iop.find(sKeyFileNrs) );
}


void SEGY::FileSpec::getReport( IOPar& iop ) const
{
    iop.set( sKey::FileName, fname_ );
    if ( mIsUdf(nrs_.start) ) return;

    BufferString str;
    str += nrs_.start; str += "-"; str += nrs_.stop;
    str += " step "; str += nrs_.step;
    if ( zeropad_ )
	{ str += "(pad to "; str += zeropad_; str += " zeros)"; }
    iop.set( "Replace '*' with", str );
}


void SEGY::FileSpec::getMultiFromString( const char* str )
{
    FileMultiString fms( str );
    const int len = fms.size();
    nrs_.start = len > 0 ? atoi( fms[0] ) : mUdf(int);
    if ( len > 1 )
	nrs_.stop = atoi( fms[1] );
    if ( len > 2 )
	nrs_.step = atoi( fms[2] );
    if ( len > 3 )
	zeropad_ = atoi( fms[3] );
}


void SEGY::FileSpec::ensureWellDefined( IOObj& ioobj )
{
    mDynamicCastGet(IOStream*,iostrm,&ioobj)
    if ( !iostrm ) return;
    iostrm->setTranslator( "SEG-Y" );
    IOPar& iop = ioobj.pars();
    if ( !iop.find( sKey::FileName ) ) return;

    SEGY::FileSpec fs; fs.usePar( iop );
    iop.removeWithKey( sKey::FileName );
    iop.removeWithKey( sKeyFileNrs );

    iostrm->setFileName( fs.fname_ );
    if ( !fs.isMultiFile() )
	iostrm->fileNumbers().start = iostrm->fileNumbers().stop = 1;
    else
    {
	iostrm->fileNumbers() = fs.nrs_;
	iostrm->setZeroPadding( fs.zeropad_ );
    }
}


void SEGY::FileSpec::fillParFromIOObj( const IOObj& ioobj, IOPar& iop )
{
    mDynamicCastGet(const IOStream*,iostrm,&ioobj)
    if ( !iostrm ) return;

    SEGY::FileSpec fs; fs.fname_ = iostrm->fileName();
    if ( iostrm->isMulti() )
    {
	fs.nrs_ = iostrm->fileNumbers();
	fs.zeropad_ = iostrm->zeroPadding();
    }

    fs.fillPar( iop );
}


const char** SEGY::FilePars::getFmts( bool fr )
{
    return SEGYSeisTrcTranslator::getFmts( fr );
}


void SEGY::FilePars::fillPar( IOPar& iop ) const
{
    iop.set( SEGYSeisTrcTranslator::sExternalNrSamples, ns_ );
    iop.set( SEGYSeisTrcTranslator::sNumberFormat, nameOfFmt(fmt_,forread_) );
    iop.setYN( SegylikeSeisTrcTranslator::sKeyBytesSwapped, byteswapped_ );
}


void SEGY::FilePars::usePar( const IOPar& iop )
{
    iop.get( SEGYSeisTrcTranslator::sExternalNrSamples, ns_ );
    iop.getYN( SegylikeSeisTrcTranslator::sKeyBytesSwapped, byteswapped_ );
    fmt_ = fmtOf( iop.find(SEGYSeisTrcTranslator::sNumberFormat), forread_ );
}


void SEGY::FilePars::getReport( IOPar& iop ) const
{
    if ( ns_ > 0 )
	iop.set( "Number of samples used", ns_ );
    if ( fmt_ > 0 )
	iop.set( forread_ ? "SEG-Y 'format' used" : "SEG-Y 'format'",
		nameOfFmt(fmt_,forread_) );
    if ( byteswapped_ )
	iop.set( forread_ ? "Bytes are swapped" : "Bytes will be swapped", "" );
}


const char* SEGY::FilePars::nameOfFmt( int fmt, bool forread )
{
    const char** fmts = getFmts(true);
    if ( fmt > 0 && fmt < 4 )
	return fmts[fmt];
    if ( fmt == 5 )
	return fmts[4];
    if ( fmt == 8 )
	return fmts[5];

    return forread ? fmts[0] : nameOfFmt( 1, false );
}


int SEGY::FilePars::fmtOf( const char* str, bool forread )
{
    if ( !str || !*str || !isdigit(*str) )
	return forread ? 0 : 1;

    return (int)(*str - '0');
}


SEGY::FileData::FileData( const char* fnm, Seis::GeomType gt )
    : fname_(fnm)
    , geom_(gt)
    , data_(*new DataPointSet(Seis::is2D(gt),false))
    , trcsz_(-1)
    , sampling_(SI().zRange(false).start,SI().zRange(false).step)
    , segyfmt_(0)
    , isrev1_(true)
    , nrstanzas_(0)
{
}


SEGY::FileData::FileData( const SEGY::FileData& fd )
    : fname_(fd.fname_)
    , geom_(fd.geom_)
    , data_(*new DataPointSet(fd.data_))
    , trcsz_(fd.trcsz_)
    , sampling_(fd.sampling_)
    , segyfmt_(fd.segyfmt_)
    , isrev1_(fd.isrev1_)
    , nrstanzas_(fd.nrstanzas_)
{
}


SEGY::FileData::~FileData()
{
    delete &data_;
}


int SEGY::FileData::nrTraces() const
{
    return data_.size();
}


BinID SEGY::FileData::binID( int nr ) const
{
    return data_.binID( nr );
}


Coord SEGY::FileData::coord( int nr ) const
{
    Coord ret( data_.coord( nr ) );
    ret.x += data_.value(0,nr); ret.y += data_.value(1,nr);
    return ret;
}


float SEGY::FileData::offset( int nr ) const
{
    return data_.z( nr );
}


int SEGY::FileData::trcNr( int nr ) const
{
    return data_.trcNr( nr );
}


bool SEGY::FileData::isNull( int nr ) const
{
    return data_.isSelected( nr );
}


bool SEGY::FileData::isUsable( int nr ) const
{
    return data_.group( nr ) != 2;
}


void SEGY::FileData::add( const BinID& bid, const Coord& c, int nr, float offs,
			  bool isnull, bool isusable )
{
    DataPointSet::DataRow dr;
    dr.pos_.nr_ = nr;
    dr.pos_.z_ = offs;
    dr.setSel( isnull );
    dr.setGroup( isusable ? 1 : 2 );

    dr.pos_.set( bid, c );
    const Coord poscoord( dr.pos_.coord() );
    dr.data_ += (float)(c.x - poscoord.x);
    dr.data_ += (float)(c.y - poscoord.y);

    data_.addRow( dr );
}


void SEGY::FileData::addEnded()
{
    data_.dataChanged();
}


void SEGY::FileData::getReport( IOPar& iop ) const
{
}
