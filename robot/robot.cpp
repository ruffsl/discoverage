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

#include "robot.h"
#include "robotmanager.h"
#include "tikzexport.h"
#include "scene.h"
#include "gridmap.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtCore/QSettings>

#include <math.h>

Robot::Robot(Scene* scene)
    : m_scene(scene)
    , m_position(scene->map().center())
{
}

Robot::~Robot()
{
}

void Robot::setPosition(const QPointF& position)
{
    m_position = position;
}

const QPointF& Robot::position() const
{
    return m_position;
}

bool Robot::hasOrientation() const
{
    return false;
}

qreal Robot::orientation() const
{
    return 0.0;
}

QPointF Robot::orientationVector() const
{
    return QPointF(0, 0);
}

bool Robot::isActive() const
{
    return RobotManager::self()->activeRobot() == this;
}

Scene* Robot::scene() const
{
    return m_scene;
}

GridMap* Robot::map() const
{
    // do not keep a local pointer to the grid map, as the pointer
    // changes when creating a new scene.
    return &m_scene->map();
}

QColor Robot::color()
{
    const int index = RobotManager::self()->indexOf(this);
    static QColor orange(255, 128, 0);
    static QColor lila(191, 127, 255);

    switch (index) {
        case 0: return Qt::blue;
        case 1: return Qt::green;
        case 2: return Qt::red;
        case 3: return Qt::yellow;
        case 4: return Qt::magenta;
        case 5: return Qt::cyan;
        case 6: return Qt::gray;
        case 7: return orange;
        case 8: return lila;
        default: return Qt::white;
    }
}

static QPainterPath circularPath(const QPointF& center, qreal radius)
{
    int segmentCount = static_cast<int>((radius + 1.0) * 10);

    QPainterPath path;
    path.moveTo(center + QPointF(radius, 0));

    for (int i = 1; i < segmentCount; ++i) {
        QPointF p(radius * cos(i * 2.0 * M_PI / segmentCount), radius * sin(i * 2.0 * M_PI / segmentCount));
        path.lineTo(center + p);
    }

    path.closeSubpath();
    return path;
}

QPainterPath Robot::visibleArea(double radius)
{
    QVector<Cell*> visibleCells = scene()->map().visibleCells(m_position, radius);
    QPainterPath visiblePath;
    foreach (Cell* cell, visibleCells) {
        visiblePath.addRect(cell->rect());
    }
    visiblePath = visiblePath.simplified();
    visiblePath = visiblePath.intersected(circularPath(m_position, radius));

    return visiblePath;
}

void Robot::draw(QPainter& p)
{
}

void Robot::exportToTikz(QTikzPicture& tp)
{
}

void Robot::tick()
{
}

void Robot::reset()
{
}

// kate: replace-tabs on; indent-width 4;
