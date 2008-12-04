#ifndef uivisisosurface_h
#define uivisisosurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uivisisosurface.h,v 1.7 2008-12-04 17:31:25 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiAxisHandler;
class uiFunctionDisplay;
class uiGenInput;
class uiIOObjSel;
class uiPushButton;
class uiStatsDisplay;
namespace visBase { class MarchingCubesSurface; }
namespace visSurvey { class VolumeDisplay; }
template <class T> class SamplingData;
template <class T> class TypeSet;

/*!\brief Dialog to set isovalue of an isosurface.  */

class uiVisIsoSurfaceThresholdDlg : public uiDlgGroup
{
public:
    		uiVisIsoSurfaceThresholdDlg(uiParent*,
			visBase::MarchingCubesSurface*,
			visSurvey::VolumeDisplay*,bool couldchoosemode);
		/*<If couldchoosemode is on, you have the choice of full mode 
		   or picks mode, otherwise, only have isovalue setting.*/
		~uiVisIsoSurfaceThresholdDlg();
    bool	acceptOK();
    bool	rejectOK();
    bool	revertChanges();
    void	drawHistogram();

protected:

    void		updatePressed(CallBacker*);
    void		mousePressed(CallBacker*);
    void		modeChangeCB(CallBacker*);
    void		doubleClick(CallBacker*);
    void		handleClick(CallBacker*,bool isdouble );
    void		updateIsoDisplay(float nv);

    uiStatsDisplay*	statsdisplay_;
    uiGenInput*		modefld_;
    uiGenInput*		thresholdfld_;
    uiGenInput*		aboveisovaluefld_;
    uiIOObjSel*		ioobjselfld_;
    uiPushButton*	updatebutton_;

    visBase::MarchingCubesSurface*	isosurfacedisplay_;
    visSurvey::VolumeDisplay*		vd_;
    float				initialvalue_;
    bool				usemode_;

    uiAxisHandler&	xAxis();
    uiFunctionDisplay&	funcDisp();
};

#endif
