/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uivelocitygridder.h"

#include "gridder2d.h"
#include "velocityfunction.h"
#include "velocitygridder.h"
#include "volprocchain.h"

#include "uigeninput.h"
#include "uigridder2d.h"
#include "uiinterpollayermodel.h"
#include "uilabel.h"
#include "uiselectvelocityfunction.h"
#include "uivolprocchain.h"

#include "od_helpids.h"

namespace VolProc
{

uiStepDialog* uiVelocityGridder::createInstance( uiParent* p, Step* ro,
						bool is2d )
{
    mDynamicCastGet(VelocityGridder*,gridop,ro);
    if ( !gridop ) return 0;

    return new uiVelocityGridder( p, gridop, is2d );
}


uiVelocityGridder::uiVelocityGridder( uiParent* p, VelocityGridder* ro,
					bool is2d )
    : uiStepDialog( p, VelocityGridder::sFactoryDisplayName(), ro, is2d )
    , operation_( ro )
{
    setHelpKey( mODHelpKey(mVelocityGridderHelpID) );

    layermodelfld_ = new uiInterpolationLayerModel( this );
    layermodelfld_->setModel( ro->getLayerModel() );

    griddersel_ = new uiGridder2DSel( this, ro->getGridder(),
				      ro->getTrendOrder() );
    griddersel_->attach( alignedBelow, layermodelfld_ );

    uiLabel* label = new uiLabel( this, tr("Velocity sources") );
    label->attach( alignedBelow, griddersel_ );

    velfuncsel_ = new Vel::uiFunctionSel( this, operation_->getSources(), 0 );
    velfuncsel_->attach( alignedBelow, label );
    velfuncsel_->listChange.notify( mCB(this,uiVelocityGridder,sourceChangeCB));

    addNameFld( velfuncsel_ );

    namenotset_ = namefld_->isUndef();
    if ( namenotset_ )
    {
	namefld_->valuechanged.notify(
		mCB(this,uiVelocityGridder,nameChangeCB) );
    }
}


void uiVelocityGridder::nameChangeCB( CallBacker* )
{
    namenotset_ = false;
}


void uiVelocityGridder::sourceChangeCB( CallBacker* )
{
    if ( namenotset_ && velfuncsel_->getVelSources().size()==1 )
    {
	NotifyStopper ns( namefld_->valuechanged );
	namefld_->setText( velfuncsel_->getVelSources()[0]->userName() );
    }
}


bool uiVelocityGridder::acceptOK()
{
    if ( !uiStepDialog::acceptOK() )
	return false;

    if ( !operation_ ) return true;

    operation_->setSources( velfuncsel_->getVelSources() );
    IOPar par;
    griddersel_->fillPar( par, true );
    operation_->setGridder( par );
    operation_->setLayerModel( layermodelfld_->getModel() );

    return true;
}

} // namespace VolProc
