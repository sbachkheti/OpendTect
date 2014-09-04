/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsimplelaymoddisp.h"
#include "uistratlaymodtools.h"
#include "uistrateditlayer.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uifileinput.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiworld2ui.h"
#include "uiflatviewer.h"
#include "uimultiflatviewcontrol.h"
#include "flatposdata.h"
#include "stratlevel.h"
#include "arrayndimpl.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratreftree.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "property.h"
#include "keystrs.h"
#include "oddirs.h"
#include "od_helpids.h"

#define mGetConvZ(var,conv) \
    if ( SI().depthsInFeet() ) var *= conv
#define mGetRealZ(var) mGetConvZ(var,mFromFeetFactorF)
#define mGetDispZ(var) mGetConvZ(var,mToFeetFactorF)
#define mGetDispZrg(src,target) \
    Interval<float> target( src ); \
    if ( SI().depthsInFeet() ) \
	target.scale( mToFeetFactorF )

static const int cMaxNrLayers4RectDisp = 50000; // Simple displayer


uiStratLayerModelDisp::uiStratLayerModelDisp( uiStratLayModEditTools& t,
					  const Strat::LayerModelProvider& lmp)
    : uiGroup(t.parent(),"LayerModel display")
    , tools_(t)
    , lmp_(lmp)
    , zrg_(0,1)
    , selseqidx_(-1)
    , flattened_(false)
    , fluidreplon_(false)
    , frtxtitm_(0)
    , isbrinefilled_(true)
    , sequenceSelected(this)
    , genNewModelNeeded(this)
    , rangeChanged(this)
    , modelEdited(this)
    , infoChanged(this)
    , dispPropChanged(this)
{
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
}


const Strat::LayerModel& uiStratLayerModelDisp::layerModel() const
{
    return lmp_.getCurrent();
}


void uiStratLayerModelDisp::selectSequence( int selidx )
{
    selseqidx_ = selidx;
    drawSelectedSequence();
}


void uiStratLayerModelDisp::setFlattened( bool yn, bool trigger )
{
    flattened_ = yn;
    if ( !trigger ) return;
    
    modelChanged();
}


bool uiStratLayerModelDisp::haveAnyZoom() const
{
    const int nrseqs = layerModel().size();
    mGetDispZrg(zrg_,dispzrg);
    uiWorldRect wr( 1, dispzrg.start, nrseqs, dispzrg.stop );
    return zoomBox().isInside( wr, 1e-5 );
}


float uiStratLayerModelDisp::getLayerPropValue( const Strat::Layer& lay,
						const PropertyRef* pr,
						int propidx ) const
{
    return propidx < lay.nrValues() ? lay.value( propidx ) : mUdf(float);
}


void uiStratLayerModelDisp::displayFRText()
{
    if ( !frtxtitm_ )
	frtxtitm_ = scene().addItem( new uiTextItem( tr("<---empty--->"),
				 mAlignment(HCenter,VCenter) ) );
    frtxtitm_->setText(isbrinefilled_ ? tr("Brine filled") 
                                      : tr("Hydrocarbon filled"));
    frtxtitm_->setPenColor( Color::Black() );
    const int xpos = mNINT32( scene().width()/2 );
    const int ypos = mNINT32( scene().height()-10 );
    frtxtitm_->setPos( uiPoint(xpos,ypos) );
    frtxtitm_->setZValue( 999999 );
    frtxtitm_->setVisible( fluidreplon_ );
}

#define mErrRet(s) { uiMSG().error(s); return false; }


class uiStratLayerModelDispIO : public uiDialog
{
public:

uiStratLayerModelDispIO( uiParent* p, const Strat::LayerModel& lm, 
			    BufferString& fnm, bool forread )
    : uiDialog( p, Setup(forread ? "Read dumped models" : "Dump models",
		mNoDlgTitle,mTODOHelpKey) )
    , presmathfld_(0)
    , eachfld_(0)
    , fnm_(fnm)
    , lm_(lm)
{
    if ( !forread )
	presmathfld_ = new uiGenInput( this, "Preserve Math Formulas",
					BoolInpSpec(false) );
    uiFileInput::Setup su( uiFileDialog::Txt, fnm_ );
    filefld_ = new uiFileInput( this, "File name", su );
    if ( presmathfld_ )
	filefld_->attach( alignedBelow, presmathfld_ );

    if ( forread )
    {
	eachfld_ = new uiGenInput( this, "Use each", IntInpSpec(1,1,999999) );
	eachfld_->attach( alignedBelow, filefld_ );
    }
}

bool acceptOK( CallBacker* )
{
    fnm_ = filefld_->fileName();
    if ( fnm_.isEmpty() )
	mErrRet( "Please provide a file name" );

    if ( presmathfld_ )
    {
	od_ostream strm( fnm_ );
	if ( !strm.isOK() )
	    mErrRet( BufferString("Cannot open:\n",fnm_,"\nfor write") )
	if ( !lm_.write(strm,0,presmathfld_->getBoolValue()) )
	    mErrRet( "Unknown error during write ..." )
    }
    else
    {
	if ( !File::exists(fnm_) )
	    mErrRet( "File does not exist" );
	od_istream strm( fnm_ );
	if ( !strm.isOK() )
	    mErrRet( BufferString("Cannot open:\n",fnm_,"\nfor read") )

	Strat::LayerModel newlm;
	if ( !newlm.read(strm) )
	    mErrRet( "Cannot read layer model from file.\nDetails may be "
		    	"in the log file ('Utilities-Show log file')" )

	const int each = eachfld_->getIntValue();
	Strat::LayerModel& lm = const_cast<Strat::LayerModel&>( lm_ );
	for ( int iseq=0; iseq<newlm.size(); iseq+=each )
	    lm.addSequence( newlm.sequence(iseq) );
    }

    return true;
}

    uiFileInput*	filefld_;
    uiGenInput*		presmathfld_;
    uiGenInput*		eachfld_;

    const Strat::LayerModel& lm_;
    BufferString&	fnm_;

};


#undef mErrRet
#define mErrRet(s) \
{ \
    uiMainWin* mw = uiMSG().setMainWin( parent()->mainwin() ); \
    uiMSG().error(s); \
    uiMSG().setMainWin(mw); \
    return false; \
}

bool uiStratLayerModelDisp::doLayerModelIO( bool foradd )
{
    const Strat::LayerModel& lm = layerModel();
    if ( !foradd && lm.isEmpty() )
	mErrRet( tr("Please generate at least one layer sequence") )

    uiStratLayerModelDispIO dlg( this, lm, dumpfnm_, foradd );
    return dlg.go();
}


bool uiStratLayerModelDisp::getCurPropDispPars(
	LMPropSpecificDispPars& pars ) const
{
    LMPropSpecificDispPars disppars;
    disppars.propnm_ = tools_.selProp();
    const int curpropidx = lmdisppars_.indexOf( disppars );
    if ( curpropidx<0 )
	return false;
    pars = lmdisppars_[curpropidx];
    return true;
}


bool uiStratLayerModelDisp::setPropDispPars(const LMPropSpecificDispPars& pars)
{
    BufferStringSet propnms;
    for ( int idx=0; idx<layerModel().propertyRefs().size(); idx++ )
	propnms.add( layerModel().propertyRefs()[idx]->name() );

    if ( !propnms.isPresent(pars.propnm_) )
	return false;
    const int propidx = lmdisppars_.indexOf( pars );
    if ( propidx<0 )
	lmdisppars_ += pars;
    else
	lmdisppars_[propidx] = pars;
    return true;
}


//=========================================================================>>


uiStratSimpleLayerModelDisp::uiStratSimpleLayerModelDisp(
		uiStratLayModEditTools& t, const Strat::LayerModelProvider& l )
    : uiStratLayerModelDisp(t,l)
    , emptyitm_(0)
    , zoomboxitm_(0)
    , dispprop_(1)
    , dispeach_(1)
    , zoomwr_(mUdf(double),0,0,0)
    , fillmdls_(false)
    , uselithcols_(true)
    , showzoomed_(true)
    , vrg_(0,1)
    , logblcklineitms_(*new uiGraphicsItemSet)
    , logblckrectitms_(*new uiGraphicsItemSet)
    , lvlitms_(*new uiGraphicsItemSet)
    , contitms_(*new uiGraphicsItemSet)
    , selseqitm_(0)
    , vwr_(* new uiFlatViewer(this))
    , selseqad_(0)
    , selectedlevel_(-1)
    , selectedcontent_(0)
    , allcontents_(false)
{
    vwr_.setInitialSize( uiSize(600,250) );
    vwr_.setStretch( 2, 2 );
    vwr_.disableStatusBarUpdate();
    FlatView::Appearance& app = vwr_.appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x1_.name_ = "Model Nr";
    app.annot_.x2_.name_ = SI().depthsInFeet() ? "Depth (ft)" : "Depth (m)";
    app.ddpars_.wva_.allowuserchange_ = false;
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
    app.annot_.x1_.showannot_ = true;
    app.annot_.x1_.showgridlines_ = false;
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.showannot_ = true;
    app.annot_.x2_.showgridlines_ = true;
    app.ddpars_.show( false, false );
    app.annot_.allowuserchangereversedaxis_ = false;

    emptydp_ = new FlatDataPack( "Layer Model", new Array2DImpl<float>(0,0) );
    DPM( DataPackMgr::FlatID() ).addAndObtain( emptydp_ );
    vwr_.setPack( true, emptydp_->id() );
    vwr_.setPack( false, emptydp_->id() );
    vwr_.rgbCanvas().getMouseEventHandler().buttonReleased.notify(
			mCB(this,uiStratSimpleLayerModelDisp,usrClicked) );
    vwr_.rgbCanvas().getMouseEventHandler().doubleClick.notify(
			mCB(this,uiStratSimpleLayerModelDisp,doubleClicked) );
    vwr_.rgbCanvas().getMouseEventHandler().movement.notify(
			mCB(this,uiStratSimpleLayerModelDisp,mouseMoved) );

    const CallBack redrawcb( mCB(this,uiStratSimpleLayerModelDisp,reDrawCB) );
    const CallBack redrawseqcb(
	    mCB(this,uiStratSimpleLayerModelDisp,reDrawSeqCB) );
    const CallBack redrawlvlcb(
	    mCB(this,uiStratSimpleLayerModelDisp,reDrawLevelsCB) );
    tools_.selPropChg.notify( redrawseqcb );
    tools_.dispLithChg.notify( redrawseqcb );
    tools_.selContentChg.notify( redrawseqcb );
    tools_.selLevelChg.notify( redrawlvlcb );
    tools_.dispEachChg.notify( redrawcb );
    tools_.dispZoomedChg.notify(
	    mCB(this,uiStratSimpleLayerModelDisp,dispZoomedChgCB) );
}


uiStratSimpleLayerModelDisp::~uiStratSimpleLayerModelDisp()
{
    DPM( DataPackMgr::FlatID() ).release( emptydp_ );
    eraseAll();
    delete &lvlitms_;
    delete &logblcklineitms_;
    delete &logblckrectitms_;
}


void uiStratSimpleLayerModelDisp::eraseAll()
{
    logblcklineitms_.erase();
    logblckrectitms_.erase();
    lvlitms_.erase();
    delete emptyitm_; emptyitm_ = 0;
    delete frtxtitm_; frtxtitm_ = 0;
    lvldpths_.erase();
    vwr_.removeAuxDatas( layerads_ );
    deepErase( layerads_ );
    vwr_.removeAuxDatas( levelads_ );
    deepErase( levelads_ );
    delete vwr_.removeAuxData( selseqad_ );
}


uiBaseObject* uiStratSimpleLayerModelDisp::getViewer()
{ return &vwr_; }


uiGraphicsScene& uiStratSimpleLayerModelDisp::scene() const
{
    return const_cast<uiStratSimpleLayerModelDisp*>(this)->
					vwr_.rgbCanvas().scene();
}


int uiStratSimpleLayerModelDisp::getClickedModelNr() const
{
    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    if ( layerModel().isEmpty() || !mevh.hasEvent() || mevh.isHandled() )
	return -1;
    const MouseEvent& mev = mevh.event();
    uiWorld2Ui w2ui;
    vwr_.getWorld2Ui( w2ui );
    const float xsel = w2ui.toWorldX( mev.pos().x );
    int selidx = (int)(ceil( xsel )) - 1;
    if ( selidx < 0 || selidx > layerModel().size() )
	selidx = -1;
    return selidx;
}


void uiStratSimpleLayerModelDisp::mouseMoved( CallBacker* )
{
    IOPar statusbarmsg;
    statusbarmsg.set( "Model Number", getClickedModelNr() );
    const MouseEvent& mev = vwr_.rgbCanvas().getMouseEventHandler().event();
    uiWorld2Ui w2ui;
    vwr_.getWorld2Ui( w2ui );
    float depth = w2ui.toWorldY( mev.pos().y );
    if ( !Math::IsNormalNumber(depth) )
    {
	mDefineStaticLocalObject( bool, havewarned, = false );
	if ( !havewarned )
	    { havewarned = true; pErrMsg("Invalid number from axis handler"); }
	depth = 0;
    }
    statusbarmsg.set( "Depth", depth );
    infoChanged.trigger( statusbarmsg, this );
}


void uiStratSimpleLayerModelDisp::usrClicked( CallBacker* )
{
    const int selidx = getClickedModelNr()-1;
    if ( selidx < 0 ) return;

    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    if ( OD::rightMouseButton(mevh.event().buttonState() ) )
	handleRightClick(selidx);
    else
    {
	selectSequence( selidx );
	sequenceSelected.trigger();
	mevh.setHandled( true );
    }
}


void uiStratSimpleLayerModelDisp::handleRightClick( int selidx )
{
    if ( selidx < 0 || selidx >= layerModel().size() )
	return;

    Strat::LayerSequence& ls = const_cast<Strat::LayerSequence&>(
					layerModel().sequence( selidx ) );
    ObjectSet<Strat::Layer>& lays = ls.layers();
    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    uiWorld2Ui w2ui;
    vwr_.getWorld2Ui( w2ui );
    float zsel = w2ui.toWorldY( mevh.event().pos().y );
    mGetRealZ( zsel );
    mevh.setHandled( true );
    if ( flattened_ )
    {
	const float lvlz = lvldpths_[selidx];
	if ( mIsUdf(lvlz) )
	    return;
	zsel += lvlz;
    }

    const int layidx = ls.layerIdxAtZ( zsel );
    if ( lays.isEmpty() || layidx < 0 )
	return;

    uiMenu mnu( parent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sProperties(false)), 0 );
    mnu.insertItem( new uiAction("Remove layer ..."), 1 );
    mnu.insertItem( new uiAction("Remove this Well"), 2 );
    mnu.insertItem( new uiAction("Dump all wells to file ..."), 3 );
    mnu.insertItem( new uiAction("Add dumped wells from file ..."), 4 );
    const int mnuid = mnu.exec();
    if ( mnuid < 0 ) return;

    Strat::Layer& lay = *ls.layers()[layidx];
    if ( mnuid == 0 )
    {
	uiStratEditLayer dlg( this, lay, ls, true );
	if ( dlg.go() && dlg.isChanged() )
	    forceRedispAll( true );
    }
    else if ( mnuid == 3 || mnuid == 4 )
	doLayModIO( mnuid == 4 );
    else if ( mnuid == 2 )
    {
	const_cast<Strat::LayerModel&>(layerModel()).removeSequence( selidx );
	forceRedispAll( true );
    }
    else
    {

	uiDialog dlg( this, uiDialog::Setup( "Remove a layer",
		                  BufferString("Remove '",lay.name(),"'"),
                                  mODHelpKey(mStratSimpleLayerModDispHelpID)));
	uiGenInput* gi = new uiGenInput( &dlg, uiStrings::sRemove(true), 
                                         BoolInpSpec(true, 
                                         "Only this layer",
                                         "All layers with this ID") );
	if ( dlg.go() )
	    removeLayers( ls, layidx, !gi->getBoolValue() );
    }
}


void uiStratSimpleLayerModelDisp::doLayModIO( bool foradd )
{
    if ( doLayerModelIO(foradd) && foradd )
	forceRedispAll( true );
}


void uiStratSimpleLayerModelDisp::removeLayers( Strat::LayerSequence& seq,
					int layidx, bool doall )
{
    if ( !doall )
    {
	delete seq.layers().removeSingle( layidx );
	seq.prepareUse();
    }
    else
    {
	const Strat::LeafUnitRef& lur = seq.layers()[layidx]->unitRef();
	for ( int ils=0; ils<layerModel().size(); ils++ )
	{
	    Strat::LayerSequence& ls = const_cast<Strat::LayerSequence&>(
						layerModel().sequence( ils ) );
	    bool needprep = false;
	    for ( int ilay=0; ilay<ls.layers().size(); ilay++ )
	    {
		const Strat::Layer& lay = *ls.layers()[ilay];
		if ( &lay.unitRef() == &lur )
		{
		    delete ls.layers().removeSingle( ilay );
		    ilay--; needprep = true;
		}
	    }
	    if ( needprep )
		ls.prepareUse();
	}
    }

    forceRedispAll( true );
}


void uiStratSimpleLayerModelDisp::doubleClicked( CallBacker* )
{
    const int selidx = getClickedModelNr()-1;
    if ( selidx < 0 ) return;

    // Should we do something else than edit?
    handleRightClick(selidx);
}


void uiStratSimpleLayerModelDisp::forceRedispAll( bool modeledited )
{
    reDrawCB( 0 );
    if ( modeledited )
	modelEdited.trigger();
}


void uiStratSimpleLayerModelDisp::dispZoomedChgCB( CallBacker* )
{
    mDynamicCastGet(uiMultiFlatViewControl*,stdctrl,vwr_.control())
    if ( stdctrl )
    {
	stdctrl->setZoomCoupled( tools_.dispZoomed() );
	stdctrl->setDrawZoomBoxes( !tools_.dispZoomed() );
    }
}


void uiStratSimpleLayerModelDisp::reDrawLevelsCB( CallBacker* )
{
    if ( flattened_ )
    {
	updateDataPack();
	updateLayerAuxData();
    }
    else
	getBounds();
    updateLevelAuxData();
    vwr_.handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiStratSimpleLayerModelDisp::reDrawSeqCB( CallBacker* )
{
    getBounds();
    updateLayerAuxData();
    vwr_.handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiStratSimpleLayerModelDisp::reDrawCB( CallBacker* )
{
    layerModel().prepareUse();
    if ( layerModel().isEmpty() )
    {
	if ( !emptyitm_ )
	    emptyitm_ = vwr_.rgbCanvas().scene().addItem(
		    		new uiTextItem( "<---empty--->",
				mAlignment(HCenter,VCenter) ) );
	emptyitm_->setPenColor( Color::Black() );
	emptyitm_->setPos( uiPoint( vwr_.rgbCanvas().width()/2,
		    		    vwr_.rgbCanvas().height() / 2 ) );
	return;
    }

    delete emptyitm_; emptyitm_ = 0;
    doDraw();
}


void uiStratSimpleLayerModelDisp::setZoomBox( const uiWorldRect& wr )
{
}


float uiStratSimpleLayerModelDisp::getDisplayZSkip() const
{
    if ( layerModel().isEmpty() ) return 0;
    return layerModel().sequence(0).startDepth();
}


void uiStratSimpleLayerModelDisp::updZoomBox()
{
}


#define mStartLayLoop(chckdisp,perseqstmt) \
    const int nrseqs = layerModel().size(); \
    for ( int iseq=0; iseq<nrseqs; iseq++ ) \
    { \
	if ( chckdisp && !isDisplayedModel(iseq) ) continue; \
	const float lvldpth = lvldpths_[iseq]; \
	if ( flattened_ && mIsUdf(lvldpth) ) continue; \
	int layzlvl = 0; \
	const Strat::LayerSequence& seq = layerModel().sequence( iseq ); \
	const int nrlays = seq.size(); \
	perseqstmt; \
	for ( int ilay=0; ilay<nrlays; ilay++ ) \
	{ \
	    layzlvl++; \
	    const Strat::Layer& lay = *seq.layers()[ilay]; \
	    float z0 = lay.zTop(); if ( flattened_ ) z0 -= lvldpth; \
	    float z1 = lay.zBot(); if ( flattened_ ) z1 -= lvldpth; \
	    const float val = \
	      getLayerPropValue(lay,seq.propertyRefs()[dispprop_],dispprop_); \

#define mEndLayLoop() \
	} \
    }



void uiStratSimpleLayerModelDisp::updateSelSeqAuxData()
{
    if ( !selseqad_ )
    {
	selseqad_ = vwr_.createAuxData( 0 );
	selseqad_->enabled_ = true;
	selseqad_->linestyle_ = LineStyle( LineStyle::Dot, 2, Color::Black() );
	selseqad_->zvalue_ = uiFlatViewer::auxDataZVal() + 2;
	vwr_.addAuxData( selseqad_ );
    }

    StepInterval<double> yrg = emptydp_->posData().range( false );
    selseqad_->poly_.erase();
    selseqad_->poly_ += FlatView::Point( mCast(double,selseqidx_+1), yrg.start);
    selseqad_->poly_ += FlatView::Point( mCast(double,selseqidx_+1), yrg.stop );
}


void uiStratSimpleLayerModelDisp::updateLevelAuxData()
{
    if ( layerModel().isEmpty() )
	return;

    lvlcol_ = tools_.selLevelColor();
    int auxdataidx = 0;
    for ( int iseq=0; iseq<lvldpths_.size(); iseq++ )
    {
	if ( !isDisplayedModel(iseq) )
	    continue;
	float zlvl = lvldpths_[iseq];
	if ( mIsUdf(zlvl) )
	    continue;

	mGetDispZ(zlvl);
	const double ypos = mCast( double, flattened_ ? 0. : zlvl );
	const double xpos1 = mCast( double, iseq+1 );
	const double xpos2 = mCast( double, iseq +1+dispeach_ );
	FlatView::AuxData* levelad = 0;
	if ( !levelads_.validIdx(auxdataidx) )
	{
	    levelad = vwr_.createAuxData( 0 );
	    levelad->zvalue_ = uiFlatViewer::auxDataZVal() + 1;
	    vwr_.addAuxData( levelad );
	    levelads_ += levelad;
	}
	else
	    levelad = levelads_[auxdataidx];

	levelad->poly_.erase();
	levelad->close_ = false;
	levelad->enabled_ = true;
	levelad->linestyle_ = LineStyle(LineStyle::Solid,2,lvlcol_);
	levelad->poly_ += FlatView::Point( xpos1, ypos );
	levelad->poly_ += FlatView::Point( xpos2, ypos );
	auxdataidx++;
    }
    while ( auxdataidx < levelads_.size() )
	levelads_[auxdataidx++]->enabled_ = false;
}

void uiStratSimpleLayerModelDisp::updateLayerAuxData()
{
    dispprop_ = tools_.selPropIdx();
    selectedlevel_ = tools_.selLevelIdx();
    dispeach_ = tools_.dispEach();
    showzoomed_ = tools_.dispZoomed();
    uselithcols_ = tools_.dispLith();
    selectedcontent_ = layerModel().refTree().contents()
				.getByName(tools_.selContent());
    allcontents_ = FixedString(tools_.selContent()) == sKey::All();
    if ( vrg_.width() == 0 )
	{ vrg_.start -= 1; vrg_.stop += 1; }
    const float vwdth = vrg_.width();
    int auxdataidx = 0; 
    mStartLayLoop( false, )
	
	if ( !isDisplayedModel(iseq) )
	    continue;
	const Color laycol = lay.dispColor( uselithcols_ );
	bool mustannotcont = false;
	if ( !lay.content().isUnspecified() )
	    mustannotcont = allcontents_
		|| (selectedcontent_ && lay.content() == *selectedcontent_);
	const Color pencol = mustannotcont ? lay.content().color_ : laycol;
	bool canjoinlayers = ilay > 0;
	if ( canjoinlayers )
	{
	    const Strat::Layer& prevlay = *seq.layers()[ilay-1]; 
	    const Color prevlaycol = prevlay.dispColor( uselithcols_ );
	    canjoinlayers = prevlaycol==laycol;
	}

	if ( canjoinlayers )
	    auxdataidx--;
	FlatView::AuxData* layad = 0;
	if ( !layerads_.validIdx(auxdataidx) )
	{
	    layad = vwr_.createAuxData( lay.name().buf() );
	    layad->zvalue_ = uiFlatViewer::auxDataZVal()-1;
	    layad->close_ = true;
	    vwr_.addAuxData( layad );
	    layerads_ += layad;
	}
	else
	    layad = layerads_[auxdataidx];

	if ( !canjoinlayers )
	    layad->poly_.erase();
	else
	    layad->poly_.pop();

	layad->fillcolor_ = laycol;
	layad->enabled_ = true;
	layad->linestyle_ = LineStyle( LineStyle::Solid, 2, pencol );
	if ( mustannotcont )
	    layad->fillpattern_ = lay.content().pattern_;
	else
	{
	    FillPattern fp; fp.setFullFill();
	    layad->fillpattern_ = fp;
	}
	const double x0 = mCast( double, iseq + 1 );
	double relx = mCast( double, (val-vrg_.start)/vwdth );
	relx *= dispeach_;
	const double x1 = mCast( double, iseq+1+relx );
	mGetDispZ( z0 ); // TODO check if needed
	mGetDispZ( z1 );
	if ( !canjoinlayers )
	    layad->poly_ += FlatView::Point( x0, (double)z0 );
	layad->poly_ += FlatView::Point( x1, (double)z0 );
	layad->poly_ += FlatView::Point( x1, (double)z1 );
	layad->poly_ += FlatView::Point( x0, (double)z1 );
	auxdataidx++;

    mEndLayLoop()

    while ( auxdataidx < layerads_.size() )
	layerads_[auxdataidx++]->enabled_ = false;

}

void uiStratSimpleLayerModelDisp::updateDataPack()
{
    getBounds();
    const Strat::LayerModel& lm = layerModel();
    const int nrseqs = lm.size();
    const bool haveprop = lm.propertyRefs().validIdx(dispprop_);
    mGetDispZrg(zrg_,dispzrg);
    StepInterval<double> zrg( dispzrg.start, dispzrg.stop,
	    		      dispzrg.width()/5.0f );
    emptydp_->posData().setRange(
	    true, StepInterval<double>(1, nrseqs<2 ? 1 : nrseqs,1) );
    emptydp_->posData().setRange( false, zrg );
    emptydp_->setName( !haveprop ? "No property selected"
	    			 : lm.propertyRefs()[dispprop_]->name().buf() );
    vwr_.setViewToBoundingBox();
}


void uiStratSimpleLayerModelDisp::modelChanged()
{
    zoomwr_ = uiWorldRect(mUdf(double),0,0,0);
    updateDataPack();
    forceRedispAll();
}



void uiStratSimpleLayerModelDisp::getBounds()
{
    lvldpths_.erase();
    dispprop_ = tools_.selPropIdx();
    const Strat::Level* lvl = tools_.selStratLevel();
    for ( int iseq=0; iseq<layerModel().size(); iseq++ )
    {
	const Strat::LayerSequence& seq = layerModel().sequence( iseq );
	if ( !lvl || seq.isEmpty() )
	    { lvldpths_ += mUdf(float); continue; }

	const int posidx = seq.positionOf( *lvl );
	float zlvl = mUdf(float);
	if ( posidx >= seq.size() )
	    zlvl = seq.layers()[seq.size()-1]->zBot();
	else if ( posidx >= 0 )
	    zlvl = seq.layers()[posidx]->zTop();
	lvldpths_ += zlvl;
    }

    Interval<float> zrg(mUdf(float),mUdf(float)), vrg(mUdf(float),mUdf(float));
    mStartLayLoop( false,  )
#	define mChckBnds(var,op,bnd) \
	if ( (mIsUdf(var) || var op bnd) && !mIsUdf(bnd) ) \
	    var = bnd
	mChckBnds(zrg.start,>,z0);
	mChckBnds(zrg.stop,<,z1);
	mChckBnds(vrg.start,>,val);
	mChckBnds(vrg.stop,<,val);
    mEndLayLoop()
    if ( mIsUdf(zrg.start) )
	zrg_ = Interval<float>( 0, 1 );
    else
	zrg_ = zrg;
    vrg_ = mIsUdf(vrg.start) ? Interval<float>(0,1) : vrg;

    if ( mIsUdf(zoomwr_.left()) )
    {
	zoomwr_.setLeft( 1 );
	zoomwr_.setRight( nrseqs+1 );
	mGetDispZrg(zrg_,dispzrg);
	zoomwr_.setTop( dispzrg.stop );
	zoomwr_.setBottom( dispzrg.start );
    }
}


int uiStratSimpleLayerModelDisp::getXPix( int iseq, float relx ) const
{
    const float margin = 0.05f;
    relx = (1-margin) * relx + margin * .5f;// get relx between 0.025 and 0.975
    relx *= dispeach_;
    uiWorld2Ui w2ui;
    vwr_.getWorld2Ui( w2ui );
    return w2ui.toUiX( iseq + 1 + relx );
}


bool uiStratSimpleLayerModelDisp::isDisplayedModel( int iseq ) const
{
    if ( iseq % dispeach_ )
	return false;

    if ( showzoomed_ )
    {
	uiWorld2Ui w2ui;
	vwr_.getWorld2Ui( w2ui );
	const int xpix0 = getXPix( iseq, 0 );
	const int xpix1 = getXPix( iseq, 1 );
	if ( w2ui.toWorldX(xpix1) > zoomwr_.right()
	  || w2ui.toWorldX(xpix0) < zoomwr_.left() )
	    return false;
    }
    return true;
}


int uiStratSimpleLayerModelDisp::totalNrLayersToDisplay() const
{
    const int nrseqs = layerModel().size();
    int ret = 0;
    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	if ( isDisplayedModel(iseq) )
	    ret += layerModel().sequence(iseq).size();
    }
    return ret;
}


void uiStratSimpleLayerModelDisp::doDraw()
{
    getBounds();
    updateLayerAuxData();
    updateLevelAuxData();
    updateSelSeqAuxData();
    displayFRText();
    vwr_.handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiStratSimpleLayerModelDisp::drawLevels()
{
}


void uiStratSimpleLayerModelDisp::doLevelChg()
{ reDrawCB( 0 ); }


void uiStratSimpleLayerModelDisp::drawSelectedSequence()
{
    updateSelSeqAuxData();
    vwr_.handleChange( mCast(unsigned int, FlatView::Viewer::Auxdata) );
}
