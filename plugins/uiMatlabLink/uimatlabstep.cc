/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uimatlabstep.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "matlabstep.h"
#include "matlablibmgr.h"

namespace VolProc
{

uiStepDialog* uiMatlabStep::createInstance( uiParent* p, Step* step )
{
    mDynamicCastGet(MatlabStep*,ms,step);
    return ms ? new uiMatlabStep(p,ms) : 0; 
}

uiMatlabStep::uiMatlabStep( uiParent* p, MatlabStep* step )
    : uiStepDialog(p,MatlabStep::sFactoryDisplayName(), step )
{
    choicefld_ = new uiGenInput( this, "Type",
			BoolInpSpec(true,"m-file","shared object file") );

    filefld_ = new uiFileInput( this, "Select" );
    filefld_->attach( alignedBelow, choicefld_ );
    filefld_->setFileName( step->sharedLibFileName() );

    addNameFld( filefld_ );
}


bool uiMatlabStep::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK(cb) )
	return false;

    mDynamicCastGet(MatlabStep*,step,step_)
    if ( !step ) return false;

    if ( !MLM().load(filefld_->fileName()) )
    {
	const FixedString errmsg = MLM().errMsg();
	if ( !errmsg.isEmpty() )
	    uiMSG().error( errmsg );
	return false;
    }

    step->setSharedLibFileName( filefld_->fileName() );

    return true;
}

} // namespace VolProc
