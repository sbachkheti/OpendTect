#pragma once
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
 _______________________________________________________________________

      -*/


#include "uiwellattribmod.h"
#include "uidialog.h"
class StratSynth;
class SyntheticData;
class StratSynthLevel;
class uiGroup;
class uiGenInput;
class uiSeisSel;
class uiIOObjSel;
class uiPickSetIOObjSel;
class uiSeis2DLineNameSel;
class uiStratSynthOutSel;

namespace Geometry { class RandomLine; }
namespace PosInfo { class Line2DData; }

mExpClass(uiWellAttrib) uiStratSynthExport : public uiDialog
{ mODTextTranslationClass(uiStratSynthExport)
public:
    enum GeomSel	{ StraightLine, Polygon, RandomLine, Existing };

			uiStratSynthExport(uiParent*,const StratSynth&);
			~uiStratSynthExport();


protected:

    uiGenInput*		crnewfld_;
    uiSeisSel*		linesetsel_;
    uiGenInput*	newlinenmfld_;
    uiSeis2DLineNameSel* existlinenmsel_;
    uiGroup*		geomgrp_;
    uiGenInput*		geomsel_;
    uiGenInput*		coord0fld_;
    uiGenInput*		coord1fld_;
    uiPickSetIOObjSel*	picksetsel_;
    uiIOObjSel*		randlinesel_;
    uiStratSynthOutSel*	poststcksel_;
    uiStratSynthOutSel*	horsel_;
    uiStratSynthOutSel*	prestcksel_;
    uiGenInput*		prefxfld_;
    uiGenInput*		postfxfld_;

    const StratSynth&	ss_;
    ObjectSet<const SyntheticData> postsds_;
    ObjectSet<const SyntheticData> presds_;
    ObjectSet<StratSynthLevel> sslvls_;

    BufferString	getWinTitle(const StratSynth&) const;
    GeomSel		selType() const;
    void		addPrePostFix(BufferString&) const;
    void		fillGeomGroup();
    void		getExpObjs();
    void		removeNonSelected();
    bool		createHor2Ds();
    Pos::GeomID		getGeometry(PosInfo::Line2DData&);
    void		create2DGeometry(const TypeSet<Coord>&,
					 PosInfo::Line2DData&);

    void		crNewChg(CallBacker*);
    void		geomSel(CallBacker*);

    bool		acceptOK();

};
