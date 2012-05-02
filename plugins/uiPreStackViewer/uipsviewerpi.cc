/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* mUnusedVar rcsID = "$Id: uipsviewerpi.cc,v 1.14 2012-05-02 11:52:52 cvskris Exp $";

#include "odplugin.h"
#include "uiodmain.h"
#include "uipsviewermanager.h"
#include "uiprestacktreeitemmgr.h"
#include "visprestackviewer.h"



mDefODPluginInfo(uiPreStackViewer)
{
    static PluginInfo retpi = {
    "Pre-Stack Viewer",
    "dGB - Kristofer/Yuancheng",
    "1.1.1",
    "This is the PreStack Viewer in the 3D scene."
	"\nIt can be activated by right-clicking on a plane in the scene." };
    return &retpi;
}


mDefODInitPlugin(uiPreStackViewer)
{
    PreStackView::Viewer3D::initClass();
    static PreStackView::uiViewer3DMgr* mgr=0;
    if ( mgr ) return 0;
    mgr = new PreStackView::uiViewer3DMgr();
    uiPreStackTreeItemManager* treemgr =  new
	uiPreStackTreeItemManager( *ODMainWin() );
    return 0; 
}
