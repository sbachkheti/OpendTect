#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2013
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
#include "od_iosfwd.h"
#include "uistring.h"

mFDQtclass(QProcess);
class qstreambuf;

namespace OS
{


enum LaunchType	{ Wait4Finish, RunInBG };


/*!\brief Specifies how to execute a command */


mExpClass(Basic) CommandExecPars
{
public:
			CommandExecPars( bool isbatchprog=false )
			    : needmonitor_(false)
			    , createstreams_(false)
			    , prioritylevel_(isbatchprog ? -1.0f : 0.0f)
			    , launchtype_(isbatchprog ? RunInBG : Wait4Finish)
			    , isconsoleuiprog_(false)	{}

    mDefSetupClssMemb(CommandExecPars,bool,createstreams);
    mDefSetupClssMemb(CommandExecPars,bool,needmonitor);
    mDefSetupClssMemb(CommandExecPars,BufferString,monitorfnm);
			    //!< when empty, will be generated (if needed)

    mDefSetupClssMemb(CommandExecPars,LaunchType,launchtype);

    mDefSetupClssMemb(CommandExecPars,float,prioritylevel);
			    //!< -1=lowest, 0=normal, 1=highest (administrator)

    mDefSetupClssMemb(CommandExecPars,bool,isconsoleuiprog);
			    //!< program uses text-based stdin console input
			    //!< if true, will ignore monitor settings

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;
    void		removeFromPar(IOPar&) const;

    static const char*	sKeyPriority()	{ return "priority"; }

    static const StepInterval<int>	cMachineUserPriorityRange(bool iswin);
			/*!< Restricted to OS-specific user available range
			     Unix: 0-19 (0=normal)
			     Windows: 6-8 (8=normal)
			  */

    static int		getMachinePriority(float priolevel,bool iswin);
};


/*!\brief Encapsulates an actual command to execute + the machine to run it on

The default remote execution command is ssh.

 */

mExpClass(Basic) MachineCommand
{
public:

			MachineCommand( const char* prognm=0 )
			    : prognm_(prognm)		{}
			MachineCommand( const char* prognm,
					const BufferStringSet& arguments )
			    : prognm_(prognm)
			    , args_(arguments)		{}

    const char*		program() const			{ return prognm_; }
    void		setProgram( const char* pn )	{ prognm_.set( pn ); }
    const BufferStringSet& args() const			{ return args_; }

    MachineCommand&	addArg(const char*);
    MachineCommand&	addArgs(const BufferStringSet&);
    MachineCommand&	addFlag( const char* flg )
			{ return addKeyedArg(flg,0); }
    MachineCommand&	addKeyedArg(const char* ky,const char* valstr);

			// convenience:
    template <class T>
    MachineCommand&	addArg( const T& t )	{ return addArg(::toString(t));}
    template <class T>
    MachineCommand&	addKeyedArg( const char* ky, const T& t )
			{ return addKeyedArg(ky,::toString(t));}

    bool		hostIsWindows() const		{ return hostiswin_; }
    void		setHostIsWindows( bool yn )	{ hostiswin_ = yn; }
    const char*		hostName() const		{ return hname_; }
    void		setHostName( const char* hnm )	{ hname_ = hnm; }
    const char*		remExec() const			{ return remexec_; }
    void		setRemExec( const char* sh )	{ remexec_ = sh; }

    bool		isBad() const		{ return prognm_.isEmpty(); }
    bool		hasHostName() const	{ return !hname_.isEmpty(); }

    static const char*	defaultRemExec()	{ return defremexec_; }
    static void		setDefaultRemExec( const char* s ) { defremexec_ = s; }

    static const char*	odRemExecCmd()		{ return "od_remexec"; }
    static const char*	sKeyRemoteHost()	{ return "machine"; }
    static const char*	sKeyRemoteCmd()		{ return "cmd"; }
    static const char*	sKeyMasterHost()	{ return "masterhost"; }
    static const char*	sKeyMasterPort()	{ return "masterport"; }
    static const char*	sKeyBG()		{ return "bg"; }
    static const char*	sKeyFG()		{ return "fg"; }
    static const char*	sKeyJobID()		{ return "jobid"; }

protected:

    BufferString	prognm_;
    BufferStringSet	args_;
    bool		hostiswin_		= __iswin__;
    BufferString	hname_;
    BufferString	remexec_		= defremexec_;

    static BufferString	defremexec_;

public:

    bool		setFromSingleStringRep(const char*,
						bool ignorehostname=false);
    BufferString	getSingleStringRep(bool noremote=false) const;
    BufferString	getExecCommand() const;

    static const char*	extractHostName(const char*,BufferString&);
			//!< returns remaining part

};


/*!\brief Launches machine commands */

mExpClass(Basic) CommandLauncher
{ mODTextTranslationClass(CommandLauncher);
public:
			CommandLauncher(const MachineCommand&);
			~CommandLauncher();

    void		set(const MachineCommand&);

    bool		execute(const CommandExecPars& pars=CommandExecPars());

    int			processID() const;
    const char*		monitorFileName() const	{ return monitorfnm_; }
    uiString		errorMsg() const	{ return errmsg_; }

    od_istream*		getStdOutput() { return stdoutput_; }
    od_istream*		getStdError() { return stderror_; }
    od_ostream*		getStdInput() { return stdinput_; }

protected:

    void		reset();
    bool		doExecute(const char* comm,bool wait4finish,
				  bool inconsole = false,
				  bool createstreams=false );
    int			catchError();
    bool		startDetached(const char*,bool inconsole=false);

    MachineCommand	machcmd_;
    BufferString	monitorfnm_;
    bool		redirectoutput_;

    BufferString	progvwrcmd_;
    uiString		errmsg_;
    const BufferString	odprogressviewer_;

    QProcess*		process_;
    od_int64		pid_;

    od_istream*		stdoutput_;
    od_istream*		stderror_;
    od_ostream*		stdinput_;

    qstreambuf*		stdoutputbuf_;
    qstreambuf*		stderrorbuf_;
    qstreambuf*		stdinputbuf_;
public:

    static void		addShellIfNeeded(BufferString& cmd);
			/*!<Analyses the cmd and looks for pipes or redirects.
			    If these are found, the cmd is converted to a
			    shell command. */
    static void		addQuotesIfNeeded(BufferString& word);
			/*!<Checks for spaces in word, and surrounds word
			    with quotes them if not already done. */
    static void		manageQProcess(QProcess*);
			/*!<Add a QProcess and it will be deleted one day. */
};


/*! convenience function; for specific options use the CommandLauncher */
mGlobal(Basic) bool ExecCommand(const char* cmd,LaunchType lt=Wait4Finish,
				BufferString* stdoutput=0,
				BufferString* stderror=0);


} // namespace OS

/*! convenience function executing a program from the OD bindir */
mGlobal(Basic) bool ExecODProgram(const char* prognm,const char* args=0,
				  OS::LaunchType lt=OS::RunInBG);
