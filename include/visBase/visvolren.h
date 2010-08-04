#ifndef visvolren_h
#define visvolren_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.h,v 1.6 2010-08-04 14:49:36 cvsbert Exp $
________________________________________________________________________

-*/


#include "visobject.h"

class SoROI;
class SoVolumeRender;

namespace visBase
{

mClass VolrenDisplay : public visBase::VisualObjectImpl
{
public:
    static VolrenDisplay*	create()
				mCreateDataObj(VolrenDisplay);

protected:
    SoROI*			roi;
    SoVolumeRender*		volren;
};

}; //Namespace

#endif
