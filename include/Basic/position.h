#ifndef position_H
#define position_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Inline/crossline and Coordinate
 RCS:		$Id: position.h,v 1.55 2008-12-29 10:50:10 cvsranojay Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "rcol.h"
#include "geometry.h"

class BufferString;


/*!\brief a cartesian coordinate in 2D space. */

mClass Coord : public Geom::Point2D<double>
{
public:
		Coord( const Geom::Point2D<double>& p )
		    :  Geom::Point2D<double>( p )			{}
		Coord() :  Geom::Point2D<double>( 0, 0 )		{}
		Coord( double cx, double cy )	
		    :  Geom::Point2D<double>( cx, cy )			{}

    bool	operator==( const Coord& crd ) const
		{ return mIsEqual(x,crd.x,mDefEps)
		      && mIsEqual(y,crd.y,mDefEps); }
    bool	operator!=( const Coord& crd ) const
		{ return ! (crd == *this); }
    bool	operator<(const Coord&crd) const
		{ return x<crd.x || (x==crd.x && y<crd.y); }
    bool	operator>(const Coord&crd) const
		{ return x>crd.x || (x==crd.x && y>crd.y); }

    double	angle(const Coord& from,const Coord& to) const;
    double	cosAngle(const Coord& from,const Coord& to) const;
    		//!< saves the expensive acos() call
		//
    Coord	normalize() const;
    double	dot(const Coord&) const;

    void	fill(char*) const;
    bool	use(const char*);
    
    static const Coord&		udf();
};

bool getDirectionStr( const Coord&, BufferString& );
/*!< Returns strings like 'South-West', NorthEast depending on the given
     coord that is assumed to have the x-axis pointing northwards, and the
     y axis pointing eastwards
*/


/*!\brief a cartesian coordinate in 3D space. */

mClass Coord3 : public Coord
{
public:

			Coord3() : z(0)					{}
			Coord3(const Coord& a, double z_ )
			    : Coord(a), z(z_)				{}
			Coord3(const Coord3& xyz )
			    : Coord( xyz.x, xyz.y )
			    , z( xyz.z )				{}
    			Coord3( double x_, double y_, double z_ )
			    : Coord(x_,y_), z(z_)			{}

    double&		operator[]( int idx )
			{ return idx ? (idx==1 ? y : z) : x; }
    double		operator[]( int idx ) const
			{ return idx ? (idx==1 ? y : z) : x; }

    inline Coord3	operator+(const Coord3&) const;
    inline Coord3	operator-(const Coord3&) const;
    inline Coord3	operator-() const;
    inline Coord3	operator*(double) const;
    inline Coord3	operator/(double) const;
    inline Coord3	scaleBy( const Coord3& ) const;
    inline Coord3	unScaleBy( const Coord3& ) const;

    inline Coord3&	operator+=(const Coord3&);
    inline Coord3&	operator-=(const Coord3&);
    inline Coord3&	operator/=(double);
    inline Coord3&	operator*=(double);
    inline Coord&	coord()				{ return *this; }
    inline const Coord&	coord() const			{ return *this; }

    inline bool		operator==(const Coord3&) const;
    inline bool		operator!=(const Coord3&) const;
    inline bool		isDefined() const;
    double		distTo( const Coord3& b ) const;
    double		sqDistTo( const Coord3& b ) const;

    inline double	dot( const Coord3& b ) const;
    inline Coord3	cross( const Coord3& ) const;
    double		abs() const;
    double		sqAbs() const;
    inline Coord3	normalize() const;

    void	fill(char* str) const { fill( str, "(", " ", ")"); }
    void	fill(char*, const char* start, const char* space,
	    		    const char* end) const;
    bool	use(const char*);

    double	z;

    static const Coord3&	udf();
};


inline Coord3 operator*( double f, const Coord3& b )
{ return Coord3(b.x*f, b.y*f, b.z*f ); }


/*!\brief 2D coordinate and a value. */

mClass CoordValue
{
public:
		CoordValue( double x=0, double y=0, float v=mUdf(float) )
		: coord(x,y), value(v)		{}
		CoordValue( const Coord& c, float v=mUdf(float) )
		: coord(c), value(v)		{}
    bool	operator==( const CoordValue& cv ) const
		{ return cv.coord == coord; }
    bool	operator!=( const CoordValue& cv ) const
		{ return cv.coord != coord; }

    Coord	coord;
    float	value;
};


/*!\brief 3D coordinate and a value. */

mClass Coord3Value
{
public:
    		Coord3Value( double x=0, double y=0, double z=0, 
			     float v=mUdf(float) )
		: coord(x,y,z), value(v) 	{}
		Coord3Value( const Coord3& c, float v=mUdf(float) )
		: coord(c), value(v)		{}
    bool	operator==( const Coord3Value& cv ) const
		{ return cv.coord == coord; }
    bool	operator!=( const Coord3Value& cv ) const
		{ return cv.coord != coord; }

    Coord3	coord;
    float	value;
};


/*!\brief positioning in a seismic survey: inline/crossline. */

mClass BinID : public RCol
{
public:
		BinID() : inl(0), crl(0)			{}
		BinID( const RCol& rc ) : inl( rc.r() ), crl( rc.c() ) {}
		BinID( int il, int cl=1 ) : inl(il), crl(cl)	{}

    int&	r() { return inl; }
    int		r() const { return inl; }
    int&	c() { return crl; }
    int		c() const { return crl; }

		/* Implements +, -, * and other operators. See the documentation
		   for details */
    		mRowColFunctions( BinID, inl, crl );

    int		inl;
    int		crl;

};



class BinIDValues;

/*!\brief BinID and a value. */

class BinIDValue
{
public:
		BinIDValue( int inl=0, int crl=0, float v=mUdf(float) )
		: binid(inl,crl), value(v)	{}
		BinIDValue( const BinID& b, float v=mUdf(float) )
		: binid(b), value(v)		{}
		BinIDValue(const BinIDValues&,int);

    bool	operator==( const BinIDValue& biv ) const
		{ return biv.binid == binid
		      && mIsEqual(value,biv.value,sCompareEpsilon()); }
    bool	operator!=( const BinIDValue& biv ) const
		{ return !(*this == biv); }

    BinID	binid;
    float	value;

    static float sCompareEpsilon()		{ return 1e-5; }
};


/*!\brief BinID and values. If one of the values is Z, make it the first one. */

mClass BinIDValues
{
public:
			BinIDValues( int inl=0, int crl=0, int n=2 )
			: binid(inl,crl), vals(0), sz(0) { setSize(n); }
			BinIDValues( const BinID& b, int n=2 )
			: binid(b), vals(0), sz(0)	{ setSize(n); }
			BinIDValues( const BinIDValues& biv )
			: vals(0), sz(0)		{ *this = biv; }
			BinIDValues( const BinIDValue& biv )
			: binid(biv.binid), vals(0), sz(0)
					{ setSize(1); value(0) = biv.value; }
			~BinIDValues();

    BinIDValues&	operator =(const BinIDValues&);

    bool		operator==( const BinIDValues& biv ) const;
    			//!< uses BinIDValue::compareepsilon
    inline bool		operator!=( const BinIDValues& biv ) const
			{ return !(*this == biv); }

    BinID		binid;
    int			size() const			{ return sz; }
    float&		value( int idx )		{ return vals[idx]; }
    float		value( int idx ) const		{ return vals[idx]; }
    float*		values()			{ return vals; }
    const float*	values() const			{ return vals; }

    void		setSize(int,bool kpvals=false);
    void		setVals(const float*);

protected:

    float*		vals;
    int			sz;
    static float	udf;

};


inline bool Coord3::operator==( const Coord3& b ) const
{
    const double dx = x-b.x; const double dy = y-b.y; const double dz = z-b.z;
    return mIsZero(dx,mDefEps) && mIsZero(dy,mDefEps) && mIsZero(dz,mDefEps);
}


inline bool Coord3::operator!=( const Coord3& b ) const
{
    return !(b==*this);
}

inline bool Coord3::isDefined() const
{
    return !Values::isUdf(z) && Geom::Point2D<double>::isDefined();
}


inline Coord3 Coord3::operator+( const Coord3& p ) const
{
    return Coord3( x+p.x, y+p.y, z+p.z );
}


inline Coord3 Coord3::operator-( const Coord3& p ) const
{
    return Coord3( x-p.x, y-p.y, z-p.z );
}


inline Coord3 Coord3::operator-() const
{
    return Coord3( -x, -y, -z );
}


inline Coord3 Coord3::operator*( double factor ) const
{ return Coord3( x*factor, y*factor, z*factor ); }


inline Coord3 Coord3::operator/( double denominator ) const
{ return Coord3( x/denominator, y/denominator, z/denominator ); }


inline Coord3 Coord3::scaleBy( const Coord3& factor ) const
{ return Coord3( x*factor.x, y*factor.y, z*factor.z ); }


inline Coord3 Coord3::unScaleBy( const Coord3& denominator ) const
{ return Coord3( x/denominator.x, y/denominator.y, z/denominator.z ); }


inline Coord3& Coord3::operator+=( const Coord3& p )
{
    x += p.x; y += p.y; z += p.z;
    return *this;
}


inline Coord3& Coord3::operator-=( const Coord3& p )
{
    x -= p.x; y -= p.y; z -= p.z;
    return *this;
}


inline Coord3& Coord3::operator*=( double factor )
{
    x *= factor; y *= factor; z *= factor;
    return *this;
}


inline Coord3& Coord3::operator/=( double denominator )
{
    x /= denominator; y /= denominator; z /= denominator;
    return *this;
}


inline double Coord3::dot(const Coord3& b) const
{ return x*b.x + y*b.y + z*b.z; }


inline Coord3 Coord3::cross(const Coord3& b) const
{ return Coord3( y*b.z-z*b.y, z*b.x-x*b.z, x*b.y-y*b.x ); }


inline Coord3 Coord3::normalize() const
{
    const double absval = abs();
    return *this / absval;
}

#endif
