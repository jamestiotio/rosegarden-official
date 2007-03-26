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


#include "MarkerRuler.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/RulerScale.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/HZoomable.h"
#include <kxmlguifactory.h>
#include <qbrush.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpoint.h>
#include <qpopupmenu.h>
#include <qrect.h>
#include <qsize.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

MarkerRuler::MarkerRuler(RosegardenGUIDoc *doc,
                         RulerScale *rulerScale,
                         int barHeight,
                         double xorigin,
                         QWidget* parent,
                         const char* name,
                         WFlags f)
        : QWidget(parent, name, f),
        m_barHeight(barHeight),
        m_xorigin(xorigin),
        m_currentXOffset(0),
        m_width(-1),
        m_clickX(0),
        m_menu(0),
        m_doc(doc),
        m_rulerScale(rulerScale),
        m_parentMainWindow(dynamic_cast<KMainWindow*>(doc->parent()))
{
    //    m_barFont = new QFont("helvetica", 12);
    //    m_barFont->setPixelSize(12);
    m_barFont = new QFont();
    m_barFont->setPointSize(10);

    new KAction(i18n("Insert Marker"), 0, this,
             SLOT(slotInsertMarkerHere()), actionCollection(),
             "insert_marker_here");
    
    new KAction(i18n("Insert Marker at Playback Position"), 0, this,
             SLOT(slotInsertMarkerAtPointer()), actionCollection(),
             "insert_marker_at_pointer");

    new KAction(i18n("Delete Marker"), 0, this,
             SLOT(slotDeleteMarker()), actionCollection(),
             "delete_marker");
    
}

MarkerRuler::~MarkerRuler()
{
    delete m_barFont;
}

void
MarkerRuler::createMenu()
{             
    setXMLFile("markerruler.rc");
    
    KXMLGUIFactory* factory = m_parentMainWindow->factory();
    factory->addClient(this);

    QWidget* tmp = factory->container("marker_ruler_menu", this);

//    if (!tmp) {
//        RG_DEBUG << "MarkerRuler::createMenu() menu not found\n"
//                 << domDocument().toString(4) << endl;
//    }
    
    m_menu = dynamic_cast<QPopupMenu*>(tmp);
    
    if (!m_menu) {
        RG_DEBUG << "MarkerRuler::createMenu() failed\n";
    }
}


void 
MarkerRuler::scrollHoriz(int x)
{
    m_currentXOffset = static_cast<int>( -x / getHScaleFactor());
    repaint();
}

QSize 
MarkerRuler::sizeHint() const
{
    int lastBar =
        m_rulerScale->getLastVisibleBar();
    double width =
        m_rulerScale->getBarPosition(lastBar) +
        m_rulerScale->getBarWidth(lastBar) + m_xorigin;

    return QSize(std::max(int(width), m_width), m_barHeight);
}

QSize 
MarkerRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;

    return QSize(static_cast<int>(firstBarWidth), m_barHeight);
}

void
MarkerRuler::slotInsertMarkerHere()
{
    emit addMarker(getClickPosition());    
}

void
MarkerRuler::slotInsertMarkerAtPointer()
{
    emit addMarker(m_doc->getComposition().getPosition());
}

void
MarkerRuler::slotDeleteMarker()
{
    RG_DEBUG << "MarkerRuler::slotDeleteMarker()\n";
    
    Rosegarden::Marker* marker = getMarkerAtClickPosition();
    
    if (marker)
        emit deleteMarker(marker->getTime(),
                          marker->getName(),
                          marker->getDescription());                          
}

timeT
MarkerRuler::getClickPosition()
{
    timeT t = m_rulerScale->getTimeForX
              (m_clickX - m_xorigin - m_currentXOffset);

    return t;
}

Rosegarden::Marker*
MarkerRuler::getMarkerAtClickPosition()
{
    QRect clipRect = visibleRect();

    int firstBar = m_rulerScale->getBarForX(clipRect.x() -
                                            m_currentXOffset -
                                            m_xorigin);
    int lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
        firstBar = m_rulerScale->getFirstVisibleBar();
    }

    Composition &comp = m_doc->getComposition();
    Composition::markercontainer markers = comp.getMarkers();

    timeT start = comp.getBarStart(firstBar);
    timeT end = comp.getBarEnd(lastBar);

    // need these to calculate the visible extents of a marker tag
    QPainter painter(this);
    painter.setFont(*m_barFont);
    QFontMetrics metrics = painter.fontMetrics();

    for (Composition::markerconstiterator i = markers.begin();
            i != markers.end(); ++i) {

        if ((*i)->getTime() >= start && (*i)->getTime() < end) {

            QString name(strtoqstr((*i)->getName()));

            int x = m_rulerScale->getXForTime((*i)->getTime())
                    + m_xorigin + m_currentXOffset;

            int width = metrics.width(name) + 5;

            int nextX = -1;
            Composition::markerconstiterator j = i;
            ++j;
            if (j != markers.end()) {
                nextX = m_rulerScale->getXForTime((*j)->getTime())
                        + m_xorigin + m_currentXOffset;
            }

            if (m_clickX >= x && m_clickX <= x + width) {

                if (nextX < x || m_clickX <= nextX) {

                    return *i;
                }
            }
        }
    }

    return 0L;
}
    
void
MarkerRuler::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setFont(*m_barFont);

    if (getHScaleFactor() != 1.0)
        painter.scale(getHScaleFactor(), 1.0);

    QRect clipRect = visibleRect();

    int firstBar = m_rulerScale->getBarForX(clipRect.x() -
                                            m_currentXOffset -
                                            m_xorigin);
    int lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
        firstBar = m_rulerScale->getFirstVisibleBar();
    }

    painter.drawLine(m_currentXOffset, 0, static_cast<int>(visibleRect().width() / getHScaleFactor()), 0);

    float minimumWidth = 25.0;
    float testSize = ((float)(m_rulerScale->getBarPosition(firstBar + 1) -
                              m_rulerScale->getBarPosition(firstBar)))
                     / minimumWidth;

    int every = 0;
    int count = 0;

    if (testSize < 1.0) {
        every = (int(1.0 / testSize));

        if (every % 2 == 0)
            every++;
    }

    for (int i = firstBar; i <= lastBar; ++i) {

        double x = m_rulerScale->getBarPosition(i) + m_xorigin + m_currentXOffset;

        if ((x * getHScaleFactor()) > clipRect.x() + clipRect.width())
            break;

        // always the first bar number
        if (every && i != firstBar) {
            if (count < every) {
                count++;
                continue;
            }

            // reset count if we passed
            count = 0;
        }

        // adjust count for first bar line
        if (every == firstBar)
            count++;

        if (i != lastBar) {
            painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), m_barHeight);

            // disable worldXForm for text
            QPoint textDrawPoint = painter.xForm(QPoint(static_cast<int>(x + 4), 12));

            bool enableXForm = painter.hasWorldXForm();
            painter.setWorldXForm(false);

            if (i >= 0)
                painter.drawText(textDrawPoint, QString("%1").arg(i + 1));

            painter.setWorldXForm(enableXForm);
        } else {
            const QPen normalPen = painter.pen();
            ;
            QPen endPen(black, 2);
            painter.setPen(endPen);
            painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), m_barHeight);
            painter.setPen(normalPen);
        }
    }

    if (m_doc) {
        Composition &comp = m_doc->getComposition();
        Composition::markercontainer markers = comp.getMarkers();
        Composition::markerconstiterator it;

        timeT start = comp.getBarStart(firstBar);
        timeT end = comp.getBarEnd(lastBar);

        QFontMetrics metrics = painter.fontMetrics();

        for (it = markers.begin(); it != markers.end(); ++it) {
            if ((*it)->getTime() >= start && (*it)->getTime() < end) {
                QString name(strtoqstr((*it)->getName()));

                double x = m_rulerScale->getXForTime((*it)->getTime())
                           + m_xorigin + m_currentXOffset;

                painter.fillRect(static_cast<int>(x), 1,
                                 static_cast<int>(metrics.width(name) + 5),
                                 m_barHeight - 2,
                                 QBrush(GUIPalette::getColour(GUIPalette::MarkerBackground)));

                painter.drawLine(int(x), 1, int(x), m_barHeight - 2);
                painter.drawLine(int(x) + 1, 1, int(x) + 1, m_barHeight - 2);

                QPoint textDrawPoint = painter.xForm
                                       (QPoint(static_cast<int>(x + 3), m_barHeight - 4));

                // disable worldXForm for text
                bool enableXForm = painter.hasWorldXForm();
                painter.setWorldXForm(false);

                painter.drawText(textDrawPoint, name);

                painter.setWorldXForm(enableXForm);
            }
        }
    }
}

void
MarkerRuler::mousePressEvent(QMouseEvent *e)
{
    RG_DEBUG << "MarkerRuler::mousePressEvent: x = " << e->x() << endl;

    if (!m_doc || !e)
        return;

    m_clickX = e->x();
    Rosegarden::Marker* clickedMarker = getMarkerAtClickPosition();
    
    // if right-click, show popup menu
    //
    if (e->button() == RightButton) {
        if (!m_menu)
            createMenu();
        if (m_menu) {
            actionCollection()->action("delete_marker")->setEnabled(clickedMarker != 0);
            m_menu->exec(QCursor::pos());
        }
        return;       
    }
            
    bool shiftPressed = ((e->state() & Qt::ShiftButton) != 0);

    Composition &comp = m_doc->getComposition();
    Composition::markercontainer markers = comp.getMarkers();

    if (shiftPressed) { // set loop

        timeT t = m_rulerScale->getTimeForX
                  (e->x() - m_xorigin - m_currentXOffset);

        timeT prev = 0;

        for (Composition::markerconstiterator i = markers.begin();
                i != markers.end(); ++i) {

            timeT cur = (*i)->getTime();

            if (cur >= t) {
                emit setLoop(prev, cur);
                return ;
            }

            prev = cur;
        }

        if (prev > 0)
            emit setLoop(prev, comp.getEndMarker());

    } else { // set pointer to clicked marker

        if (clickedMarker)
            emit setPointerPosition(clickedMarker->getTime());
    }
}

void
MarkerRuler::mouseDoubleClickEvent(QMouseEvent *)
{
    RG_DEBUG << "MarkerRuler::mouseDoubleClickEvent" << endl;

    emit editMarkers();
}

}
#include "MarkerRuler.moc"
