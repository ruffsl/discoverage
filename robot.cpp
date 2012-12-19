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

Robot::Robot(Scene* scene)
    : m_scene(scene)
    , m_position(scene->map().center())
    , m_sensingRange(3.0)
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

void Robot::setSensingRange(qreal sensingRange)
{
    m_sensingRange = sensingRange;
}

qreal Robot::sensingRange() const
{
    return m_sensingRange;
}

void Robot::clearTrajectory()
{
    m_trajectory.clear();
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

void Robot::draw(QPainter& p)
{
    drawRobot(p);
}

void Robot::drawRobot(QPainter& p)
{
    static QPen blackPen(Qt::black);
    p.setPen(blackPen);
    p.setBrush(color());
    p.drawEllipse(m_position, 0.05, 0.05);
}

void Robot::exportToTikz(QTextStream& ts)
{
}

void Robot::load(QSettings& config)
{
    setPosition(config.value("position", QPointF(0.0, 0.0)).toPointF());
    setSensingRange(config.value("sensing-range", 3.0).toDouble());
}

void Robot::save(QSettings& config)
{
    config.setValue("position", position());
    config.setValue("sensing-range", sensingRange());
}

// kate: replace-tabs on; indent-width 4;
