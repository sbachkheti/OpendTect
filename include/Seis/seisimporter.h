#ifndef seisimporter_h
#define seisimporter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2006
 RCS:		$Id: seisimporter.h,v 1.3 2006-12-08 13:57:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "executor.h"
#include "bufstring.h"
class IOObj;
class BinID;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcWriter;
class BinIDSorting;
class BinIDSortingAnalyser;


/*!\brief Helps import or export of seismic data. */

class SeisImporter : public Executor
{
public:

    /*!<\brief provides traces from the import storage

      fetch() must return false at end or when an error occurs.
      On error, the errmsg_ must be filled.

      */

    struct Reader
    {
	virtual			~Reader()			{}

	virtual const char*	name() const			= 0;
	virtual bool		fetch(SeisTrc&)			= 0;
	virtual int		totalNr() const			{ return -1; }

	BufferString		errmsg_;
    };


    			SeisImporter(Reader*,SeisTrcWriter&,Seis::GeomType);
				//!< Reader becomes mine. Has to be non-null.
    virtual		~SeisImporter();

    const char*		message() const;
    int			nrDone() const;
    const char*		nrDoneText() const;
    int			totalNr() const;
    int			nextStep();

    bool		removenulltrcs_;

protected:

    enum State		{ ReadBuf, WriteBuf, ReadWrite };

    Reader*		rdr_;
    SeisTrcWriter&	wrr_;
    SeisTrcBuf&		buf_;
    SeisTrc&		trc_;
    BinID&		prevbinid_;
    int			sort2ddir_;
    BinIDSorting*	sorting_;
    BinIDSortingAnalyser* sortanal_;
    Seis::GeomType	geomtype_;
    State		state_;
    int			nrread_;
    int			nrwritten_;
    bool		crlsorted_;
    Executor*		postproc_;

    bool		needInlCrlSwap() const;
    bool		sortingOk(const SeisTrc&);
    int			doWrite(SeisTrc&);
    int			readIntoBuf();
    Executor*		mkPostProc();

    mutable BufferString errmsg_;
    mutable BufferString	hndlmsg_;

};


#endif
