/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/

static const char* rcsID = "$Id: timefun.cc,v 1.10 2004-10-06 12:03:26 dgb Exp $";

#include "timefun.h"
#include <time.h>

#if defined(sgi) || defined (sun5) || defined(mac)
# define __notimeb__ 1
#endif

#ifndef __notimeb__
# ifdef __win__
#  include <windows.h>
# endif
# include <sys/timeb.h>
#endif

#ifdef __notimeb__
static time_t tim;
#else
# ifdef __win__
static struct _timeb timebstruct;
# else
static struct timeb timebstruct;
# endif
#endif
static struct tm* ptrtm;


static struct tm* getLocal( void )
{
#ifdef __notimeb__

    (void)time( &tim ) ;
    return localtime( &tim );

#else

# ifdef __win__

    (void)_ftime( &timebstruct ) ;

# else

    (void)ftime( &timebstruct ) ;

# endif

    return localtime( &timebstruct.time );

#endif
}


int Time_getMilliSeconds( void )
{
    ptrtm = getLocal();

    return

#ifdef __notimeb__

    0

#else

    timebstruct.millitm

#endif

    + ptrtm->tm_sec	* 1000
    + ptrtm->tm_min	* 60000
    + ptrtm->tm_hour	* 3600000;

}


const char* Time_getLocalString( void )
{
    char *chp ;
    int lastch ;

    ptrtm = getLocal() ;
    chp = asctime( ptrtm ) ;

    lastch = strlen( chp ) - 1 ;
    if ( chp[lastch] == '\n' ) chp[lastch] = '\0' ;

    return chp;
}


void Time_sleep( double s )
{
#ifdef __notimeb__

    if ( s > 0 ) sleep( mNINT(s) );

#else

# ifdef __win__

    double ss = s*1000;
    Sleep( (DWORD)mNINT(ss) );

# else

    struct timespec ts;
    if ( s <= 0 ) return;

    ts.tv_sec = (time_t)s;
    ts.tv_nsec = (long)((((double)s - ts.tv_sec) * 1000000000L) + .5);

    nanosleep( &ts, &ts );

# endif

#endif
}
