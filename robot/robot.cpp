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
    , m_sensingRange(3.0)
    , m_fillSensingRange(false)
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

qreal Robot::orientation() const
{
    const int count = m_trajectory.size();
    if (count < 2) {
        return 0.0;
    } else {
        const QPointF lastMove = m_trajectory[count - 1] - m_trajectory[count - 2];
        qreal delta = atan2(lastMove.y(), lastMove.x());
        return delta;
    }
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

void Robot::setFillSensingRange(bool fill)
{
    m_fillSensingRange = fill;
}

bool Robot::fillSensingRange() const
{
    return m_fillSensingRange;
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

void Robot::drawSensedArea(QPainter& p)
{
    QVector<Cell*> visibleCells = scene()->map().visibleCells(m_position, m_sensingRange);
    QPainterPath visiblePath;
    foreach (Cell* cell, visibleCells) {
        visiblePath.addRect(cell->rect());
    }
    visiblePath = visiblePath.simplified();
    visiblePath = visiblePath.intersected(circularPath(m_position, m_sensingRange));

    QColor col(color());
    QPen pen(color(), map()->resolution() * 0.3);
    p.setPen(pen);
    if (fillSensingRange()) {
        col.setAlpha(50);
        p.setBrush(col);
    } else {
        p.setBrush(Qt::NoBrush);
    }
    p.drawPath(visiblePath);
}

void Robot::draw(QPainter& p)
{
    p.setRenderHints(QPainter::Antialiasing, true);

    drawRobot(p);

    p.setPen(QPen(QColor(0, 0, 0, 196), map()->resolution() * 0.3, Qt::DotLine));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(m_position, 0.5, 0.5);

    drawSensedArea(p);

    // draw trajectory
    if (m_trajectory.size()) p.drawPolyline(&m_trajectory[0], m_trajectory.size());

    p.setRenderHints(QPainter::Antialiasing, false);
}

void Robot::drawRobot(QPainter& p)
{
    static QPen blackPen(Qt::black);
    blackPen.setWidthF(map()->resolution() * 0.1);
    p.setOpacity(1.0);
    p.setPen(blackPen);
    p.setBrush(color());
    p.drawEllipse(m_position, 0.05, 0.05);
}

void Robot::exportToTikz(QTextStream& ts)
{
    // construct path of visibility region
    QVector<Cell*> visibleCells = scene()->map().visibleCells(m_position, m_sensingRange);
    QPainterPath visiblePath;
    foreach (Cell* cell, visibleCells) {
        visiblePath.addRect(cell->rect());
    }
    visiblePath = visiblePath.simplified();
    visiblePath = visiblePath.intersected(circularPath(m_position, m_sensingRange));
    tikz::path(ts, visiblePath, "thick, blue, fill=black, fill opacity=0.2");

    tikz::circle(ts, m_position, 0.5, "dashed, thick");
    tikz::circle(ts, m_position, 0.05, "draw=black, fill=white");
//     tikz::circle(ts, m_robotPosition, operationRadius());
}

void Robot::load(QSettings& config)
{
    setPosition(config.value("position", QPointF(0.0, 0.0)).toPointF());
    setSensingRange(config.value("sensing-range", 3.0).toDouble());
    setSensingRange(config.value("fill-sensing-range", false).toBool());
}

void Robot::save(QSettings& config)
{
    config.setValue("position", position());
    config.setValue("sensing-range", sensingRange());
    config.setValue("fill-sensing-range", fillSensingRange());
}

void Robot::tick()
{
    if (m_trajectory.size() == 0) {
        m_trajectory.append(m_position);
    }

    m_position += scene()->toolHandler()->gradient(this, true) * scene()->map().resolution();

    bool changed = scene()->map().exploreInRadius(m_position, m_sensingRange, Cell::Explored);

    m_trajectory.append(m_position);
}

void Robot::reset()
{
    clearTrajectory();
}

// kate: replace-tabs on; indent-width 4;
