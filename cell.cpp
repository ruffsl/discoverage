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

#include <QtGui/QPainter>
#include <QtGui/QBrush>
#include <QtCore/QDataStream>

//BEGIN helpers
static QColor densityToColor(float density)
{
    const float sat = density == 0 ? 0.0 : 1.0;
    const float val = 1.0;
    const float hue = density;

    return QColor::fromHsvF(hue, sat, val);
}
//END

Cell::Cell()
    : m_pathState(PathNone)
    , m_pathParent(-1)
    , m_rect()
    , m_state(static_cast<State>(Free | Unknown))
    , m_costF(0.0)
    , m_costG(0.0)
    , m_density(0.0)
{
}

Cell::Cell(const QRectF& rect)
    : m_pathState(PathNone)
    , m_pathParent(-1)
    , m_rect(rect)
    , m_state(static_cast<State>(Free | Unknown))
    , m_costF(0.0)
    , m_costG(0.0)
    , m_density(0.2)
{
}

void Cell::setRect(const QRectF& rect)
{
    m_rect = rect;
}

void Cell::setIndex(const QPoint& index)
{
    m_index = index;
}

const QPoint& Cell::index() const
{
    return m_index;
}

void Cell::draw(QPainter& p)
{
    QBrush sBrushFrontier = QBrush(QColor(255, 127, 0, 255));
    QBrush sBrushExplored = QBrush(QColor(255, 255, 255, 0));

    QBrush sBrushObstacle = QBrush(QColor(127, 127, 127));
    QBrush sBrushFree = QBrush(QColor(255, 255, 255));

    p.setPen(Qt::NoPen);
    if (m_state & Free) {
        p.fillRect(m_rect, sBrushFree);
    } else {
        p.fillRect(m_rect, sBrushObstacle);
    }

    if (m_state & Unknown) {
        p.setPen(Qt::gray);
        p.drawRect(m_rect);
    } else if (m_state & Frontier) {
        p.fillRect(m_rect, sBrushFrontier);
        p.setPen(Qt::gray);
        p.drawRect(m_rect);
    } else { // m_state & Explored
//         densityToColor
        p.fillRect(m_rect, densityToColor(m_density));
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

// kate: replace-tabs on; indent-width 4;
