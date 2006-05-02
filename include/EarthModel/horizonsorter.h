#ifndef horizonsorter_h
#define horizonsorter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		April 2006
 RCS:		$Id: horizonsorter.h,v 1.3 2006-05-02 14:21:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "executor.h"

#include "cubesampling.h"
#include "multiid.h"
#include "position.h"


namespace EM { class Horizon; }
template <class T> class Array3D;
class HorSamplingIterator;


class HorizonSorter : public Executor
{
public:

				HorizonSorter(const TypeSet<MultiID>&);
				~HorizonSorter();

    void			getSortedList(TypeSet<MultiID>&);
    const HorSampling&		getBoundingBox() const	{ return hrg_; }
    int				getNrCrossings(const MultiID&,
	    				       const MultiID&) const;

    const char*			message() const;
    int				totalNr() const;
    int				nrDone() const;
    const char*			nrDoneText() const;

protected:

    int				nextStep();
    void			calcBoundingBox();
    void			init();
    void			sort();

    int				totalnr_;
    int				nrdone_;

    HorSamplingIterator*	iterator_;
    BinID			binid_;
    HorSampling			hrg_;
    ObjectSet<EM::Horizon>	horizons_;
    Array3D<int>*		result_;
    TypeSet<MultiID>		unsortedids_;
    TypeSet<MultiID>		sortedids_;
};


#endif
