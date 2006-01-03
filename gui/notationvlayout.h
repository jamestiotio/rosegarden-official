// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef NOTATIONVLAYOUT_H
#define NOTATIONVLAYOUT_H

#include "progressreporter.h"

#include "LayoutEngine.h"
#include "Staff.h"
#include "notationelement.h"
#include "FastVector.h"

class NotationStaff;
class NotationProperties;
class NotePixmapFactory;

/**
 * Vertical notation layout
 *
 * computes the Y coordinate of notation elements
 */

class NotationVLayout : public ProgressReporter,
                        public Rosegarden::VerticalLayoutEngine
{
public:
    NotationVLayout(Rosegarden::Composition *c, NotePixmapFactory *npf,
		    const NotationProperties &properties,
                    QObject* parent, const char* name = 0);

    virtual ~NotationVLayout();

    void setNotePixmapFactory(NotePixmapFactory *npf) {
	m_npf = npf;
    }

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(Rosegarden::Staff &,
			    Rosegarden::timeT = 0,
			    Rosegarden::timeT = 0);

    /**
     * Lay out a single staff.
     */
    virtual void scanStaff(Rosegarden::Staff &,
			   Rosegarden::timeT = 0,
			   Rosegarden::timeT = 0);

    /**
     * Do any layout dependent on more than one staff.  As it
     * happens, we have none, but we do have some layout that
     * depends on the final results from the horizontal layout
     * (for slurs), so we should do that here
     */
    virtual void finishLayout(Rosegarden::timeT = 0,
			      Rosegarden::timeT = 0);

private:

    void positionSlur(NotationStaff &staff, NotationElementList::iterator i);

    typedef FastVector<NotationElementList::iterator> SlurList;
    typedef std::map<Rosegarden::Staff *, SlurList> SlurListMap;

    //--------------- Data members ---------------------------------

    SlurListMap m_slurs;
    SlurList &getSlurList(Rosegarden::Staff &);

    Rosegarden::Composition *m_composition;
    NotePixmapFactory *m_npf;
    const Rosegarden::Quantizer *m_notationQuantizer;
    const NotationProperties &m_properties;
};

#endif
