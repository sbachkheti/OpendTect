#ifndef color_h
#define color_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		22-3-2000
 RCS:		$Id: color.h,v 1.14 2008-05-19 19:58:45 cvskris Exp $
________________________________________________________________________

Color is an RGB color object, with a transparancy. The storage is in a 4-byte
integer, similar to Qt.

-*/


#include "gendefs.h"


class Color
{
public:

			Color( unsigned char r_=255, unsigned char g_=255,
				unsigned char b_=255, unsigned char t_=0 );
			Color( unsigned int rgbval );

    bool		operator ==( const Color& c ) const;
    bool		operator !=( const Color& c ) const;

    unsigned char	r() const;
    unsigned char	g() const;
    unsigned char	b() const;
    unsigned char	t() const;

    bool		isVisible() const;

    unsigned int	rgb() const;
    unsigned int&	rgb();

    void         	set( unsigned char r_, unsigned char g_,
			     unsigned char b_, unsigned char t_=0 );

    Color		complementaryColor() const;
    Color		operator*(float) const;
    void		lighter( float f=1.1 );
    void        	setRgb( unsigned int rgb_  );
    void		setTransparency( unsigned char t_ );
    void		setHSV(unsigned char h,unsigned char s,unsigned char v);
    void		getHSV(unsigned char&,unsigned char&,unsigned char&);
    void		setStdStr(const char*); //!< e.g. "#00ff32"
    const char*		getStdStr(bool withhash=true,
	    			  int transpopt=0) const;
    			//!< transpopt -1=opacity 0=not 1=transparency

    void		fill(char*) const;
    bool		use(const char*);

    static Color	NoColor;
    static Color	Black;
    static Color	White;
    static Color	DgbColor;
    static Color	Wheat;
    static Color	LightGrey;

    static unsigned char getUChar( float v );

    static int		nrStdDrawColors();
    static Color	stdDrawColor(int);

protected:

    unsigned int	col_;
};


#endif
