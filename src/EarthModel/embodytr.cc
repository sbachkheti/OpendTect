/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: embodytr.cc,v 1.8 2012-05-02 11:53:05 cvskris Exp $";

#include "embodytr.h"
#include "embody.h"

int EMBodyTranslatorGroup::selector( const char* s )
{
    int res = defaultSelector( EMBodyTranslatorGroup::sKeyword(), s );
    if ( res==mObjSelUnrelated )
	res = defaultSelector( "MarchingCubesSurface", s );

    return res;
}



const IOObjContext& EMBodyTranslatorGroup::ioContext()
{
    static PtrMan<IOObjContext> ctxt = 0;
    if ( !ctxt )
    {
	ctxt = new IOObjContext( 0 );
	ctxt->stdseltype = IOObjContext::Surf;
	ctxt->toselect.allownonreaddefault_ = true;
    }

    ctxt->trgroup = &theInst();
    return *ctxt;
}
