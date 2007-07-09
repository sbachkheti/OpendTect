#ifndef draw_h
#define draw_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: draw.h,v 1.16 2007-07-09 16:47:00 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "color.h"
#include "geometry.h"


class Alignment
{
public:

    enum Pos		{ Start, Middle, Stop };
			DeclareEnumUtils(Pos)

			Alignment( Pos h=Start, Pos v=Start )
			: hor_(h), ver_(v)	{}

    Pos			hor_;
    Pos			ver_;

};

#define mAlign(h,v) Alignment(Alignment::h,Alignment::v)


class MarkerStyle2D
{
public:

    enum Type		{ None, Square, Circle, Cross };
			DeclareEnumUtils(Type)

			MarkerStyle2D( Type tp=Square, int sz=2,
				       Color col=Color::Black,
				       const char* fk=0 )
			: type_(tp), size_(sz), color_(col), fontkey_(fk) {}

    bool		operator==(const MarkerStyle2D& a) const
			{ return a.type_==type_ && a.size_==size_ &&
			         a.color_==color_ && a.fontkey_==a.fontkey_; }

    Type		type_;
    int			size_;
    Color		color_;
    BufferString	fontkey_;

    inline bool		isVisible() const
			{ return type_!=None && size_>0 && color_.isVisible(); }

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


class MarkerStyle3D
{
public:

    enum Type		{ None=-1,
			  Cube=0, Cone, Cylinder, Sphere, Arrow, Cross };
			DeclareEnumUtils(Type)

			MarkerStyle3D( Type tp=Cube, int sz=3,
				       Color col=Color::White )
			: type_(tp), size_(sz), color_(col)	{}

    Type		type_;
    int			size_;
    Color		color_;

    inline bool		isVisible() const
			{ return size_>0 && color_.isVisible(); }

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


class LineStyle
{
public:

    enum Type		{ None, Solid, Dash, Dot, DashDot, DashDotDot };
			// This enum should not be changed: it is cast
			// directly to a UI enum.
			DeclareEnumUtils(Type)

			LineStyle( Type t=Solid,int w=1,Color c=Color::Black )
			: type_(t), width_(w), color_(c)	{}

    bool		operator ==( const LineStyle& ls ) const
			{ return type_ == ls.type_ && width_ == ls.width_
			      && color_ == ls.color_; }
    bool		operator !=( const LineStyle& ls ) const
			{ return !(*this == ls); }

    Type		type_;
    int			width_;
    Color		color_;

    inline bool		isVisible() const
			{ return type_!=None && width_>0 && color_.isVisible();}

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


template <class PTCLSS,class T>
class Arrow
{
public:

    enum Type		{ HeadOnly, TwoSided, TailOnly };
    enum HeadType	{ Line, Triangle, Square, Circle, Cross };
    enum HandedNess	{ TwoHanded, LeftHanded, RightHanded };

			Arrow( T boldness=1, Type t=HeadOnly )
			: linestyle_(LineStyle::Solid,boldness)
			, headtype_(Line)
			, headsz_(2*boldness)
			, tailtype_(Line)
			, tailsz_(2*boldness)
			, handedness_(TwoHanded)		{}

    inline bool		operator ==( const Arrow& a ) const
			{ return start_ == a.start_ && stop_ == a.stop_; }
    inline bool		operator !=( const Arrow& a ) const
			{ return !(*this == a); }

    void		setBoldNess( T b )
			{ linestyle_.width_ = b; headsz_ = tailsz_ = 2*b; }

    PTCLSS		start_;
    PTCLSS		stop_;
    LineStyle		linestyle_;	//!< contains the color of the arrow
    HeadType		headtype_;
    T			headsz_;
    HeadType		tailtype_;
    T			tailsz_;
    HandedNess		handedness_;

};

#define Arrow2D Arrow<Geom::Point2D<int>,int>
#define Arrow3D Arrow<Geom::Point3D<int>,int>


#endif
