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
#include "tikzexport.h"
#include "scene.h"

#include <QtCore/QDebug>

Robot::Robot(Scene* scene)
    : m_scene(scene)
    , m_position(0, 0)
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

void Robot::clearTrajectory()
{
    m_trajectory.clear();
}


Scene* Robot::scene() const
{
    return m_scene;
}

GridMap* Robot::map() const
{
    return &m_scene->map();
}


void Robot::draw(QPainter& p)
{
    // todo
}

void Robot::exportToTikz(QTextStream& ts)
{
    tikz::begin(ts, "yscale=-1");
    tikz::end(ts);
}

// kate: replace-tabs on; indent-width 4;
