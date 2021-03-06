#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		15-1-2000
________________________________________________________________________

-*/


#include "basicmod.h"
#include "bufstring.h"


/*!\brief Concatenated short keys separated by dots.
Used for Object identifiers in the Object Manager, or stratigraphic IDs. */

mExpClass(Basic) CompoundKey : public OD::String
{
public:

    typedef int		IdxType;

    inline		CompoundKey( const char* s=0 )	{ if ( s ) impl_ = s; }
    inline		CompoundKey( const CompoundKey& ck )
			: impl_(ck.impl_)	{}
    inline CompoundKey&	operator=( const char* s )
						{ impl_ = s; return *this;}
    inline CompoundKey&	operator+=(const char*);
    inline bool		operator==( const char* s ) const
						{ return impl_ == s; }
    inline bool		operator==( const CompoundKey& oth ) const
						{ return impl_ == oth.impl_; }
    inline bool		operator!=( const char* s ) const
						{ return impl_ != s; }
    inline bool		operator!=(const CompoundKey& u) const
						{ return impl_ != u.impl_; }
    inline void		setEmpty()		{ impl_.setEmpty(); }
    inline bool		isEmpty() const		{ return impl_.isEmpty();}
    inline char*	getCStr()		{ return impl_.getCStr(); }
    inline		operator const char*() const
						{ return buf(); }

    IdxType		nrKeys() const;
    BufferString	key(IdxType) const;
    void		setKey(IdxType,const char*);
    CompoundKey		upLevel() const;
    bool		isUpLevelOf(const CompoundKey&) const;

protected:

    BufferString	impl_;
    char*		fromKey(IdxType) const;
    const char*		getKeyPart(IdxType) const;

    virtual const char*	gtBuf() const		{ return impl_.buf(); }
    virtual const char*	gtStr() const		{ return impl_.str(); }

private:

    char*		fetchKeyPart(IdxType,bool) const;

};


inline CompoundKey& CompoundKey::operator +=( const char* s )
{
    if ( !impl_.isEmpty() )
	impl_.add( "." );
    impl_.add( s );
    return *this;
}
