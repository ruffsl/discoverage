/* This file is part of the DisCoverage project.

   Copyright (C) Dominik Haumann <dhaumann at rtr.tu-darmstadt.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "scene.h"
#include "tikzexport.h"

#include <QtGui/QPainter>
#include <QtGui/QBrush>
#include <QtCore/QDataStream>
#include <math.h>

//BEGIN helpers
static QColor densityToColor(float density)
{
    const float sat = density == 0 ? 0.0 : 0.5;
    const float val = 1.0;
    float tmp = sqrt(-logf(density)) / 10;
    const float hue = tmp;

    return QColor::fromHsvF(hue, sat, val);
}
//END

Cell::Cell()
    : m_pathState(PathNone)
    , m_pathParent(-1)
    , m_rect()
    , m_state(static_cast<State>(Free | Unknown))
    , m_gradient(0, 0)
    , m_costF(0.0)
    , m_costG(0.0)
    , m_density(1.0)
    , m_frontierDist(0.0)
{
}

Cell::Cell(const QRectF& rect)
    : m_pathState(PathNone)
    , m_pathParent(-1)
    , m_rect(rect)
    , m_state(static_cast<State>(Free | Unknown))
    , m_gradient(0, 0)
    , m_costF(0.0)
    , m_costG(0.0)
    , m_density(1.0)
    , m_frontierDist(0.0)
{
}

void Cell::setRect(const QRectF& rect)
{
    m_rect = rect;
}

const QPointF Cell::center() const
{
    return m_rect.center();
}

void Cell::setIndex(const QPoint& index)
{
    m_index = index;
}

const QPoint& Cell::index() const
{
    return m_index;
}

void Cell::draw(QPainter& p, bool showDensity, bool showGradient)
{
    static QBrush sBrushFrontier = QBrush(QColor(255, 127, 0, 255));
    static QBrush sBrushExplored = QBrush(QColor(255, 255, 255, 0));

    static QBrush sBrushExploredObstacle = QBrush(QColor(127, 127, 127));
    static QBrush sBrushUnexploredObstacle = QBrush(QColor(Qt::gray));
    static QBrush sBrushFree = QBrush(QColor(255, 255, 255));

    p.setPen(Qt::NoPen);
    if (m_state & Free) {
        p.fillRect(m_rect, sBrushFree);
    } else {
        p.fillRect(m_rect, (m_state & Explored) ? sBrushExploredObstacle : sBrushUnexploredObstacle);
    }

    if (m_state & Unknown) {
        p.setPen(Qt::gray);
        p.drawRect(m_rect);
    } else if (m_state & Frontier) {
        p.fillRect(m_rect, sBrushFrontier);
        p.setPen(Qt::gray);
        p.drawRect(m_rect);
    } else if (!(m_state & Obstacle)) { // m_state & Explored
        if (showDensity) {
            p.fillRect(m_rect, densityToColor(m_density));
        } else {
            p.fillRect(m_rect, Qt::white);
        }

        if (showGradient && !m_gradient.isNull()) {
            // Draw the arrows
            double angle = ::acos(m_gradient.x() / sqrt(m_gradient.x()*m_gradient.x() + m_gradient.y()*m_gradient.y()));
            if (m_gradient.y() >= 0)
                angle = 2 * M_PI - angle;

            qreal arrowSize = m_rect.width() / 2.0;
            
            QPointF src = center() - m_gradient * m_rect.width() / 4.0;
            QPointF dst = center() + m_gradient * m_rect.width() / 4.0;

            QPointF destArrowP1 = dst + QPointF(sin(angle - M_PI / 3) * arrowSize/2,
                                                     cos(angle - M_PI / 3) * arrowSize/2);
            QPointF destArrowP2 = dst + QPointF(sin(angle - M_PI + M_PI / 3) * arrowSize/2,
                                                     cos(angle - M_PI + M_PI / 3) * arrowSize/2);

            p.setRenderHints(QPainter::Antialiasing, true);
            p.setPen(Qt::black);
            p.drawLine(src, dst);
            p.drawPolyline(QPolygonF() << destArrowP1 << dst << destArrowP2);
            p.setRenderHints(QPainter::Antialiasing, false);
        }
    }
}

const QRectF& Cell::rect() const
{
    return m_rect;
}

void Cell::setState(State newState)
{
    if (newState & 0x3) {
        m_state = (m_state & 0x1C) | newState; // 1C = 28 = 111|00
        if ((newState & Obstacle) && (m_state & Frontier)) {
            m_state = (m_state & 0x3) | Unknown;
        }
    }

    if (newState & 0x1C) {
        if ((newState & Frontier) && (m_state & Obstacle)) {
            m_state = (m_state & 0x3) | Explored;
        } else {
            m_state = (m_state & 0x3) | newState;
        }
    }
}

void Cell::setDensity(float density)
{
    m_density = density;
}

void Cell::setGradient(const QPointF& gradient)
{
    m_gradient = gradient;
}

void Cell::setFrontierDist(float dist)
{
    m_frontierDist = dist;
}

QDataStream& Cell::load(QDataStream& ds)
{
    qint32 s;
    QRectF r;

    ds >> s;
    ds >> r;

    setState((Cell::State)s);
    setRect(r);

    return ds;
}

QDataStream& Cell::save(QDataStream& ds)
{
    ds << qint32(state());
    ds << rect();

    return ds;
}

void Cell::exportToTikz(QTextStream& ts, bool fillDensity, bool exportGradient)
{
    static QColor sBrushFrontier(255, 127, 0, 255);
    static QColor sBrushExplored(255, 255, 255, 0);

    static QColor sBrushExploredObstacle(127, 127, 127);
    static QColor sBrushUnexploredObstacle(Qt::gray);
    static QColor sBrushFree(255, 255, 255);

    if (m_state & Frontier) { // frontiers
        tikz::fill(ts, m_rect, sBrushFrontier, Qt::black);
    } else if (m_state == (Explored | Free)) { // free explored
        QColor color = densityToColor(m_density);
        tikz::fill(ts, m_rect, color, Qt::white);
    } else {
        QColor color = densityToColor(m_density);
        tikz::fill(ts, m_rect, color, color);
    }

    if (exportGradient && !m_gradient.isNull()) {
        QPointF src = center() - m_gradient * m_rect.width() / 4.0;
        QPointF dst = center() + m_gradient * m_rect.width() / 4.0;
        tikz::arrow(ts, src, dst);
    }
}

// kate: replace-tabs on; indent-width 4;
