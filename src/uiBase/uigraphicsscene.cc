/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsscene.cc,v 1.36 2010-01-08 14:05:56 cvsbruno Exp $";


#include "uigraphicsscene.h"

#include "draw.h"
#include "uigraphicsitemimpl.h"

#include <QByteArray>
#include <QGraphicsLinearLayout>
#include <QGraphicsItemGroup>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QImageWriter>
#include <QKeyEvent>
#include <QList>
#include <QPainter>
#include <QPoint>
#include <QPrinter>
#include <QString>
#include <math.h>

#if !defined( __win__ ) && !defined( __mac__ )
# include <QX11Info>
#endif



class ODGraphicsScene : public QGraphicsScene
{
public:
    			ODGraphicsScene( uiGraphicsScene& scene )
			    : uiscene_(scene)
			    , bgopaque_(false)
			    , mousepressedbs_(OD::NoButton) {}

    void		setBackgroundOpaque( bool yn )	{ bgopaque_ = yn; }
protected:
    virtual void	wheelEvent(QGraphicsSceneWheelEvent* qevent);
    virtual void	keyPressEvent(QKeyEvent* qkeyevent);
    virtual void	mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void	mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);

    virtual void	drawBackground(QPainter*,const QRectF&);
    OD::ButtonState	mousepressedbs_;

private:

    bool		bgopaque_;

    uiGraphicsScene&	uiscene_;
};


void ODGraphicsScene::drawBackground( QPainter* painter, const QRectF& rect )
{
    painter->setBackgroundMode( bgopaque_ ? Qt::OpaqueMode 
	    				  : Qt::TransparentMode );
    QGraphicsScene::drawBackground( painter, rect );
}


void ODGraphicsScene::keyPressEvent( QKeyEvent* qkeyevent )
{
    OD::KeyboardKey key = OD::KeyboardKey( qkeyevent->key() );
    OD::ButtonState modifier = OD::ButtonState( (int)qkeyevent->modifiers() );
    if ( key == OD::P && modifier == OD::ControlButton )
	uiscene_.ctrlPPressed.trigger();
}


void ODGraphicsScene::wheelEvent( QGraphicsSceneWheelEvent* ev )
{
    MouseEvent me( OD::NoButton, (int)ev->pos().x(), (int)ev->pos().y(),
	    	   ev->delta() );
    uiscene_.getMouseEventHandler().triggerWheel( me );
}


void ODGraphicsScene::mouseMoveEvent( QGraphicsSceneMouseEvent* qev )
{
    MouseEvent mev( mousepressedbs_, (int)qev->scenePos().x(),
	    	    (int)qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerMovement( mev );
    QGraphicsScene::mouseMoveEvent( qev );
}


void ODGraphicsScene::mousePressEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    mousepressedbs_ = bs;
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerButtonPressed( mev );
    QGraphicsScene::mousePressEvent( qev );
}


void ODGraphicsScene::mouseReleaseEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    if ( qev->modifiers() == Qt::ControlModifier )
	bs = OD::ControlButton;
    else if ( qev->modifiers() == Qt::ShiftModifier )
	bs = OD::ShiftButton;
    mousepressedbs_ = OD::NoButton;
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerButtonReleased( mev );
    QGraphicsScene::mouseReleaseEvent( qev );
}


void ODGraphicsScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    if ( uiscene_.isMouseEventActive() )
	uiscene_.getMouseEventHandler().triggerDoubleClick( mev );
    QGraphicsScene::mouseDoubleClickEvent( qev );
}



uiGraphicsScene::uiGraphicsScene( const char* nm )
    : NamedObject(nm)
    , mousehandler_(MouseEventHandler())
    , ismouseeventactive_(true)
    , odgraphicsscene_(new ODGraphicsScene(*this))
    , ctrlPPressed(this)
{
    odgraphicsscene_->setObjectName( nm );
    odgraphicsscene_->setBackgroundBrush( Qt::white );
}


uiGraphicsScene::~uiGraphicsScene()
{
    removeAllItems();
    delete odgraphicsscene_;
}


int uiGraphicsScene::nrItems() const
{
    return odgraphicsscene_->items().size();
}


uiGraphicsItem* uiGraphicsScene::doAddItem( uiGraphicsItem* itm )
{
    if ( !itm ) return 0;
    odgraphicsscene_->addItem( itm->qGraphicsItem() );
    items_ += itm;
    return itm;
}


uiGraphicsItemGroup* uiGraphicsScene::addItemGrp( uiGraphicsItemGroup* itmgrp )
{
    if ( !itmgrp ) return 0;
    odgraphicsscene_->addItem( itmgrp->qGraphicsItemGroup() );
    items_ += itmgrp;
    return itmgrp;
}


uiGraphicsItem* uiGraphicsScene::removeItem( uiGraphicsItem* itm )
{
    if ( !itm )
	return 0;

    odgraphicsscene_->removeItem( itm->qGraphicsItem() );
    items_ -= itm;

    return itm;
}


uiRectItem* uiGraphicsScene::addRect( float x, float y, float w, float h )
{ 
    uiRectItem* uirectitem =
	new uiRectItem( odgraphicsscene_->addRect(x,y,w,h) );
    items_ += uirectitem;
    return uirectitem;
}


uiPolygonItem* uiGraphicsScene::addPolygon( const TypeSet<uiPoint>& pts,
					    bool fill )
{
    QVector<QPointF> qpts;
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	QPointF pt( QPoint(pts[idx].x,pts[idx].y) );
	qpts += pt;
    }

    uiPolygonItem* uipolyitem = new uiPolygonItem(
	odgraphicsscene_->addPolygon(QPolygonF(qpts)) );
    if ( fill )
	uipolyitem->fill();
    items_ += uipolyitem;
    return uipolyitem;
}

uiPolyLineItem* uiGraphicsScene::addPolyLine( const TypeSet<uiPoint>& ptlist )
{
    uiPolyLineItem* polylineitem = new uiPolyLineItem();
    polylineitem->setPolyLine( ptlist );
    addItem( polylineitem );
    return polylineitem;
}


void uiGraphicsScene::setBackGroundColor( const Color& color )
{
    QBrush brush( QColor(color.r(),color.g(),color.b()) );
    odgraphicsscene_->setBackgroundBrush( brush );
}


const Color uiGraphicsScene::backGroundColor() const
{
    QColor color( odgraphicsscene_->backgroundBrush().color() ); 
    return Color( color.red() , color.green(), color.blue() );
}


void uiGraphicsScene::removeAllItems()
{
    for ( int idx=items_.size()-1; idx>=0; idx-- )
	delete removeItem( items_[idx] );

    QList<QGraphicsItem *> items = odgraphicsscene_->items();
    for ( int idx=0; idx<items.size(); idx++ )
	odgraphicsscene_->removeItem( items[idx] );
}


int uiGraphicsScene::getSelItemSize() const
{
    QList<QGraphicsItem*> items = odgraphicsscene_->selectedItems();
    return items.size();
}


uiRect uiGraphicsScene::getSelectedArea() const
{
    QRectF selarea( odgraphicsscene_->selectionArea().boundingRect().toRect() );
    return uiRect( (int)selarea.topLeft().x(), (int)selarea.topLeft().y(),
	   	   (int)selarea.bottomRight().x(),
		   (int)selarea.bottomRight().y() );
}


void uiGraphicsScene::setSelectionArea( const uiRect& uirect )
{
    uiRect rect = uirect;
    rect.checkCorners();
    const QRectF selrect( rect.topLeft().x, rect.topLeft().y, rect.width(),
	    		  rect.height() );
    QPainterPath selareapath;
    selareapath.addRect( selrect );
    odgraphicsscene_->setSelectionArea( selareapath );
}


void uiGraphicsScene::useBackgroundPattern( bool usebgpattern )
{
    odgraphicsscene_->setBackgroundOpaque( usebgpattern );

    if ( usebgpattern )
    {
	QBrush brush;
	brush.setColor( QColor(0,0,0) );
	brush.setStyle( Qt::DiagCrossPattern );
	odgraphicsscene_->setBackgroundBrush( brush );
    }
}


double uiGraphicsScene::width() const
{ return odgraphicsscene_->width(); }

double uiGraphicsScene::height() const
{ return odgraphicsscene_->height(); }

int uiGraphicsScene::getDPI() const
{
// TODO: move to Basic
#if defined( __win__ ) || defined( __mac__ )
    return 100;
#else
    return QX11Info::appDpiX();
#endif
}


void uiGraphicsScene::setSceneRect( float x, float y, float w, float h )
{ odgraphicsscene_->setSceneRect( x, y, w, h ); }


uiRect uiGraphicsScene::sceneRect()
{
    QRectF qrect = odgraphicsscene_->sceneRect();
    return uiRect( (int)qrect.x(), (int)qrect.y(),
		   (int)qrect.width(), (int)qrect.height() );
}


void uiGraphicsScene::saveAsImage( const char* filename, int width,
				   int height, int resolution )
{
    QString fileName( filename );
    QPainter *imagepainter = new QPainter();
    QImage* image = new QImage( QSize(width,height), QImage::Format_ARGB32);
    image->setDotsPerMeterX( resolution*254 );
    image->setDotsPerMeterY( resolution*254 );
    imagepainter->begin(image);
    QGraphicsView* view = qGraphicsScene()->views()[0];
    QRectF sourcerect( view->mapToScene(0,0),
	    	       view->mapToScene(view->width(),view->height()) );
    qGraphicsScene()->render( imagepainter,QRectF(0,0,width,height),sourcerect);
    imagepainter->end();
    image->save(fileName);
    delete imagepainter;
    delete image;
}


void uiGraphicsScene::saveAsPDF_PS( const char* filename, bool aspdf,
       				    int resolution )
{
    QString fileName( filename );
    QPrinter* pdfprinter = new QPrinter();
    pdfprinter->setOutputFormat( aspdf ? QPrinter::PdfFormat
				       : QPrinter::PostScriptFormat );
    pdfprinter->setPageSize( QPrinter::A4 );
    pdfprinter->setFullPage( true );
    pdfprinter->setOutputFileName( filename );

    QPainter* pdfpainter = new QPainter();
    pdfpainter->begin( pdfprinter );
    QGraphicsView* view = qGraphicsScene()->views()[0];
    QRectF sourcerect( view->mapToScene(0,0),
	    	       view->mapToScene(view->width(),view->height()) );
    qGraphicsScene()->render( pdfpainter,
	    QRectF(0,0,pdfprinter->width(),pdfprinter->height()) ,sourcerect );
    pdfpainter->end();
    delete pdfpainter;
    delete pdfprinter;
}


void uiGraphicsScene::saveAsPDF( const char* filename, int resolution )
{ saveAsPDF_PS( filename, true, resolution ); }

void uiGraphicsScene::saveAsPS( const char* filename, int resolution )
{ saveAsPDF_PS( filename, false, resolution ); }



uiGraphicsObjectScene::uiGraphicsObjectScene( const char* nm )
    : uiGraphicsScene(nm)
    , layout_(new QGraphicsLinearLayout)  
    , layoutitem_(new QGraphicsWidget)
{
    qGraphicsScene()->addItem( layoutitem_ );
    layoutitem_->setLayout( layout_ );
}


void uiGraphicsObjectScene::resizeLayoutToContent()
{
    float width = 0; float height = 0; 
    for ( int idx=0; idx<layout_->count(); idx++ )
    {
	mDynamicCastGet(uiObjectItem*,item,items_[idx]);
	if ( !item ) continue;
	width += item->objectSize().width();
	height += item->objectSize().height();
    }
    layoutitem_->resize( width, height );
}


void uiGraphicsObjectScene::addObjectItem( uiObjectItem* item  )
{
    layout_->addItem( item->qWidgetItem() );
    items_ += item;
    resizeLayoutToContent();
}


void uiGraphicsObjectScene::removeObjectItem( uiObjectItem* item )
{
    layout_->removeItem( item->qWidgetItem() );
    items_ -= item;
    resizeLayoutToContent();
}


void uiGraphicsObjectScene::setItemStretch( uiObjectItem* item, int stretch )
{
    layout_->setStretchFactor( item->qWidgetItem(), stretch );
}


int uiGraphicsObjectScene::stretchFactor( uiObjectItem* item ) const
{
    return layout_->stretchFactor( item->qWidgetItem() );
}
