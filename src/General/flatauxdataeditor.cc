/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2000
 RCS:           $Id: flatauxdataeditor.cc,v 1.13 2007-07-09 16:47:00 cvsbert Exp $
________________________________________________________________________

-*/

#include "flatauxdataeditor.h"

#include "mouseevent.h"


namespace FlatView
{

AuxDataEditor::AuxDataEditor( Viewer& v, MouseEventHandler& meh )
    : viewer_( v )
    , mousehandler_( meh )
    , feedback_( 0 )
    , mousedown_( false )
    , hasmoved_( false )
    , addauxdataid_( 0 )
    , removeSelected( this )
    , movementStarted( this )
    , movementFinished( this )
    , seldatasetidx_( -1 )
    , selptidx_( -1 )
{
    meh.buttonPressed.notify( mCB(this,AuxDataEditor,mousePressCB) );
    meh.buttonReleased.notify( mCB(this,AuxDataEditor,mouseReleaseCB) );
    meh.movement.notify( mCB(this,AuxDataEditor,mouseMoveCB) );
}


AuxDataEditor::~AuxDataEditor()
{
    mousehandler_.buttonPressed.remove( mCB(this,AuxDataEditor,mousePressCB) );
    mousehandler_.buttonReleased.remove(mCB(this,AuxDataEditor,mouseReleaseCB));
    mousehandler_.movement.remove( mCB(this,AuxDataEditor,mouseMoveCB) );
}


int AuxDataEditor::addAuxData( FlatView::Annotation::AuxData* nd, bool doedit )
{
    if ( viewer_.appearance().annot_.auxdata_.indexOf( nd )==-1 )
    {
	pErrMsg("Auxdata not present in viewer");
	return -1;
    }

    int res = 0;
    while ( ids_.indexOf( res )!=-1 )
	res++;

    ids_ += res;
    auxdata_ += nd;
    allowadd_ += true;
    allowmove_ += true;
    allowremove_ += true;
    doedit_ += doedit;

    return res;
}


void AuxDataEditor::removeAuxData( int id )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return;

    ids_.remove( idx );
    auxdata_.remove( idx );
    allowadd_.remove( idx );
    allowmove_.remove( idx );
    allowremove_.remove( idx );
    doedit_.remove( idx );
}


void AuxDataEditor::enableEdit( int id, bool allowadd, bool allowmove,
			        bool allowdelete )
{
    const int idx = ids_.indexOf( id );

    allowadd_[idx] = allowadd;
    allowmove_[idx] = allowmove;
    allowremove_[idx] = allowdelete;
}


void AuxDataEditor::setAddAuxData( int id )
{ addauxdataid_ = id; }


int AuxDataEditor::getAddAuxData() const
{
    return addauxdataid_;
}

    
void AuxDataEditor::setView( const Rect& wv,
			     const Geom::Rectangle<int>& mouserect )
{
    curview_ = wv;
    mousearea_ = mouserect;
}


int AuxDataEditor::getSelPtDataID() const
{ return seldatasetidx_!=-1 ? ids_[seldatasetidx_] : -1; }


int AuxDataEditor::getSelPtIdx() const
{ return selptidx_; }


const Point& AuxDataEditor::getSepPtPos() const
{ return selptcoord_; }


const TypeSet<int>& AuxDataEditor::getIds() const
{ return ids_; }


const ObjectSet<Annotation::AuxData>&
AuxDataEditor::getAuxData() const
{ return auxdata_; }


void AuxDataEditor::mousePressCB( CallBacker* cb )
{
    if ( mousehandler_.isHandled() ) 
	return; 

    const MouseEvent& ev = mousehandler_.event(); 
    if ( !(ev.buttonState() & OD::LeftButton ) ||
	  (ev.buttonState() & OD::MidButton ) ||
	  (ev.buttonState() & OD::RightButton ) )
	return;

    if ( mousedown_ )
    {
	mouseReleaseCB( cb );
	return;
    }

    if ( !updateSelection( ev.pos() ) &&
	 !ev.ctrlStatus() && !ev.shiftStatus() && !ev.altStatus() )
    {
	seldatasetidx_ = ids_.indexOf( addauxdataid_ );
	if ( seldatasetidx_!=-1 && !allowadd_[seldatasetidx_] )
	    seldatasetidx_ = -1;
    }

    if ( seldatasetidx_==-1 )
	return;

    const Rect wr = movementlimit_ = getWorldRect( ids_[seldatasetidx_] );
    seldatatransform_.set3Pts( wr.topLeft(), wr.topRight(), wr.bottomLeft(), 
	       RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
	       RowCol( mousearea_.topRight().x, mousearea_.topRight().y ),
	       mousearea_.bottomLeft().y );

    selptcoord_ = selptidx_!=-1 ? auxdata_[seldatasetidx_]->poly_[selptidx_]
				: seldatatransform_.transform(
				    RowCol(ev.pos().x,ev.pos().y ) );
    mousedown_ = true;
    hasmoved_ = false;

    movementStarted.trigger();

    mousehandler_.setHandled( true );
}


void AuxDataEditor::mouseReleaseCB( CallBacker* cb )
{
    if ( !mousedown_ )
	return;

    if ( mousehandler_.isHandled() ) 
	return; 

    const MouseEvent& ev = mousehandler_.event(); 
    if ( !(ev.buttonState() & OD::LeftButton ) ||
	  (ev.buttonState() & OD::MidButton ) ||
	  (ev.buttonState() & OD::RightButton ) )
	return;

    if ( !hasmoved_ && ev.ctrlStatus() && !ev.shiftStatus() &&
	 !ev.altStatus() && seldatasetidx_!=-1 &&
	 allowremove_[seldatasetidx_] && selptidx_!=-1 )
    {
	removeSelected.trigger();

	if ( doedit_[seldatasetidx_] )
	{
	    auxdata_[seldatasetidx_]->poly_.remove( selptidx_ );
	    viewer_.handleChange( Viewer::Annot );
	}

	mousehandler_.setHandled( true );
	mousedown_ = false;
	return;
    }

    mousedown_ = false;

    if ( hasmoved_ || selptidx_==-1 )
    {
	movementFinished.trigger();
	mousehandler_.setHandled( true );
    }

    if ( feedback_ )
    {
	viewer_.appearance().annot_.auxdata_ -= feedback_;
	delete feedback_;
	feedback_ = 0;

	if ( selptidx_!=-1 )
	    viewer_.appearance().annot_.auxdata_ += auxdata_[seldatasetidx_];

	viewer_.handleChange( Viewer::Annot );
    }
}


void AuxDataEditor::mouseMoveCB( CallBacker* cb )
{
    if ( !mousedown_ || !allowmove_[seldatasetidx_] )
	return;

    if ( mousehandler_.isHandled() ) 
	return; 


    const MouseEvent& ev = mousehandler_.event(); 
    if ( !(ev.buttonState() & OD::LeftButton ) ||
	  (ev.buttonState() & OD::MidButton ) ||
	  (ev.buttonState() & OD::RightButton ) )
	return;

    hasmoved_ = true;

    if ( ev.ctrlStatus() || ev.shiftStatus() || ev.altStatus() )
	return;

    const Geom::Point2D<int> mousedisplaypos = mousearea_.moveInside(ev.pos());
    selptcoord_ = seldatatransform_.transform(
	    RowCol(mousedisplaypos.x,mousedisplaypos.y ) );

    selptcoord_ = movementlimit_.moveInside( selptcoord_ );

    if ( doedit_[seldatasetidx_]  && selptidx_!=-1 )
	auxdata_[seldatasetidx_]->poly_[selptidx_] = selptcoord_;
    else if ( !feedback_ )
    {
	feedback_ = new Annotation::AuxData( *auxdata_[seldatasetidx_] );
	viewer_.appearance().annot_.auxdata_ += feedback_;
	if ( selptidx_==-1 )
	{
	    feedback_->poly_.erase();
	    feedback_->poly_ += selptcoord_;
	}
	else
	    viewer_.appearance().annot_.auxdata_ -= auxdata_[seldatasetidx_];
    }
    else if ( selptidx_==-1 )
	feedback_->poly_[0] = selptcoord_;
    else 
	feedback_->poly_[selptidx_] = selptcoord_;

    viewer_.handleChange( Viewer::Annot );
    mousehandler_.setHandled( true );
}


bool AuxDataEditor::updateSelection( const Geom::Point2D<int>& pt )
{
    seldatasetidx_ = -1;
    selptidx_ = -1;

    int minsqdist;
    for ( int idx=0; idx<auxdata_.size(); idx++ )
    {
	if ( !auxdata_[idx] )
	    continue;

	const int rng = auxdata_[idx]->markerstyle_.size_;
	const Geom::PixRectangle<int> markerrect( pt.x-rng, pt.y-rng,
						  pt.x+rng, pt.y+rng );

	const Rect wr = getWorldRect( ids_[idx] );
	RCol2Coord transform;
	transform.set3Pts( wr.topLeft(), wr.topRight(), wr.bottomLeft(), 
	       RowCol( mousearea_.topLeft().x, mousearea_.topLeft().y ),
	       RowCol( mousearea_.topRight().x, mousearea_.topRight().y ),
	       mousearea_.bottomLeft().y );

	const TypeSet<Point>& dataset = auxdata_[idx]->poly_;

	for ( int idy=0; idy<dataset.size(); idy++ )
	{
	    const RowCol rc = transform.transformBack( dataset[idy] );
	    const Geom::Point2D<int> displaypos( rc.row, rc.col );
	    if ( !markerrect.isInside( displaypos ) )
		continue;

	    const int sqdist = displaypos.sqDistTo( pt );
	    if ( seldatasetidx_==-1 || sqdist<minsqdist )
	    {
		seldatasetidx_ = idx;
		selptidx_ = idy;
		minsqdist = sqdist;
	    }
	}
    }

    return seldatasetidx_!=-1;

}


Rect AuxDataEditor::getWorldRect( int id ) const
{
    Rect res( curview_ );
    const int idx = ids_.indexOf( id );

    if ( auxdata_[idx]->x1rg_ )
	res.setLeftRight( *auxdata_[idx]->x1rg_ );

    if ( auxdata_[idx]->x2rg_ )
	res.setTopBottom( *auxdata_[idx]->x2rg_ );

    return res;
}


void AuxDataEditor::limitMovement( const Rect& r )
{ movementlimit_ = r; }


};
