/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Aug 2001
 RCS:		$Id: od_SEGYExaminer.cc,v 1.14 2008-09-15 10:10:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyexamine.h"

#include "uimain.h"
#include "prog.h"
#include <unistd.h>
#include <iostream>

#ifdef __win__
#include "filegen.h"
#endif


int main( int argc, char ** argv )
{
    bool dofork = !__ismac__ && !__iswin__;

    uiSEGYExamine::Setup su;
    int argidx = 1;
    while ( argc > argidx
	 && *argv[argidx] == '-' && *(argv[argidx]+1) == '-' )
    {
	if ( !strcmp(argv[argidx],"--ns") )
	    { argidx++; su.fp_.ns_ = atoi( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--fmt") )
	    { argidx++; su.fp_.fmt_ = atoi( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--nrtrcs") )
	    { argidx++; su.nrtrcs_ = atoi( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--filenrs") )
	    { argidx++; su.fs_.getMultiFromString( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--swapbytes") )
	    { su.fp_.byteswapped_ = true; }
	else if ( !strcmp(argv[argidx],"--fg") )
	    dofork = false;
	else
	    { std::cerr << "Ignoring option: " << argv[argidx] << std::endl; }

	argidx++;
    }

    if ( argc <= argidx )
    {
	std::cerr << "Usage: " << argv[0]
		  << "\n\t[--ns #samples]""\n\t[--nrtrcs #traces]"
		     "\n\t[--fmt segy_format_number]"
	    	     "\n\t[--filenrs start`stop`step[`nrzeropad]]"
		     "\n\t[--swapbytes]"
		     "\n\tfilename\n"
	     << "Note: filename must be with FULL path." << std::endl;
	ExitProgram( 1 );
    }

    const int forkres = dofork ? fork() : 0;
    switch ( forkres )
    {
    case -1:
	std::cerr << argv[0] << ": cannot fork: " << errno_message()
	    	  << std::endl;
	ExitProgram( 1 );
    case 0:	break;
    default:	return 0;
    }

    char* fnm = argv[argidx];
    replaceCharacter( fnm, (char)128, ' ' );

    uiMain app( argc, argv );

#ifdef __win__
    if ( File_isLink( fnm ) )
	fnm = const_cast<char*>(File_linkTarget(fnm));
#endif

    su.fs_.fname_ = fnm;
    uiSEGYExamine* sgyex = new uiSEGYExamine( 0, su );
    app.setTopLevel( sgyex );
    sgyex->show();
    ExitProgram( app.exec() ); return 0;
}
