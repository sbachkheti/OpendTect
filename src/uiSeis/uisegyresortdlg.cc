/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegyresortdlg.cc,v 1.2 2011-03-23 12:00:18 cvsbert Exp $";

#include "uisegyresortdlg.h"
#include "uiioobjsel.h"
#include "uifileinput.h"
#include "uitaskrunner.h"
#include "uisegydef.h"
#include "uimsg.h"
#include "segyresorter.h"
#include "segydirecttr.h"
#include "survinfo.h"

static const char* sKeySEGYDirect = "SEGYDirect";


uiResortSEGYDlg::uiResortSEGYDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup("Re-sort SEG-Y scanned",
		"Produce new SEG-Y file from scanned data",mTODOHelpID) )
    , geomfld_(0)
    , volfld_(0)
    , ps3dfld_(0)
    , ps2dfld_(0)
    , linesfld_(0)
{
    BufferStringSet geomnms;
    if ( SI().has3D() )
    {
	geomnms.add( Seis::nameOf(Seis::VolPS) );
	geomnms.add( Seis::nameOf(Seis::Vol) );
    }
    if ( SI().has2D() )
	geomnms.add( Seis::nameOf(Seis::LinePS) );

    if ( geomnms.size() > 1 )
    {
	const CallBack geomcb( mCB(this,uiResortSEGYDlg,geomSel) );
	geomfld_ = new uiGenInput( this, "Type", StringListInpSpec(geomnms) );
	geomfld_->valuechanged.notify( geomcb );
	finaliseDone.notify( mCB(this,uiResortSEGYDlg,geomSel) );
    }


#define mDefSeisSelFld(fldnm,geom,trtyp) \
    IOObjContext ctxt##fldnm( mIOObjContext(trtyp) ); \
    ctxt##fldnm.toselect.allowtransls_ = sKeySEGYDirect; \
    uiIOObjSel::Setup ossu##fldnm( "Scanned input" ); \
    ossu##fldnm.filldef( false ); \
    fldnm##fld_ = new uiIOObjSel( this, ctxt##fldnm, ossu##fldnm ); \
    fldnm##fld_->attach( alignedBelow, geomfld_ )

    if ( SI().has3D() )
    {
	mDefSeisSelFld(vol,Vol,SeisTrc);
	mDefSeisSelFld(ps3d,VolPS,SeisPS3D);
	linesfld_ = new uiGenInput( this, "Number of lines per file",
				      IntInpSpec(100) );
	linesfld_->setWithCheck( true );
	linesfld_->setChecked( false );
	linesfld_->attach( alignedBelow, ps3dfld_ );
    }
    if ( SI().has2D() )
    {
	mDefSeisSelFld(ps2d,LinePS,SeisPS2D);
    }

    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.forread( false ).filter( uiSEGYFileSpec::fileFilter() );
    outfld_ = new uiFileInput( this, "Output file(s)", fisu );
    outfld_->attach( alignedBelow, ps2dfld_ ? ps2dfld_ : ps3dfld_ );
    if ( linesfld_ )
	outfld_->attach( ensureBelow, linesfld_ );
}


void uiResortSEGYDlg::geomSel( CallBacker* )
{
    if ( !geomfld_ )
	return;

    uiIOObjSel* curos = objSel();
#define mDispFld(nm) \
    if ( nm##fld_ ) nm##fld_->display( nm##fld_ == curos )
    mDispFld(vol); mDispFld(ps3d); mDispFld(ps2d);
}


Seis::GeomType uiResortSEGYDlg::geomType() const
{
    return geomfld_ ? Seis::geomTypeOf( geomfld_->text() ) : Seis::LinePS; 
}


uiIOObjSel* uiResortSEGYDlg::objSel()
{
    return geomfld_ ? (geomType() == Seis::VolPS ? ps3dfld_ : volfld_)
		    : ps2dfld_;
}


bool uiResortSEGYDlg::acceptOK( CallBacker* )
{
    const IOObj* ioobj = objSel()->ioobj();
    if ( !ioobj )
	return false;

    const char* fnm = outfld_->fileName();
    if ( !fnm || !*fnm )
    {
	uiMSG().error( "Please enter the output file name" );
	return false;
    }

    SEGY::ReSorter::Setup su( geomType(), ioobj->key(), fnm );
    if ( linesfld_ && linesfld_->isChecked() )
	su.nridxsperfile( linesfld_->getIntValue() );

    SEGY::ReSorter sr( su );
    uiTaskRunner tr( this );
    return tr.execute( sr );
}
