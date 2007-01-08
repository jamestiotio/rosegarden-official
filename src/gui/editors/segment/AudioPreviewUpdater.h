
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

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_AUDIOPREVIEWUPDATER_H_
#define _RG_AUDIOPREVIEWUPDATER_H_

#include <qobject.h>
#include <qrect.h>
#include <vector>


class QEvent;


namespace Rosegarden
{

class Segment;
class CompositionModelImpl;
class Composition;
class AudioPreviewThread;


class AudioPreviewUpdater : public QObject
{
    Q_OBJECT

public:
    AudioPreviewUpdater(AudioPreviewThread &thread,
                        const Composition &composition,
                        const Segment *segment,
                        const QRect &displayExtent,
                        CompositionModelImpl *parent);
    ~AudioPreviewUpdater();

    void update();
    void cancel();

    QRect getDisplayExtent() const { return m_rect; }
    void setDisplayExtent(const QRect &rect) { m_rect = rect; }

    const Segment *getSegment() const { return m_segment; }

    const std::vector<float> &getComputedValues(unsigned int &channels) const
    { channels = m_channels; return m_values; }

signals:
    void audioPreviewComplete(AudioPreviewUpdater*);

protected:
    virtual bool event(QEvent*);

    AudioPreviewThread& m_thread;

    const Composition& m_composition;
    const Segment*     m_segment;
    QRect                          m_rect;
    bool                           m_showMinima;
    unsigned int                   m_channels;
    std::vector<float>             m_values;

    intptr_t m_previewToken;
};


}

#endif
