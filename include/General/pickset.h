#ifndef pickset_h
#define pickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		May 2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: pickset.h,v 1.5 2001-09-06 11:32:33 bert Exp $
________________________________________________________________________

-*/

#include <uidobj.h>
#include <sets.h>
#include <position.h>


/*!\brief Pick location in space */

class PickLocation
{
public:
		PickLocation( double x=0, double y=0, float f=0 )
		: pos(x,y), z(f)			{}
		PickLocation( const Coord& c, float f=0 )
		: pos(c), z(f)				{}

    inline bool	operator ==( const PickLocation& pl ) const
		{ return pos == pl.pos && mIS_ZERO(z-pl.z); }
    inline bool	operator !=( const PickLocation& pl ) const
		{ return !(*this == pl); }

    bool	fromString(const char*);
    void	toString(char*);

    Coord	pos;
    float	z;

};


/*!\brief Group of picks with a common 'value' at the location */

class PickGroup : public UserIDObject
		, public TypeSet<PickLocation>
{
public:

			PickGroup( const char* nm )
			: UserIDObject(nm)	{}

};


/*!\brief Set of Pick Groups */

class PickSet : public UserIDObject
{
public:

			PickSet( const char* nm=0 )
			: UserIDObject(nm)		{}
			~PickSet()		{ deepErase(groups); }

    int			nrGroups() const	{ return groups.size(); }
    PickGroup*		get( int nr )		{ return groups[nr]; }
    const PickGroup*	get( int nr ) const	{ return groups[nr]; }
    void		add(PickGroup*&);
			//!< PickGroup becomes mine. Will merge if necessary.
			//!< So PickGroup may be deleted (will be set to null)
    void		remove( int idx )
			{ delete groups[idx]; groups.remove(idx); }

protected:

    ObjectSet<PickGroup> groups;

};


#endif
