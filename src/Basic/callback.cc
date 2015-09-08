/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "callback.h"
#include "thread.h"
#include "threadlock.h"
#include "ptrman.h"

#define mOneMilliSecond 0.001

#ifndef OD_NO_QT
#include <QCoreApplication>

class QEventLoopReceiver : public QObject
{
public:
    QEventLoopReceiver()
	: receiverlock_( true )
    { cbers_.allowNull( true ); }

    bool event( QEvent* )
    {
	Threads::Locker locker( receiverlock_ );
	for ( int idx=0; idx<cbs_.size(); idx++ )
	{
	    cbs_[idx].doCall( cbers_[idx] );
	}

	cbs_.erase();
	cbers_.erase();


	return true;
    }

    void add( CallBack& cb, CallBacker* cber )
    {
	Threads::Locker locker( receiverlock_ );

	if ( cbs_.isEmpty() )
	{
	    QCoreApplication::postEvent( this, new QEvent(QEvent::None) );
	}

	cbs_ += cb;
	cbers_ += cber;
    }

    void remove( CallBack& cb )
    {
	Threads::Locker locker( receiverlock_ );
	const int idx = cbs_.indexOf( cb );
	if ( idx>=0 )
	{
	    cbs_.removeSingle( idx );
	    cbers_.removeSingle( idx );
	}
    }

private:

    TypeSet<CallBack>		cbs_;
    ObjectSet<CallBacker>	cbers_;
    Threads::Lock		receiverlock_;
};


static PtrMan<QEventLoopReceiver> currentreceiver = 0;

static QEventLoopReceiver* getQELR()
{
    if ( !currentreceiver )
    {
	QEventLoopReceiver* rec  = new QEventLoopReceiver;
	if ( !currentreceiver.setIfNull( rec ) )
	    delete rec;
    }

    return currentreceiver;
}

#endif // OD_NO_QT



CallBacker::CallBacker()
{}


CallBacker::CallBacker( const CallBacker& )
{}


CallBacker::~CallBacker()
{
    if ( attachednotifiers_.size() )
    {
	pErrMsg("Notifiers not detached.");
	/* Notifiers should be removed in the class where they were attached,
	   normally by calling detachAllNotifiers in the destructor of that
	   class.
	   If not done, they may still get callbacks after the destuctor
	   is called, but before this destructor is called.
	 */

	//Remove them now.
	detachAllNotifiers();
    }
}


void CallBacker::detachAllNotifiers()
{
    /*Avoid deadlocks (will happen if one thread deletes the notifier while
     the other thread deletes the callbacker at the same time) by using
     try-locks and retry after releasing own lock. */

    Threads::Locker lckr( attachednotifierslock_ );

    while ( attachednotifiers_.size() )
    {
	for ( int idx=attachednotifiers_.size()-1; idx>=0; idx-- )
	{
	    NotifierAccess* na = attachednotifiers_[idx];
	    if ( na->removeWith( this, false ) &&
		na->removeShutdownSubscription( this, false ) )
		attachednotifiers_.removeSingle( idx );

	}

	if ( attachednotifiers_.size() )
	{
	    lckr.unlockNow();
	    Threads::sleep( mOneMilliSecond );
	    lckr.reLock();
	}
    }
}


bool CallBacker::attachCB(NotifierAccess* notif,const CallBack& cb,
	bool onlyifnew)
{
    return notif
	? attachCB(*notif,cb,onlyifnew)
	: false;
}


bool CallBacker::attachCB(NotifierAccess& notif, const CallBack& cb,
			  bool onlyifnew )
{
    CallBacker* cbobj = const_cast<CallBacker*>( cb.cbObj() );
    if ( cbobj!=this )
    {
	pErrMsg("You can only attach a callback to yourself" );
	return false;
    }

    if ( onlyifnew )
    {
	if ( !notif.notifyIfNotNotified( cb ) )
	    return false;
    }
    else
    {
	notif.notify( cb );
    }

    //If the notifier is belonging to me, it will only be messy if
    // we subscribe to the shutdown messages.
    if ( notif.cber_!=this )
	notif.addShutdownSubscription( this );

    Threads::Locker lckr( attachednotifierslock_ );
    if ( !attachednotifiers_.isPresent( &notif ) )
	attachednotifiers_ += &notif;

    return true;
}


void CallBacker::detachCB( NotifierAccess& notif, const CallBack& cb )
{
    if ( cb.cbObj()!=this )
    {
	pErrMsg( "You can only detach a callback to yourself" );
	return;
    }

    Threads::Locker lckr( attachednotifierslock_ );
    if ( !attachednotifiers_.isPresent( &notif ) )
    {
	//It may be deleted. Don't touch it
	return;
    }

    notif.remove( cb );

    if ( !notif.willCall( this ) )
    {
	while ( attachednotifiers_.isPresent( &notif ) )
	{
            if ( notif.removeShutdownSubscription( this, false ) )
            {
                attachednotifiers_ -= &notif;
            }
            else
            {
                lckr.unlockNow();
                Threads::sleep( mOneMilliSecond );
                lckr.reLock();
            }
	}
    }
}


bool CallBacker::isNotifierAttached( NotifierAccess* na ) const
{
    Threads::Locker lckr( attachednotifierslock_ );
    return attachednotifiers_.isPresent( na );
}


#define mGetLocker( thelock, wait ) \
    Threads::Locker lckr( thelock, wait ? Threads::Locker::WaitIfLocked \
					: Threads::Locker::DontWaitForLock ); \
    if ( !lckr.isLocked() ) return false


bool CallBacker::notifyShutdown( NotifierAccess* na, bool wait )
{
    mGetLocker( attachednotifierslock_, wait );
    attachednotifiers_ -= na;
    return true;
}


void CallBack::initClass()
{
#ifndef OD_NO_QT
    getQELR(); //Force creation
#endif
}


void CallBack::doCall( CallBacker* cber )
{
    if ( obj_ && fn_ )
	(obj_->*fn_)( cber );
    else if ( sfn_ )
	sfn_( cber );
}


bool CallBack::addToMainThread( CallBack cb, CallBacker* cber )
{
#ifdef OD_NO_QT
    return false;
#else
    QEventLoopReceiver* rec = getQELR();
    rec->add( cb, cber );
    return true;
#endif
}


bool CallBack::queueIfNotInMainThread( CallBack cb, CallBacker* cber )
{
#ifndef OD_NO_QT
    QCoreApplication* instance = QCoreApplication::instance();
    if ( instance && instance->thread()!=Threads::currentThread() )
    {
	QEventLoopReceiver* rec = getQELR();
	rec->add( cb, cber );
	return true;
    }
#endif

    return false;
}


bool CallBack::callInMainThread( CallBack cb, CallBacker* cber )
{
    if ( addToMainThread( cb, cber ) )
	return false;

    cb.doCall( cber );
    return true;
}


CallBackSet::CallBackSet()
    : lock_(true)
    , enabled_(true)
{}


CallBackSet::CallBackSet( const CallBackSet& cbs )
    : TypeSet<CallBack>( cbs )
    , lock_( true )
    , enabled_( cbs.enabled_ )
{}


CallBackSet::~CallBackSet()
{
}


CallBackSet& CallBackSet::operator=( const CallBackSet& cbs )
{
    Threads::Locker lckr( cbs.lock_ );
    TypeSet<CallBack>::operator=( cbs );
    return *this;
}


void CallBackSet::doCall( CallBacker* obj,
			  CallBacker* exclude )
{
    if ( !enabled_ ) return;

    Threads::Locker lckr( lock_ );
    TypeSet<CallBack> cbscopy = *this;
    lckr.unlockNow();

    for ( int idx=0; idx<cbscopy.size(); idx++ )
    {
	CallBack& cb = cbscopy[idx];
	lckr.reLock();
	if ( !isPresent(cb) )
	    { lckr.unlockNow(); continue; }

	lckr.unlockNow();
	if ( !exclude || cb.cbObj()!=exclude )
	    cb.doCall( obj );
    }
}


bool CallBackSet::doEnable( bool yn )
{
    bool ret = enabled_;
    enabled_ = yn;
    return ret;
}


void CallBackSet::removeWith( CallBacker* cbrm )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.cbObj() == cbrm )
	    { removeSingle( idx ); idx--; }
    }
}


void CallBackSet::removeWith( CallBackFunction cbfn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.cbFn() == cbfn )
	    { removeSingle( idx ); idx--; }
    }
}


void CallBackSet::removeWith( StaticCallBackFunction cbfn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.scbFn() == cbfn )
	    { removeSingle( idx ); idx--; }
    }
}


NotifierAccess::NotifierAccess( const NotifierAccess& na )
    : cber_( na.cber_ )
    , cbs_(*new CallBackSet(na.cbs_) )
{
    cbs_.ref();
}


NotifierAccess::NotifierAccess()
    : cbs_(*new CallBackSet )
{
    cbs_.ref();
}


NotifierAccess::~NotifierAccess()
{
    /*Avoid deadlocks (will happen if one thread deletes the notifier while
     the other thread deletes the callbacker at the same time) by using
     try-locks and retry after releasing own lock. */

    Threads::Locker lckr( shutdownsubscriberlock_ );
    while ( shutdownsubscribers_.size() )
    {
	for ( int idx=shutdownsubscribers_.size()-1; idx>=0; idx-- )
	{
	    if ( shutdownsubscribers_[idx]->notifyShutdown( this, false ) )
		shutdownsubscribers_.removeSingle( idx );
	}

	if ( shutdownsubscribers_.size() )
	{
	    lckr.unlockNow();
	    Threads::sleep( mOneMilliSecond );
	    lckr.reLock();
	}
    }

    cbs_.unRef();
}


void NotifierAccess::addShutdownSubscription( CallBacker* cber )
{
    Threads::Locker lckr( shutdownsubscriberlock_ );
    shutdownsubscribers_.addIfNew( cber );
}


bool NotifierAccess::isShutdownSubscribed( CallBacker* cber ) const
{
    Threads::Locker lckr( shutdownsubscriberlock_ );
    return shutdownsubscribers_.isPresent( cber );
}


bool NotifierAccess::removeShutdownSubscription( CallBacker* cber, bool wait )
{
    mGetLocker( shutdownsubscriberlock_, wait );
    shutdownsubscribers_ -= cber;
    return true;
}


void NotifierAccess::notify( const CallBack& cb, bool first )
{
    Threads::Locker lckr( cbs_.lock_ );

    if ( first )
	cbs_.insert(0,cb);
    else
	cbs_ += cb;
}


bool NotifierAccess::notifyIfNotNotified( const CallBack& cb )
{
    Threads::Locker lckr( cbs_.lock_ );
    return cbs_.addIfNew( cb );
}


void NotifierAccess::remove( const CallBack& cb )
{
    Threads::Locker lckr( cbs_.lock_ );

    cbs_ -= cb;
}


bool NotifierAccess::removeWith( CallBacker* cber, bool wait )
{
    mGetLocker( cbs_.lock_, wait );
    if ( cber_ == cber )
	{ cbs_.erase(); cber_ = 0; return true; }
    cbs_.removeWith( cber );
    return true;
}


bool NotifierAccess::willCall( CallBacker* cber ) const
{
    Threads::Locker lckr( cbs_.lock_ );

    for ( int idx=0; idx<cbs_.size(); idx++ )
    {
        if ( cbs_[idx].cbObj()==cber )
            return true;
    }

    return false;
}


void NotifierAccess::doTrigger( CallBackSet& cbs, CallBacker* c,
				CallBacker* exclude)
{
    cbs.ref();
    cbs.doCall( c, exclude );
    cbs.unRef();
}


NotifyStopper::NotifyStopper( NotifierAccess& na )
    : oldst_(na.disable())
    , thenotif_(na)
{}


NotifyStopper::~NotifyStopper()
{ restore(); }

void NotifyStopper::enable()
{ thenotif_.cbs_.doEnable(false); }


void NotifyStopper::disable()
{ thenotif_.cbs_.doEnable(true); }


void NotifyStopper::restore()
{ thenotif_.cbs_.doEnable(oldst_);}
