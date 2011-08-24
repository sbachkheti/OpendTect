/*+
_______________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR:	Kristofer Tingdahl
 DATE:		May 2011
_______________________________________________________________________________

 -*/
static const char* rcsID = "$Id: uivoxelconnectivityfilter.cc,v 1.2 2011-08-24 12:32:24 cvskris Exp $";

#include "uivoxelconnectivityfilter.h"

#include "datainpspec.h"
#include "voxelconnectivityfilter.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uivolprocchain.h"

namespace VolProc
{

uiStepDialog* uiVoxelConnectivityFilter::createInstance( uiParent* p,
							 Step* step )
{
    mDynamicCastGet( VoxelConnectivityFilter*, vcf, step );
    return vcf 
	?  new uiVoxelConnectivityFilter( p, vcf )
	: 0; 
}


#define mCutBelow 0
#define mCutAbove 1
#define mCutOutside 2


uiVoxelConnectivityFilter::uiVoxelConnectivityFilter( uiParent* p,
	VoxelConnectivityFilter* step )
    : uiStepDialog( p, VoxelConnectivityFilter::sFactoryDisplayName(), step )
{
    const char* cutofftypes[] = { "Input less than", "Input more than", "Input outside", 0 };
    cutofftypefld_ = new uiGenInput( this, "Clip",
                              StringListInpSpec( cutofftypes ) );
    const Interval<float>& acceptrange = step->getAcceptRange();

    cutoffvalfld_ = new uiGenInput( this, "Value", FloatInpSpec() );
    cutoffvalfld_->attach( alignedBelow, cutofftypefld_ );
    cutoffrangefld_ = new uiGenInput( this, "Range", FloatInpIntervalSpec(false) );
    cutoffrangefld_->attach( alignedBelow, cutofftypefld_ );
    cutoffrangefld_->setValue( acceptrange );

    if ( !mIsUdf(acceptrange.start) && !mIsUdf(acceptrange.stop) )
    {
	cutofftypefld_->setValue( mCutOutside );
    }
    else if ( !mIsUdf(acceptrange.start) )
    {
	cutofftypefld_->setValue( mCutBelow );
	cutoffvalfld_->setValue( acceptrange.start );
    }
    else
    {
	cutofftypefld_->setValue( mCutAbove );
	cutoffvalfld_->setValue( acceptrange.stop );
    }

    cutofftypefld_->valuechanged.notify(
          mCB( this, uiVoxelConnectivityFilter, updateFieldsCB) );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, cutoffrangefld_ );

    connectivityfld_ = new uiGenInput( this, "Connectivity",
	    StringListInpSpec( VoxelConnectivityFilter::ConnectivityNames() ) );
    connectivityfld_->setValue( (int) step->getConnectivity() );
    connectivityfld_->attach( alignedBelow, cutoffrangefld_ );
    connectivityfld_->attach( ensureBelow, sep );

    minbodysizefld_ = new uiGenInput( this, "Minimum body size",
	    IntInpSpec( step->getMinimumBodySize() ) );
    minbodysizefld_->attach( alignedBelow, connectivityfld_ );

    acceptoutputfld_ = new uiGenInput( this, "Inside",
	    StringListInpSpec( VoxelConnectivityFilter::AcceptOutputNames() ) );
    acceptoutputfld_->setValue( (int) step->getAcceptOutput() );
    acceptoutputfld_->valuechanged.notify(
	    mCB( this, uiVoxelConnectivityFilter, updateFieldsCB) );
    acceptoutputfld_->attach( alignedBelow, minbodysizefld_ );

    acceptvaluefld_ = new uiGenInput( this, "Inside value",
	    FloatInpSpec( step->getAcceptValue() ) );
    acceptvaluefld_->attach( alignedBelow, acceptoutputfld_ );

    rejectoutputudffld_ = new uiGenInput( this, "Outside",
	    BoolInpSpec(mIsUdf(step->getRejectValue() ),
			"Undefined value", "Value" ) );
    rejectoutputudffld_->attach( alignedBelow, acceptvaluefld_ );
    rejectoutputudffld_->valuechanged.notify(
	    mCB( this, uiVoxelConnectivityFilter, updateFieldsCB) );

    rejectoutputvalfld_ = new uiGenInput( this, "Outside value",
	    FloatInpSpec(step->getRejectValue() ) );
    rejectoutputvalfld_->attach( alignedBelow, rejectoutputudffld_ );

    updateFieldsCB( 0 );
}


void uiVoxelConnectivityFilter::updateFieldsCB( CallBacker* )
{
    cutoffvalfld_->display( cutofftypefld_->getIntValue()!=mCutOutside );
    cutoffrangefld_->display( cutofftypefld_->getIntValue()==mCutOutside );

    VoxelConnectivityFilter::AcceptOutput output;
    VoxelConnectivityFilter::parseEnumAcceptOutput( acceptoutputfld_->text(),
	     					    output );
    acceptvaluefld_->display( output==VoxelConnectivityFilter::Value );
    rejectoutputvalfld_->display( !rejectoutputudffld_->getBoolValue() );
}


bool uiVoxelConnectivityFilter::acceptOK( CallBacker* )
{
    Interval<float> range;
    if ( cutofftypefld_->getIntValue()==mCutOutside )
    {
	range = cutoffrangefld_->getFInterval();
	if ( mIsUdf(range.start) || mIsUdf(range.stop) )
	{
	    uiMSG().error("Cut range not set");
	    return false;
	}
    }
    else 
    {
	const float cutoffval = cutoffvalfld_->getfValue();
	if ( mIsUdf(cutoffval) )
	{
	    uiMSG().error( "Cut value value not set" );
	    return false;
	}

	if ( cutofftypefld_->getIntValue()==mCutBelow )
	{
	    range.start = cutoffval;
	    range.stop = mUdf(float);
	}
	else
	{
	    range.start = mUdf(float);
	    range.stop = cutoffval;
	}
    }

    if ( mIsUdf(minbodysizefld_->getIntValue() ) ||
         minbodysizefld_->getIntValue()<1 )
    {
	uiMSG().error("Minimum size is not set or is less than 1.");
	return false;
    }

    VoxelConnectivityFilter::AcceptOutput output;
    VoxelConnectivityFilter::parseEnumAcceptOutput( acceptoutputfld_->text(),
	     					    output );

    if ( output==VoxelConnectivityFilter::Value &&
	 mIsUdf(acceptvaluefld_->getfValue() ) )
    {
	uiMSG().error("Accept-value not set");
    }

    if ( !rejectoutputudffld_->getBoolValue() &&
	    mIsUdf(rejectoutputvalfld_->getfValue() ) )
    {
	uiMSG().error("Rejection value is not set");
	return false;
    }

    mDynamicCastGet( VoxelConnectivityFilter*, step, step_ );
    step->setAcceptRange(range);
    step->setConnectivity( (VoxelConnectivityFilter::Connectivity)
			    connectivityfld_->getIntValue() );
    step->setMinimumBodySize( minbodysizefld_->getIntValue() );
    step->setAcceptOutput( output );

    if ( output==VoxelConnectivityFilter::Value )
	 step->setAcceptValue( acceptvaluefld_->getfValue() );

    step->setRejectValue( rejectoutputudffld_->getBoolValue()
	   ? mUdf(float)
	   : rejectoutputvalfld_->getfValue() );

    return true;
}


}; //Namespace
