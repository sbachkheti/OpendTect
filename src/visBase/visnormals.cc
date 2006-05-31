/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2002
 RCS:           $Id: visnormals.cc,v 1.8 2006-05-31 12:42:53 cvskris Exp $
________________________________________________________________________

-*/

#include "visnormals.h"

#include "errh.h"
#include "trigonometry.h"
#include "thread.h"

#include <Inventor/nodes/SoNormal.h>

mCreateFactoryEntry( visBase::Normals );

namespace visBase
{

Normals::Normals()
    : normals( new SoNormal )
    , mutex( *new Threads::Mutex )
{
    normals->ref();
    unusednormals += 0;
    //!<To compensate for that the first coord is set by default by coin
}


Normals::~Normals()
{
    normals->unref();
    delete &mutex;
}


void Normals::setNormal( int idx, const Vector3& normal )
{
    Threads::MutexLocker lock( mutex );

    for ( int idy=normals->vector.getNum(); idy<idx; idy++ )
	unusednormals += idy;

    normals->vector.set1Value( idx, SbVec3f( normal.x, normal.y, normal.z ));
}


int Normals::addNormal( const Vector3& normal )
{
    Threads::MutexLocker lock( mutex );
    const int res = getFreeIdx();
    normals->vector.set1Value( res, SbVec3f( normal.x, normal.y, normal.z ));
    return res;
}


void Normals::removeNormal(int idx)
{
    Threads::MutexLocker lock( mutex );
    const int nrnormals = normals->vector.getNum();
    if ( idx>=nrnormals )
    {
	pErrMsg("Invalid index");
	return;
    }
    
    if ( idx==nrnormals-1 )
    {
	normals->vector.deleteValues( idx );
    }
    else
    {
	unusednormals += idx;
    }
}


SoNode* Normals::getInventorNode()
{ return normals; }


int  Normals::getFreeIdx()
{
    if ( unusednormals.size() )
    {
	const int res = unusednormals[unusednormals.size()-1];
	unusednormals.remove(unusednormals.size()-1);
	return res;
    }

    return normals->vector.getNum();
}

}; // namespace visBase
