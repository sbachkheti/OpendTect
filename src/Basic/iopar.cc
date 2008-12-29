/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-12-1995
-*/

static const char* rcsID = "$Id: iopar.cc,v 1.76 2008-12-29 10:47:53 cvsranojay Exp $";

#include "iopar.h"
#include "multiid.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"
#include "globexpr.h"
#include "position.h"
#include "separstr.h"
#include "ascstream.h"
#include "samplingdata.h"
#include "bufstringset.h"
#include "color.h"
#include "convert.h"
#include "errh.h"


IOPar::IOPar( const char* nm )
	: NamedObject(nm)
	, keys_(*new BufferStringSet(true))
	, vals_(*new BufferStringSet(true))
{
}


IOPar::IOPar( ascistream& astream )
	: NamedObject("")
	, keys_(*new BufferStringSet(true))
	, vals_(*new BufferStringSet(true))
{
    getFrom( astream );
}


IOPar::IOPar( const IOPar& iop )
	: NamedObject(iop.name())
	, keys_(*new BufferStringSet(true))
	, vals_(*new BufferStringSet(true))
{
    for ( int idx=0; idx<iop.size(); idx++ )
	add( iop.keys_.get(idx), iop.vals_.get(idx) );
}


IOPar& IOPar::operator =( const IOPar& iop )
{
    if ( this != &iop )
    {
	clear();
	setName( iop.name() );
	for ( int idx=0; idx<iop.size(); idx++ )
	    add( iop.keys_.get(idx), iop.vals_.get(idx) );
    }
    return *this;
}


bool IOPar::isEqual( const IOPar& iop, bool worder ) const
{
    if ( &iop == this ) return true;
    const int sz = size();
    if ( iop.size() != sz ) return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	if ( worder )
	{
	    if ( iop.keys_.get(idx) != keys_.get(idx)
	      || iop.vals_.get(idx) != vals_.get(idx) )
		return false;
	}
	else
	{
	    const char* res = iop.find( getKey(idx) );
	    if ( !res || strcmp(res,getValue(idx)) )
		return false;
	}
    }

    return true;
}


IOPar::~IOPar()
{
    clear();
    delete &keys_;
    delete &vals_;
}


int IOPar::size() const
{
    return keys_.size();
}


int IOPar::indexOf( const char* key ) const
{
    return keys_.indexOf( key );
}


const char* IOPar::getKey( int nr ) const
{
    if ( nr >= size() ) return "";
    return keys_.get( nr ).buf();
}


const char* IOPar::getValue( int nr ) const
{
    if ( nr >= size() ) return "";
    return vals_.get( nr ).buf();
}


bool IOPar::setKey( int nr, const char* s )
{
    if ( nr >= size() || !s || !*s || keys_.indexOf(s) >= 0 )
	return false;

    keys_.get(nr) = s;
    return true;
}


void IOPar::setValue( int nr, const char* s )
{
    if ( nr < size() )
	vals_.get(nr) = s;
}


void IOPar::clear()
{
    deepErase( keys_ ); deepErase( vals_ );
}


void IOPar::remove( int idx )
{
    if ( idx >= size() ) return;
    keys_.remove( idx ); vals_.remove( idx );
}


void IOPar::remove( const char* key )
{
    const int idx = keys_.indexOf( key );
    if ( idx<0 )
	return;

    remove( idx );
}


void IOPar::merge( const IOPar& iopar )
{
    if ( &iopar == this ) return;

    for ( int idx=0; idx<iopar.size(); idx++ )
	set( iopar.keys_.get(idx), iopar.vals_.get(idx) );
}


const char* IOPar::compKey( const char* key1, int k2 )
{
    static BufferString intstr;
    intstr = ""; intstr += k2;
    return compKey( key1, (const char*)intstr );
}


const char* IOPar::compKey( const char* key1, const char* key2 )
{
    static BufferString ret;
    ret = key1;
    if ( key1 && key2 && *key1 && *key2 ) ret += ".";
    ret += key2;
    return ret;
}


IOPar* IOPar::subselect( int nr ) const
{
    BufferString s; s+= nr;
    return subselect( s.buf() );
}


IOPar* IOPar::subselect( const char* key ) const
{
    if ( !key ) return 0;

    IOPar* iopar = new IOPar( name() );
    for ( int idx=0; idx<keys_.size(); idx++ )
    {
	const char* nm = keys_.get(idx).buf();
	if ( !matchString(key,nm) ) continue;
	nm += strlen(key);
	if ( *nm == '.' && *(nm+1) )
	    iopar->add( nm+1, vals_.get(idx) );
    }

    if ( iopar->size() == 0 )
	{ delete iopar; iopar = 0; }
    return iopar;
}


void IOPar::removeSubSelection( int nr )
{
    BufferString s; s+= nr;
    return removeSubSelection( s.buf() );
}


void IOPar::removeSubSelection( const char* key )
{
    if ( !key ) return;

    for ( int idx=0; idx<keys_.size(); idx++ )
    {
	const char* nm = keys_.get(idx).buf();
	if ( !matchString(key,nm) ) continue;
	nm += strlen(key);
	if ( *nm == '.' && *(nm+1) )
	    { remove( idx ); idx--; }
    }
}


void IOPar::mergeComp( const IOPar& iopar, const char* ky )
{
    BufferString key( ky );
    char* ptr = key.buf() + key.size()-1;
    while ( ptr != key.buf() && *ptr == '.' )
	*ptr = '\0';

    const bool havekey = !key.isEmpty();
    if ( !havekey && &iopar == this ) return;

    BufferString buf;
    for ( int idx=0; idx<iopar.size(); idx++ )
    {
	buf = key;
	if ( havekey ) buf += ".";
	buf += iopar.keys_.get(idx);
	set( buf, iopar.vals_.get(idx) );
    }
}


const char* IOPar::findKeyFor( const char* s, int nr ) const
{
    if ( !s ) return 0;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( vals_.get(idx) == s )
	{
	    if ( nr )	nr--;
	    else	return keys_.get(idx).buf();
	}
    }

    return 0;
}


void IOPar::removeWithKey( const char* key )
{
    GlobExpr ge( key );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ge.matches( keys_.get(idx) ) )
	{
	    remove( idx );
	    idx--;
	}
    }
}


const char* IOPar::operator[]( const char* keyw ) const
{
    const char* res = find( keyw );
    return res ? res : "";
}


const char* IOPar::find( const char* keyw ) const
{
    int idx = keys_.indexOf( keyw );
    return idx < 0 ? 0 : vals_.get(idx).buf();
}


void IOPar::add( const char* nm, const char* val )
{
    keys_.add( nm ); vals_.add( val );
}


#define mDefYNFns(fnnm) \
void IOPar::fnnm##YN( const char* keyw, bool yn ) \
{ \
    fnnm( keyw, getYesNoString(yn) ); \
} \
void IOPar::fnnm##YN( const char* keyw, bool yn1, bool yn2 ) \
{ \
    FileMultiString fms( getYesNoString(yn1) ); \
    fms.add( getYesNoString(yn2) ); \
    fnnm( keyw, fms ); \
} \
void IOPar::fnnm##YN( const char* keyw, bool yn1, bool yn2, bool yn3 ) \
{ \
    FileMultiString fms( getYesNoString(yn1) ); \
    fms.add( getYesNoString(yn2) ); \
    fms.add( getYesNoString(yn3) ); \
    fnnm( keyw, fms ); \
} \
void IOPar::fnnm##YN( const char* keyw, bool yn1, bool yn2, bool yn3, bool yn4 ) \
{ \
    FileMultiString fms( getYesNoString(yn1) ); \
    fms.add( getYesNoString(yn2) ); \
    fms.add( getYesNoString(yn3) ); \
    fms.add( getYesNoString(yn4) ); \
    fnnm( keyw, fms ); \
}

mDefYNFns(set)
mDefYNFns(add)


#define mDefSet1Val( type ) \
void IOPar::set( const char* keyw, type val ) \
{\
    set( keyw, Conv::to<const char*>(val) );\
}
#define mDefSet2Val( type ) \
void IOPar::set( const char* s, type v1, type v2 ) \
{ \
    FileMultiString fms = Conv::to<const char*>(v1); \
    fms.add( Conv::to<const char*>(v2) ); \
    set( s, fms ); \
}
#define mDefSet3Val( type ) \
void IOPar::set( const char* s, type v1, type v2, type v3 ) \
{ \
    FileMultiString fms = Conv::to<const char*>(v1); \
    fms.add( Conv::to<const char*>(v2) ); \
    fms.add( Conv::to<const char*>(v3) ); \
    set( s, fms ); \
}
#define mDefSet4Val( type ) \
void IOPar::set( const char* s, type v1, type v2, type v3, type v4 ) \
{ \
    FileMultiString fms = Conv::to<const char*>(v1); \
    fms.add( Conv::to<const char*>(v2) ); \
    fms.add( Conv::to<const char*>(v3) ); \
    fms.add( Conv::to<const char*>(v4) ); \
    set( s, fms ); \
}

#define mDefAdd1Val(type) \
void IOPar::add( const char* keyw, type val ) \
{\
    add( keyw, Conv::to<const char*>(val) );\
}
#define mDefAdd2Val( type ) \
void IOPar::add( const char* s, type v1, type v2 ) \
{ \
    FileMultiString fms = Conv::to<const char*>(v1); \
    fms.add( Conv::to<const char*>(v2) ); \
    add( s, fms ); \
}
#define mDefAdd3Val( type ) \
void IOPar::add( const char* s, type v1, type v2, type v3 ) \
{ \
    FileMultiString fms = Conv::to<const char*>(v1); \
    fms.add( Conv::to<const char*>(v2) ); \
    fms.add( Conv::to<const char*>(v3) ); \
    add( s, fms ); \
}
#define mDefAdd4Val( type ) \
void IOPar::add( const char* s, type v1, type v2, type v3, type v4 ) \
{ \
    FileMultiString fms = Conv::to<const char*>(v1); \
    fms.add( Conv::to<const char*>(v2) ); \
    fms.add( Conv::to<const char*>(v3) ); \
    fms.add( Conv::to<const char*>(v4) ); \
    add( s, fms ); \
}

mDefSet1Val(int)	mDefSet2Val(int)
mDefSet3Val(int)	mDefSet4Val(int)
mDefSet1Val(od_uint32)	mDefSet2Val(od_uint32)
mDefSet3Val(od_uint32)	mDefSet4Val(od_uint32)
mDefSet1Val(od_int64)	mDefSet2Val(od_int64)
mDefSet3Val(od_int64)	mDefSet4Val(od_int64)
mDefSet1Val(od_uint64)	mDefSet2Val(od_uint64)
mDefSet3Val(od_uint64)	mDefSet4Val(od_uint64)
mDefSet1Val(float)	mDefSet2Val(float)
mDefSet3Val(float)	mDefSet4Val(float)
mDefSet1Val(double)	mDefSet2Val(double)
mDefSet3Val(double)	mDefSet4Val(double)

mDefAdd1Val(int)	mDefAdd2Val(int)
mDefAdd3Val(int)	mDefAdd4Val(int)
mDefAdd1Val(od_uint32)	mDefAdd2Val(od_uint32)
mDefAdd3Val(od_uint32)	mDefAdd4Val(od_uint32)
mDefAdd1Val(od_int64)	mDefAdd2Val(od_int64)
mDefAdd3Val(od_int64)	mDefAdd4Val(od_int64)
mDefAdd1Val(od_uint64)	mDefAdd2Val(od_uint64)
mDefAdd3Val(od_uint64)	mDefAdd4Val(od_uint64)
mDefAdd1Val(float)	mDefAdd2Val(float)
mDefAdd3Val(float)	mDefAdd4Val(float)
mDefAdd1Val(double)	mDefAdd2Val(double)
mDefAdd3Val(double)	mDefAdd4Val(double)


#define mDefGetI1Val( type, convfunc ) \
bool IOPar::get( const char* s, type& v1 ) const \
{ \
    const char* ptr = find(s); \
    if ( !ptr || !*ptr ) return false; \
\
    char* endptr; \
    type tmpval = convfunc; \
    if ( ptr==endptr ) return false; \
    v1 = tmpval; \
    return true; \
}

#define mDefGetI2Val( type, convfunc ) \
bool IOPar::get( const char* s, type& v1, type& v2 ) const \
{ \
    const char* ptr = find(s); \
    if ( !ptr || !*ptr ) return false; \
    FileMultiString fms( ptr ); \
    if ( fms.size() < 2 ) return false; \
    char* endptr; \
\
    ptr = fms[0]; \
    type tmpval = convfunc; \
    if ( ptr == endptr ) return false; \
    v1 = tmpval; \
\
    ptr = fms[1]; tmpval = convfunc; \
    if ( ptr != endptr ) v2 = tmpval; \
\
    return true; \
}

#define mDefGetI3Val( type, convfunc ) \
bool IOPar::get( const char* s, type& v1, type& v2, type& v3 ) const \
{ \
    const char* ptr = find(s); \
    if ( !ptr || !*ptr ) return false; \
    FileMultiString fms( ptr ); \
    if ( fms.size() < 3 ) return false; \
    char* endptr; \
\
    ptr = fms[0]; \
    type tmpval = convfunc; \
    if ( ptr == endptr ) return false; \
    v1 = tmpval; \
\
    ptr = fms[1]; tmpval = convfunc; \
    if ( ptr != endptr ) v2 = tmpval; \
\
    ptr = fms[2]; tmpval = convfunc; \
    if ( ptr != endptr ) v3 = tmpval; \
\
    return true; \
}

#define mDefGetI4Val( type, convfunc ) \
bool IOPar::get( const char* s, type& v1, type& v2, type& v3, type& v4 ) const \
{ \
    const char* ptr = find(s); \
    if ( !ptr || !*ptr ) return false; \
    FileMultiString fms( ptr ); \
    if ( fms.size() < 4 ) return false; \
    char* endptr; \
\
    ptr = fms[0]; \
    type tmpval = convfunc; \
    if ( ptr == endptr ) return false; \
    v1 = tmpval; \
\
    ptr = fms[1]; tmpval = convfunc; \
    if ( ptr != endptr ) v2 = tmpval; \
\
    ptr = fms[2]; tmpval = convfunc; \
    if ( ptr != endptr ) v3 = tmpval; \
\
    ptr = fms[3]; tmpval = convfunc; \
    if ( ptr != endptr ) v3 = tmpval; \
\
    return true; \
}

mDefGetI1Val(int,strtol(ptr, &endptr, 0));
mDefGetI2Val(int,strtol(ptr, &endptr, 0));
mDefGetI3Val(int,strtol(ptr, &endptr, 0));
mDefGetI4Val(int,strtol(ptr, &endptr, 0));
mDefGetI1Val(od_uint32,strtoul(ptr, &endptr, 0));
mDefGetI2Val(od_uint32,strtoul(ptr, &endptr, 0));
mDefGetI3Val(od_uint32,strtoul(ptr, &endptr, 0));
mDefGetI4Val(od_uint32,strtoul(ptr, &endptr, 0));
mDefGetI1Val(od_int64,strtoll(ptr, &endptr, 0));
mDefGetI2Val(od_int64,strtoll(ptr, &endptr, 0));
mDefGetI3Val(od_int64,strtoll(ptr, &endptr, 0));
mDefGetI4Val(od_int64,strtoll(ptr, &endptr, 0));
mDefGetI1Val(od_uint64,strtoull(ptr, &endptr, 0));
mDefGetI2Val(od_uint64,strtoull(ptr, &endptr, 0));
mDefGetI3Val(od_uint64,strtoull(ptr, &endptr, 0));
mDefGetI4Val(od_uint64,strtoull(ptr, &endptr, 0));


#define mDefGetFVals(typ) \
bool IOPar::get( const char* k, typ& v ) const \
{ return getSc(k,v,1,false); } \
bool IOPar::get( const char* k, typ& v1, typ& v2 ) const \
{ return getSc(k,v1,v2,1,false); } \
bool IOPar::get( const char* k, typ& v1, typ& v2, typ& v3 ) const \
{ return getSc(k,v1,v2,v3,1,false); } \
bool IOPar::get( const char* k, typ& v1, typ& v2, typ& v3, typ& v4 ) const \
{ return getSc(k,v1,v2,v3,v4,1,false); }

mDefGetFVals(float)
mDefGetFVals(double)


template <class T>
static bool iopget_typeset( const IOPar& iop, const char* s, TypeSet<T>& res )
{
    const char* ptr = iop.find(s); \
    if ( !ptr || !*ptr ) return false;

    int keyidx = 0;
    while ( ptr && *ptr )
    {
	FileMultiString fms( ptr );
	const int len = fms.size();
	for ( int idx=0; idx<len; idx++ )
	{
	    const char* valstr = fms[idx];
	    const T newval = Conv::to<T>( valstr ); 
	    res += newval;
	}

	keyidx++;
	BufferString newkey = IOPar::compKey(s,keyidx);
	ptr = iop.find( newkey );
    }
    return true;
}


bool IOPar::get( const char* s, TypeSet<int>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<od_uint32>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<od_int64>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<od_uint64>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<double>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<float>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::getSc( const char* s, float& f, float sc, bool udf ) const
{
    const char* ptr = find( s );
    if ( ptr && *ptr )
    {
	Conv::udfset( f, ptr );
	if ( !mIsUdf(f) ) f *= sc;
	return true;
    }
    else if ( udf )
	Values::setUdf(f);

    return false;
}


bool IOPar::getSc( const char* s, double& d, double sc, bool udf ) const
{
    const char* ptr = find( s );
    if ( ptr && *ptr )
    {
	d = atof( ptr );
	if ( !mIsUdf(d) ) d *= sc;
	return true;
    }
    else if ( udf )
	Values::setUdf(d);

    return false;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float sc,
		   bool udf ) const
{
    double d1 = f1, d2 = f2;
    if ( getSc( s, d1, d2, sc, udf ) )
	{ f1 = (float)d1; f2 = (float)d2; return true; }
    return false;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float& f3, float sc,
		   bool udf ) const
{
    double d1=f1, d2=f2, d3=f3;
    if ( getSc( s, d1, d2, d3, sc, udf ) )
	{ f1 = (float)d1; f2 = (float)d2; f3 = (float)d3; return true; }
    return false;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float& f3, float& f4,
		   float sc, bool udf ) const
{
    double d1=f1, d2=f2, d3=f3, d4=f4;
    if ( getSc( s, d1, d2, d3, d4, sc, udf ) )
    {
	f1 = (float)d1; f2 = (float)d2; f3 = (float)d3; f4 = (float)d4;
	return true;
    }
    return false;

}


bool IOPar::getSc( const char* s, double& d1, double& d2, double sc,
		 bool udf ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( udf || (ptr && *ptr) )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    havedata = true;
	    d1 = atof( ptr );
	    if ( !mIsUdf(d1) ) d1 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d1);

	ptr = fms[1];
	if ( *ptr )
	{
	    havedata = true;
	    d2 = atof( ptr );
	    if ( !mIsUdf(d2) ) d2 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d2);
    }
    return havedata;
}


bool IOPar::getSc( const char* s, double& d1, double& d2, double& d3, double sc,
		 bool udf ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( udf || (ptr && *ptr) )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    d1 = atof( ptr );
	    if ( !mIsUdf(d1) ) d1 *= sc;
	    havedata = true;
	}
	else if ( udf )
	    Values::setUdf(d1);

	ptr = fms[1];
	if ( *ptr )
	{
	    d2 = atof( ptr );
	    if ( !mIsUdf(d2) ) d2 *= sc;
	    havedata = true;
	}
	else if ( udf )
	    Values::setUdf(d2);

	ptr = fms[2];
	if ( *ptr )
	{
	    d3 = atof( ptr );
	    if ( !mIsUdf(d3) ) d3 *= sc;
	    havedata = true;
	}
	else if ( udf )
	    Values::setUdf(d3);
    }
    return havedata;
}


bool IOPar::getSc( const char* s, double& d1, double& d2, double& d3,
		   double& d4, double sc, bool udf ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( udf || (ptr && *ptr) )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    havedata = true;
	    d1 = atof( ptr );
	    if ( !mIsUdf(d1) ) d1 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d1);

	ptr = fms[1];
	if ( *ptr )
	{
	    havedata = true;
	    d2 = atof( ptr );
	    if ( !mIsUdf(d2) ) d2 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d2);

	ptr = fms[2];
	if ( *ptr )
	{
	    havedata = true;
	    d3 = atof( ptr );
	    if ( !mIsUdf(d3) ) d3 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d3);

	ptr = fms[3];
	if ( *ptr )
	{
	    havedata = true;
	    d4 = atof( ptr );
	    if ( !mIsUdf(d4) ) d4 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d4);
    }
    return havedata;
}


bool IOPar::get( const char* s, int& i1, int& i2, float& f ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( ptr && *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr ) { i1 = atoi( ptr ); havedata = true; }
	ptr = fms[1];
	if ( *ptr ) { i2 = atoi( ptr ); havedata = true; }
	ptr = fms[2];
	if ( *ptr ) { f = atof( ptr ); havedata = true; }
    }
    return havedata;
}


bool IOPar::getYN( const char* s, bool& i ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    i = yesNoFromString(ptr);
    return true;
}


bool IOPar::getYN( const char* s, bool& i1, bool& i2 ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    FileMultiString fms( ptr );
    i1 = yesNoFromString(fms[0]);
    i2 = yesNoFromString(fms[1]);
    return true;
}


bool IOPar::getYN( const char* s, bool& i1, bool& i2, bool& i3 ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    FileMultiString fms( ptr );
    i1 = yesNoFromString(fms[0]);
    i2 = yesNoFromString(fms[1]);
    i3 = yesNoFromString(fms[2]);
    return true;
}


bool IOPar::getYN( const char* s, bool& i1, bool& i2, bool& i3, bool& i4 ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    FileMultiString fms( ptr );
    i1 = yesNoFromString(fms[0]);
    i2 = yesNoFromString(fms[1]);
    i3 = yesNoFromString(fms[2]);
    i4 = yesNoFromString(fms[3]);
    return true;
}


bool IOPar::getPtr( const char* s, void*& res ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    return sscanf( ptr, "%p", &res ) > 0;
}


void IOPar::set( const char* keyw, const char* vals )
{
    int idx = keys_.indexOf( keyw );
    if ( idx < 0 )
	add( keyw, vals );
    else
	setValue( idx, vals );
}


void IOPar::set( const char* keyw, const char* vals1, const char* vals2 )
{
    FileMultiString fms( vals1 ); fms += vals2;
    int idx = keys_.indexOf( keyw );
    if ( idx < 0 )
	add( keyw, fms );
    else
	setValue( idx, fms );
}


const int mMaxFMSLineSize = 100;

template <class T>
static void iopset_typeset( IOPar& iop, const char* keyw, 
			    const TypeSet<T>& vals )
{
    const int nrvals = vals.size();

    int validx = 0; 
    int keyidx = 0;
   
    while ( validx != nrvals )
    {
	T val = vals[ validx++ ];
	FileMultiString fms( Conv::to<const char*>(val) );

	for ( int cnt=1; cnt<mMaxFMSLineSize; cnt++ )
	{
	    if ( validx == nrvals ) break;
	    
	    val = vals[ validx++ ];
	    fms += Conv::to<const char*>( val );
	}
	
	BufferString newkey = keyidx ? IOPar::compKey(keyw,keyidx) : keyw;
	iop.set( newkey, fms );
	keyidx++;
    }
}


void IOPar::set( const char* keyw, const TypeSet<int>& vals ) 
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<od_uint32>& vals ) 
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<od_int64>& vals )
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<od_uint64>& vals )
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<float>& vals )
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<double>& vals )
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* s, int i1, int i2, float f )
{
    FileMultiString fms = Conv::to<const char*>( i1 );
    fms.add( Conv::to<const char*>(i2) );
    fms.add( Conv::to<const char*>(f) );
    set( s, fms );
}


void IOPar::setPtr( const char* keyw, void* ptr )
{
    char buf[80]; sprintf( buf, "%p", ptr );
    set( keyw, buf );
}


bool IOPar::get( const char* s, Coord& crd ) const
{ return get( s, crd.x, crd.y ); }
void IOPar::set( const char* s, const Coord& crd )
{ set( s, crd.x, crd.y ); }

bool IOPar::get( const char* s, Coord3& crd ) const
{ return get( s, crd.x, crd.y, crd.z ); }
void IOPar::set( const char* s, const Coord3& crd )
{ set( s, crd.x, crd.y, crd.z ); }

bool IOPar::get( const char* s, BinID& binid ) const
{ return get( s, binid.inl, binid.crl ); }
void IOPar::set( const char* s, const BinID& binid )
{ set( s, binid.inl, binid.crl ); }


bool IOPar::get( const char* s, SeparString& ss ) const
{
    const char* res = find( s );
    if ( !res ) return false;
    ss = res;
    return true;
}


bool IOPar::get( const char* s, BufferString& bs ) const
{
    const char* res = find( s );
    if ( !res ) return false;
    bs = res;
    return true;
}


bool IOPar::get( const char* s, BufferString& bs1, BufferString& bs2 ) const
{
    const char* res = find( s );
    if ( !res ) return false;
    FileMultiString fms( res );
    bs1 = fms[0]; bs2 = fms[1];
    return true;
}


bool IOPar::get( const char* s, BufferStringSet& bss ) const
{
    const char* res = find( s );
    if ( !res ) return false;

    bss.erase();
    FileMultiString fms( res );
    const int sz = fms.size();
    if ( sz )
	bss.setIsOwner( true );
    for ( int idx=0; idx<sz; idx++ )
	bss.add( fms[idx] );
    return true;
}


void IOPar::set( const char* s, const SeparString& ss )
{
    set( s, ss.buf() );
}


void IOPar::set( const char* s, const BufferString& bs )
{
    set( s, bs.buf() );
}


void IOPar::set( const char* s, const BufferString& bs1,
				const BufferString& bs2 )
{
    set( s, bs1.buf(), bs2.buf() );
}


void IOPar::set( const char* s, const BufferStringSet& bss )
{
    FileMultiString fms;
    for ( int idx=0; idx<bss.size(); idx++ )
	fms += bss.get( idx );
    set( s, fms.buf() );
}


bool IOPar::get( const char* s, MultiID& mid ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;
    mid = ptr;
    return true;
}


void IOPar::set( const char* s, const MultiID& mid )
{
    set( s, mid.buf() );
}


bool IOPar::get( const char* s, SamplingData<int>& sd ) const
{
    return get( s, sd.start, sd.step );
}

bool IOPar::get( const char* s, SamplingData<float>& sd ) const
{
    return get( s, sd.start, sd.step );
}

bool IOPar::get( const char* s, SamplingData<double>& sd ) const
{
    return get( s, sd.start, sd.step );
}

bool IOPar::get( const char* s, Interval<int>& rg ) const
{
    mDynamicCastGet(StepInterval<int>*,si,&rg)
    if ( si )
	return get( s, rg.start, rg.stop, si->step );
    else
	return get( s, rg.start, rg.stop );
}

bool IOPar::get( const char* s, Interval<float>& rg ) const
{
    mDynamicCastGet(StepInterval<float>*,si,&rg)
    if ( si )
	return get( s, rg.start, rg.stop, si->step );
    else
	return get( s, rg.start, rg.stop );
}

bool IOPar::get( const char* s, Interval<double>& rg ) const
{
    mDynamicCastGet(StepInterval<double>*,si,&rg)
    if ( si )
	return get( s, rg.start, rg.stop, si->step );
    else
	return get( s, rg.start, rg.stop );
}

void IOPar::set( const char* s, const Interval<int>& rg )
{
    mDynamicCastGet(const StepInterval<int>*,si,&rg)
    if ( si )
	set( s, rg.start, rg.stop, si->step );
    else
	set( s, rg.start, rg.stop );
}

void IOPar::set( const char* s, const Interval<float>& rg )
{
    mDynamicCastGet(const StepInterval<float>*,si,&rg)
    if ( si )
	set( s, rg.start, rg.stop, si->step );
    else
	set( s, rg.start, rg.stop );
}

void IOPar::set( const char* s, const Interval<double>& rg )
{
    mDynamicCastGet(const StepInterval<double>*,si,&rg)
    if ( si )
	set( s, rg.start, rg.stop, si->step );
    else
	set( s, rg.start, rg.stop );
}

void IOPar::set( const char* s, const SamplingData<double>& sd )
{
    set( s, sd.start, sd.step );
}

void IOPar::set( const char* s, const SamplingData<float>& sd )
{
    set( s, sd.start, sd.step );
}

void IOPar::set( const char* s, const SamplingData<int>& sd )
{
    set( s, sd.start, sd.step );
}



bool IOPar::get( const char* s, Color& c ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    return c.use( ptr );
}

void IOPar::set( const char* s, const Color& c )
{
    BufferString bs; c.fill( bs.buf() );
    set( s, bs );
}


void IOPar::getFrom( ascistream& strm )
{
    if ( atEndOfSection(strm) )
	strm.next();
    if ( strm.type() == ascistream::Keyword )
    {
	setName( strm.keyWord() );
	strm.next();
    }

    while ( !atEndOfSection(strm) )
    {
	set( strm.keyWord(), strm.value() );
	strm.next();
    }
}


void IOPar::putTo( ascostream& strm ) const
{
    if ( !name().isEmpty() )
	strm.put( name() );
    for ( int idx=0; idx<size(); idx++ )
	strm.put( keys_.get(idx), vals_.get(idx) );
    strm.newParagraph();
}


static const char* startsep	= "+## [";
static const char* midsep	= "] ## [";
static const char* endsep	= "] ##.";

void IOPar::putTo( BufferString& str ) const
{
    str = name();
    putParsTo( str );
}


void IOPar::putParsTo( BufferString& str ) const
{
    BufferString buf;
    for ( int idx=0; idx<size(); idx++ )
    {
	buf = startsep;
	buf += keys_.get(idx);
	buf += midsep;
	buf += vals_.get(idx);
	buf += endsep;
	str += buf;
    }
}

#define mAdvanceSep( ptr, sep ) \
    while ( *ptr && ( *ptr!=*sep || strncmp(ptr,sep,strlen(sep)) ) ) \
	{ ptr++; } \
\
    if( *ptr && !strncmp(ptr,sep,strlen(sep) ) ) \
    { \
	*ptr++ = '\0'; \
\
	for ( int idx=1; idx<strlen(sep); idx++ ) \
	    { if( *ptr ) ptr++; } \
    }

void IOPar::getFrom( const char* str )
{
    clear();

    BufferString buf = str;
    char* ptrstart = buf.buf();
    char* ptr = ptrstart;

    mAdvanceSep( ptr, startsep )
    setName( ptrstart );

    while ( *ptr )
    {
	ptrstart = ptr; mAdvanceSep( ptr, midsep )

	keys_.add( ptrstart );

	ptrstart = ptr; mAdvanceSep( ptr, endsep )

	vals_.add( ptrstart );

	ptrstart = ptr; mAdvanceSep( ptr, startsep )
    }
}


void IOPar::getParsFrom( const char* str )
{
    clear();

    BufferString buf = str;
    char* ptrstart = buf.buf();
    char* ptr = ptrstart;
    mAdvanceSep( ptr, startsep )

    while ( *ptr )
    {
	ptrstart = ptr;	mAdvanceSep( ptr, midsep )
	keys_.add( ptrstart );
	ptrstart = ptr;	mAdvanceSep( ptr, endsep )
	vals_.add( ptrstart );
	ptrstart = ptr; mAdvanceSep( ptr, startsep )
    }
}


bool IOPar::read( const char* fnm, const char* typ, bool chktyp )
{
    StreamData sd = StreamProvider(fnm).makeIStream();
    if ( !sd.usable() ) return false;
    read( *sd.istrm, typ, chktyp );
    sd.close();
    return true;
}


void IOPar::read( std::istream& strm, const char* typ, bool chktyp )
{
    const bool havetyp = typ && *typ;
    ascistream astream( strm, havetyp ? true : false );
    if ( havetyp && chktyp && !astream.isOfFileType(typ) )
    {
	BufferString msg( "File has wrong file type: '" );
	msg += astream.fileType();
	msg += "' should be: '"; msg += typ; msg += "'";
	ErrMsg( msg );
    }
    else
	getFrom( astream );
}


bool IOPar::write( const char* fnm, const char* typ ) const
{
    StreamData sd = StreamProvider(fnm).makeOStream();
    if ( !sd.usable() ) return false;
    bool ret = write( *sd.ostrm, typ );
    sd.close();
    return ret;
}


bool IOPar::write( std::ostream& strm, const char* typ ) const
{

    if ( typ && !strcmp(typ,sKeyDumpPretty()) )
	dumpPretty( strm );
    else
    {
	ascostream astream( strm );
	if ( typ && *typ )
	    astream.putHeader( typ );
	putTo( astream );
    }
    return true;
}


void IOPar::dumpPretty( std::ostream& strm ) const
{
    if ( !name().isEmpty() )
	strm << "> " << name() << " <\n";

    int maxkeylen = 0;
    bool haveval = false;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( keys_[idx]->size() > maxkeylen )
	    maxkeylen = keys_[idx]->size();
	if ( !haveval && !vals_[idx]->isEmpty() )
	    haveval = true;
    }
    if ( maxkeylen == 0 ) return;

    const int valpos = haveval ? maxkeylen + 3 : 0;
    BufferString valposstr( valpos + 1, true );
    for ( int ispc=0; ispc<valpos; ispc++ )
	valposstr[ispc] = ' ';

    for ( int idx=0; idx<size(); idx++ )
    {
	const BufferString& ky = *keys_[idx];
	if ( ky == sKeyHdr() )
	    { strm << "\n\n* " << vals_.get(idx) << " *\n\n"; continue; }
	else if ( ky == sKeySubHdr() )
	    { strm << "\n  - " << vals_.get(idx) << "\n\n"; continue; }

	BufferString keyprint( maxkeylen + 1, true );
	const int extra = maxkeylen - ky.size();
	for ( int ispc=0; ispc<extra; ispc++ )
	    keyprint[ispc] = ' ';
	keyprint += ky;
	strm << keyprint << (haveval ? " : " : "");

	BufferString valstr( vals_.get(idx) );
	char* startptr = valstr.buf();
	while ( startptr && *startptr )
	{
	    char* nlptr = strchr( startptr, '\n' );
	    if ( nlptr )
		*nlptr = '\0';
	    strm << startptr;
	    if ( !nlptr ) break;

	    startptr = nlptr + 1;
	    if ( *startptr )
		strm << '\n' << valposstr;
	}
	strm << std::endl;
    }
}
