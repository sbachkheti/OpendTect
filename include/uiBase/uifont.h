#ifndef uifont_H
#define uifont_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id: uifont.h,v 1.2 2001-01-26 09:54:02 arend Exp $
________________________________________________________________________

-*/
#include "fontdata.h"

class QFont;
class QFontMetrics;
class Settings;
class uiObject;
class UserIDSet;

class uiFont 
{			//!< font stuff that needs Qt.

    friend bool		select(uiFont&,uiObject*,const char*); 
    friend class	uiFontList;

protected:
			uiFont(const char* key, const char* family,
				int ps=FontData::defaultPointSize(),
				FontData::Weight w=FontData::defaultWeight(),
				bool it=FontData::defaultItalic());
			uiFont(const char* key,FontData fd=FontData());
			uiFont(const uiFont&);

public:
			//! uiFont must be created through the uiFontList

    virtual		~uiFont();
    uiFont&		 operator=(const uiFont&);
    
    FontData		fontData() const ;
    void		setFontData( const FontData& ); 
                        //!< Updates internal QFont and QFontMetrics.

    inline const QFont&	qFont() const { return *mQtThing; } 

    int			height() const;
    int			leading() const; 
    int 		maxWidth() const;
    int 		avgWidth() const;
    int 		width (const char* str) const;
    int			ascent() const; 
    int			descent() const; 

    const char*		key() const		{ return key_; }

protected: 
    inline QFont*&	qFont_() { return mQtThing; } 

    // don't change order of these attributes!
    QFont*		mQtThing; 
    QFontMetrics&	mQFontMetrics; 

    BufferString	key_;

    void		updateMetrics();

};


class uiFontList : public CallBacker
{
    friend class	uiSetFonts;

public:

    static int		nrKeys();
    static const char*	key(int);
    static void		listKeys(ObjectSet<const char>&);
    static void		listKeys(UserIDSet&);

    static uiFont&	get(const char* key=0);

    static uiFont&	add(const char* key,const FontData&);
    static uiFont&	add(const char* key,
			    const char* f=FontData::defaultFamily(),
			    int ptsz=FontData::defaultPointSize(),
			    FontData::Weight w=FontData::defaultWeight(),
			    bool it=FontData::defaultItalic());

    static void		use(const Settings&);
    static void		update(Settings&);

protected:

    static ObjectSet<uiFont> fonts;
    static void		initialise();
    static uiFont&	gtFont(const char* key,const FontData* =0);

private:

    static bool		inited;

    static void		addOldGuess(const Settings&,const char*,int);
    static void		removeOldEntries(Settings&);

};


#endif
