#ifndef uigraphicsscene_h
#define uigraphicsscene_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsscene.h,v 1.26 2010-01-08 14:05:56 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsitem.h"
#include "bufstringset.h"
#include "color.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "namedobj.h"


class QGraphicsScene;
class QGraphicsLinearLayout;
class QGraphicsWidget;
class ODGraphicsScene;

class ArrowStyle;
class Alignment;
class MarkerStyle2D;

class uiPolygonItem;
class uiPolyLineItem;
class uiRectItem;
class uiObjectItem;

mClass uiGraphicsScene : public NamedObject
{
public:
				uiGraphicsScene(const char*);
				~uiGraphicsScene();

    void			removeAllItems();
    uiGraphicsItem*		removeItem(uiGraphicsItem*);
    				/*!<Gives object back to caller (not deleted) */

    template <class T> T*	addItem(T*);
    uiGraphicsItemGroup*	addItemGrp(uiGraphicsItemGroup*);
    				//!<Item becomes mine
    int				nrItems() const;
    int				getDPI() const;

    uiRectItem*			addRect(float x,float y,float w,float h);

    uiPolygonItem*		addPolygon(const TypeSet<uiPoint>&,bool fill);
    uiPolyLineItem*		addPolyLine(const TypeSet<uiPoint>&);

    void			useBackgroundPattern(bool);
    void 			setBackGroundColor(const Color&);
    const Color			backGroundColor() const;

    int				getSelItemSize() const;
    uiRect			getSelectedArea() const;
    void			setSelectionArea(const uiRect&);

    MouseEventHandler&		getMouseEventHandler()	
    				{ return mousehandler_; }

    Notifier<uiGraphicsScene>	ctrlPPressed;
    double			width() const;
    double			height() const;

    void			saveAsImage(const char*,int,int,int);
    void			saveAsPDF(const char*,int);
    void			saveAsPS(const char*,int);
    void			saveAsPDF_PS(const char*,bool pdf_or_ps,int);
    void			setSceneRect(float x,float y,float w,float h);
    uiRect			sceneRect();

    const bool			isMouseEventActive() const	
    				{ return ismouseeventactive_; }
    void			setMouseEventActive( bool yn )
    				{ ismouseeventactive_ = yn; }
    QGraphicsScene*		qGraphicsScene()
    				{ return (QGraphicsScene*)odgraphicsscene_; }

protected:

    ObjectSet<uiGraphicsItem>	items_;
    ODGraphicsScene*		odgraphicsscene_;

    MouseEventHandler		mousehandler_;
    bool			ismouseeventactive_;
    friend class		uiGraphicsItem;
    uiGraphicsItem*		doAddItem(uiGraphicsItem*);

};


template <class T>
inline T* uiGraphicsScene::addItem( T* itm )
{
    return (T*)(itm ? itm->addToScene( this ) : itm);
}


class uiGraphicsObjectScene : public uiGraphicsScene
{
public:
				uiGraphicsObjectScene(const char*);
    
    void                        addObjectItem(uiObjectItem*);
    void                        removeObjectItem(uiObjectItem*);
    void			setItemStretch(uiObjectItem*,int stretch);
    int 			stretchFactor(uiObjectItem*) const;

protected:

    void 			resizeLayoutToContent();

    QGraphicsLinearLayout*      layout_;
    QGraphicsWidget*		layoutitem_;
};

#endif
