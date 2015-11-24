/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uipixmap.h"

#include "commandlineparser.h"
#include "file.h"
#include "moddepmgr.h"
#include "prog.h"


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );

    OD::ModDeps().ensureLoaded( "uiTools" );

    CommandLineParser clp( argc, argv );
    BufferStringSet args;
    clp.getNormalArguments( args );

    if ( args.isEmpty() || clp.hasKey("help") || clp.hasKey("h") )
    {
	od_cout() << "Usage: " << argv[0]
		<< " filename [title]\nNote: filename has to be with FULL path."
		<< od_endl;
	return ExitProgram( 0 );
    }

    if ( clp.hasKey("bg") )
	ForkProcess();

    BufferString& fnm = args.get( 0 );
#ifdef __win__
    if ( File::isLink(fnm) )
	fnm = File::linkTarget( fnm );
#endif
    if ( !File::exists(fnm.buf()) )
    {
	od_cerr() << "File name does not exist." << od_endl;
	return ExitProgram( 0 );
    }

    const BufferString title = args.size() > 1 ? args.get(1).buf() : fnm.buf();

    uiMain app( argc, argv );
    uiMainWin* mw = new uiMainWin( 0, toUiString(title.buf()) );
    uiPixmap pm( fnm );
    uiGraphicsView* view = new uiGraphicsView( mw, "Graphics Viewer" );
    view->setPrefWidth( pm.width() );
    view->setPrefHeight( pm.height() );
    view->scene().addItem( new uiPixmapItem(pm) );
    app.setTopLevel( mw );
    mw->show();

    return ExitProgram( app.exec() );
}
