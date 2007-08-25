
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    Numerous additions and bug fixes by
        Michael McIntyre    <dmmcintyr@users.sourceforge.net>

    Some restructuring by Chris Cannam.

    Brain surgery to support LilyPond 2.x export by Heikki Junes.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_LILYPONDEXPORTER_H_
#define _RG_LILYPONDEXPORTER_H_

#include "base/Event.h"
#include "base/PropertyName.h"
#include "base/Segment.h"
#include "gui/general/ProgressReporter.h"
#include <fstream>
#include <set>
#include <string>
#include <utility>


class QObject;


namespace Rosegarden
{

class TimeSignature;
class Studio;
class RosegardenGUIApp;
class RosegardenGUIView;
class RosegardenGUIDoc;
class NotationView;
class Key;
class Composition;

const std::string headerDedication = "dedication";
const std::string headerTitle = "title";
const std::string headerSubtitle = "subtitle";
const std::string headerSubsubtitle = "subsubtitle";
const std::string headerPoet = "poet";
const std::string headerComposer = "composer";
const std::string headerMeter = "meter";
const std::string headerOpus = "opus";
const std::string headerArranger = "arranger";
const std::string headerInstrument = "instrument";
const std::string headerPiece = "piece";
const std::string headerCopyright = "copyright";
const std::string headerTagline = "tagline";

/**
 * Lilypond scorefile export
 */

class LilypondExporter : public ProgressReporter
{
public:
    typedef std::multiset<Event*, Event::EventCmp> eventstartlist;
    typedef std::multiset<Event*, Event::EventEndCmp> eventendlist;

public:
    LilypondExporter(RosegardenGUIApp *parent, RosegardenGUIDoc *, std::string fileName);
    LilypondExporter(NotationView *parent, RosegardenGUIDoc *, std::string fileName);
    ~LilypondExporter();

    bool write();

protected:
    RosegardenGUIView *m_view;
    NotationView *m_notationView;
    RosegardenGUIDoc *m_doc;
    Composition *m_composition;
    Studio *m_studio;
    std::string m_fileName;

    void readConfigVariables(void);
    void writeBar(Segment *, int barNo, int barStart, int barEnd, int col,
                  Rosegarden::Key &key, std::string &lilyText,
                  std::string &prevStyle, eventendlist &eventsInProgress,
                  std::ofstream &str, int &MultiMeasureRestCount,
                  bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                  bool &nextBarIsDouble, bool &nextBarIsEnd, bool &nextBarIsDot);
    
    timeT calculateDuration(Segment *s,
                                        const Segment::iterator &i,
                                        timeT barEnd,
                                        timeT &soundingDuration,
                                        const std::pair<int, int> &tupletRatio,
                                        bool &overlong);

    void handleStartingEvents(eventstartlist &eventsToStart, std::ofstream &str);
    void handleEndingEvents(eventendlist &eventsInProgress,
                            const Segment::iterator &j, std::ofstream &str);

    // convert note pitch into Lilypond format note string
    std::string convertPitchToLilyNote(int pitch,
                                       Accidental accidental,
                                       const Rosegarden::Key &key);

    // compose an appropriate Lilypond representation for various Marks
    std::string composeLilyMark(std::string eventMark, bool stemUp);

    // find/protect illegal characters in user-supplied strings
    std::string protectIllegalChars(std::string inStr);

    // return a string full of column tabs
    std::string indent(const int &column);
                  
    void writeSkip(const TimeSignature &timeSig,
                   timeT offset,
                   timeT duration,
                   bool useRests,
                   std::ofstream &);

    /*
     * Handle Lilypond directive.  Returns true if the event was a directive,
     * so subsequent code does not bother to process the event twice
     */
    bool handleDirective(const Event *textEvent,
                         std::string &lilyText,
                         bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                         bool &nextBarIsDouble, bool &nextBarIsEnd, bool &nextBarIsDot);

    void handleText(const Event *, std::string &lilyText);
    void writePitch(const Event *note, const Rosegarden::Key &key, std::ofstream &);
    void writeStyle(const Event *note, std::string &prevStyle, int col, std::ofstream &, bool isInChord);
    void writeDuration(timeT duration, std::ofstream &);
    void writeSlashes(const Event *note, std::ofstream &);
       
private:
    static const int MAX_DOTS = 4;
    static const PropertyName SKIP_PROPERTY;
    
    unsigned int m_paperSize;
    bool m_paperLandscape;
    unsigned int m_fontSize;
    bool m_exportLyrics;
    bool m_exportMidi;

    unsigned int m_exportTempoMarks;
    static const int EXPORT_NONE_TEMPO_MARKS = 0;
    static const int EXPORT_FIRST_TEMPO_MARK = 1;
    static const int EXPORT_ALL_TEMPO_MARKS = 2;
    
    unsigned int m_exportSelection;
    static const int EXPORT_ALL_TRACKS = 0;
    static const int EXPORT_NONMUTED_TRACKS = 1;
    static const int EXPORT_SELECTED_TRACK = 2;
    static const int EXPORT_SELECTED_SEGMENTS = 3;

    bool m_exportPointAndClick;
    bool m_exportBeams;
    bool m_exportStaffGroup;
    bool m_exportStaffMerge;
    bool m_raggedBottom;

    int m_languageLevel;
    static const int LILYPOND_VERSION_2_6  = 0;
    static const int LILYPOND_VERSION_2_8  = 1;
    static const int LILYPOND_VERSION_2_10 = 2;
    static const int LILYPOND_VERSION_2_12 = 3;

};



}

#endif

