
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include <qpopupmenu.h>

#include <klocale.h>
#include <kmessagebox.h>

#include "trackscanvas.h"

#include "rosedebug.h"


TrackPartItem::TrackPartItem(QCanvas* canvas)
    : QCanvasRectangle(canvas),
      m_part(0)
{
}

TrackPartItem::TrackPartItem(const QRect &r, QCanvas* canvas)
    : QCanvasRectangle(r, canvas),
      m_part(0)
{
}

TrackPartItem::TrackPartItem(int x, int y,
                             int width, int height,
                             QCanvas* canvas)
    : QCanvasRectangle(x, y, width, height, canvas),
      m_part(0)
{
}




TrackPart::TrackPart(TrackPartItem *r, unsigned int widthToLengthRatio)
    : m_trackNb(0),
      m_length(0),
      m_widthToLengthRatio(widthToLengthRatio),
      m_canvasPartItem(r)
{
    if (m_canvasPartItem) {
        m_length = m_canvasPartItem->width() / m_widthToLengthRatio;
        m_canvasPartItem->setPart(this);
    }

    kdDebug(KDEBUG_AREA) << "TrackPart::TrackPart : Length = "
                         << m_length << endl;

}

TrackPart::~TrackPart()
{
    delete m_canvasPartItem;
}

void
TrackPart::updateLength()
{
    m_length = m_canvasPartItem->width() / m_widthToLengthRatio;
    kdDebug(KDEBUG_AREA) << "TrackPart::updateLength : Length = "
                         << m_length << endl;
}



TracksCanvas::TracksCanvas(int gridH, int gridV,
                           QCanvas& c, QWidget* parent,
                           const char* name, WFlags f) :
    QCanvasView(&c,parent,name,f),
    m_toolType(Pencil),
    m_tool(new TrackPencil(this)),
    m_grid(gridH, gridV),
    m_brush(new QBrush(Qt::blue)),
    m_pen(new QPen(Qt::black)),
    m_editMenu(new QPopupMenu(this))
{
    m_editMenu->insertItem(I18N_NOOP("Edit"),
                           this, SLOT(onEdit()));
    m_editMenu->insertItem(I18N_NOOP("Edit Small"),
                           this, SLOT(onEditSmall()));
}

TracksCanvas::~TracksCanvas()
{
    delete m_brush;
    delete m_pen;
}

void
TracksCanvas::update()
{
    canvas()->update();
}

void
TracksCanvas::setTool(ToolType t)
{
    if (t == m_toolType) return;

    delete m_tool;
    m_tool = 0;
    m_toolType = t;

    switch(t) {
    case Pencil:
        m_tool = new TrackPencil(this);
        break;
    case Eraser:
        m_tool = new TrackEraser(this);
        break;
    case Mover:
        m_tool = new TrackMover(this);
        break;
    default:
        KMessageBox::error(0, QString("TracksCanvas::setTool() : unknown tool id %1").arg(t));
    }
}

TrackPartItem*
TracksCanvas::findPartClickedOn(QPoint pos)
{
    QCanvasItemList l=canvas()->collisions(pos);

    if (l.count()) {

        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            if (TrackPartItem *item = dynamic_cast<TrackPartItem*>(*it))
                return item;
        }

    }

    return 0;
}

void
TracksCanvas::contentsMousePressEvent(QMouseEvent* e)
{
    if (e->button() == LeftButton) { // delegate event handling to tool

        m_tool->handleMouseButtonPress(e);

    } else if (e->button() == RightButton) { // popup menu if over a part

        TrackPartItem *item = findPartClickedOn(e->pos());

        if (item) {
            m_currentItem = item;
//             kdDebug(KDEBUG_AREA) << "TracksCanvas::contentsMousePressEvent() : edit m_currentItem = "
//                                  << m_currentItem << endl;

            m_editMenu->exec(QCursor::pos());
        }
    }
}

void TracksCanvas::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton) m_tool->handleMouseButtonRelase(e);
}

void TracksCanvas::contentsMouseMoveEvent(QMouseEvent* e)
{
    m_tool->handleMouseMove(e);
}

void
TracksCanvas::wheelEvent(QWheelEvent *e)
{
    e->ignore();
}


void TracksCanvas::clear()
{
    QCanvasItemList list = canvas()->allItems();
    QCanvasItemList::Iterator it = list.begin();
    for (; it != list.end(); ++it) {
	if ( *it )
	    delete *it;
    }
}

TrackPartItem*
TracksCanvas::addPartItem(int x, int y, unsigned int nbBars)
{
    TrackPartItem* newPartItem = new TrackPartItem(x, y,
                                                   gridHStep() * nbBars,
                                                   grid().vstep(),
                                                   canvas());
    newPartItem->setPen(*m_pen);
    newPartItem->setBrush(*m_brush);
    newPartItem->setVisible(true);     

    return newPartItem;
}


void
TracksCanvas::onEdit()
{
    emit editTrackPart(m_currentItem->part());
}

void
TracksCanvas::onEditSmall()
{
    emit editTrackPartSmall(m_currentItem->part());
}

//////////////////////////////////////////////////////////////////////
//                 Track Tools
//////////////////////////////////////////////////////////////////////

TrackTool::TrackTool(TracksCanvas* canvas)
    : m_canvas(canvas)
{
}

TrackTool::~TrackTool()
{
}

//////////////////////////////
// TrackPencil
//////////////////////////////

TrackPencil::TrackPencil(TracksCanvas *c)
    : TrackTool(c),
      m_currentItem(0),
      m_newRect(false)
{
    connect(this, SIGNAL(addTrackPart(TrackPart*)),
            c,    SIGNAL(addTrackPart(TrackPart*)));

    kdDebug(KDEBUG_AREA) << "TrackPencil()\n";
}

void TrackPencil::handleMouseButtonPress(QMouseEvent *e)
{
    m_newRect = false;
    m_currentItem = 0;

    // Check if we're clicking on a rect
    //
    TrackPartItem *item = m_canvas->findPartClickedOn(e->pos());

    if (item) {
        // we are, so set currentItem to it
        m_currentItem = item;
        return;

    } else { // we are not, so create one

        int gx = m_canvas->grid().snapX(e->pos().x()),
            gy = m_canvas->grid().snapY(e->pos().y());

        m_currentItem = new TrackPartItem(gx, gy,
                                          m_canvas->grid().hstep(),
                                          m_canvas->grid().vstep(),
                                          m_canvas->canvas());
        
        m_currentItem->setPen(m_canvas->pen());
        m_currentItem->setBrush(m_canvas->brush());
        m_currentItem->setVisible(true);

        m_newRect = true;

        m_canvas->update();
    }

}

void TrackPencil::handleMouseButtonRelase(QMouseEvent*)
{
    if (!m_currentItem) return;

    if (m_currentItem->width() == 0) {
        kdDebug(KDEBUG_AREA) << "TracksCanvas::contentsMouseReleaseEvent() : rect deleted"
                             << endl;
        // delete m_currentItem; - TODO emit signal
    }

    if (m_newRect) {

        TrackPart *newPart = new TrackPart(m_currentItem,
                                           m_canvas->gridHStep());

        emit addTrackPart(newPart);

    } else {
        kdDebug(KDEBUG_AREA) << "TracksCanvas::contentsMouseReleaseEvent() : shorten m_currentItem = "
                             << m_currentItem << endl;
        // readjust size of corresponding track
        TrackPart *part = m_currentItem->part();
        part->updateLength();
    }


    m_currentItem = 0;
}

void TrackPencil::handleMouseMove(QMouseEvent *e)
{
    if ( m_currentItem ) {

        //         qDebug("Enlarging rect. to h = %d, v = %d",
        //                gpos.x() - m_currentItem->rect().x(),
        //                gpos.y() - m_currentItem->rect().y());

        kdDebug(KDEBUG_AREA) << "TracksPencil::handleMouseMove() : changing current Item size\n";

	m_currentItem->setSize(m_canvas->grid().snapX(e->pos().x()) - m_currentItem->rect().x(),
                               m_currentItem->rect().height());
	m_canvas->canvas()->update();
    }
}

//////////////////////////////
// TrackEraser
//////////////////////////////

TrackEraser::TrackEraser(TracksCanvas *c)
    : TrackTool(c)
{
    kdDebug(KDEBUG_AREA) << "TrackEraser()\n";
}

void TrackEraser::handleMouseButtonPress(QMouseEvent*)
{
}

void TrackEraser::handleMouseButtonRelase(QMouseEvent*)
{
}

void TrackEraser::handleMouseMove(QMouseEvent*)
{
}

//////////////////////////////
// TrackMover
//////////////////////////////

TrackMover::TrackMover(TracksCanvas *c)
    : TrackTool(c)
{
    kdDebug(KDEBUG_AREA) << "TrackMover()\n";
}

void TrackMover::handleMouseButtonPress(QMouseEvent*)
{
}

void TrackMover::handleMouseButtonRelase(QMouseEvent*)
{
}

void TrackMover::handleMouseMove(QMouseEvent*)
{
}

