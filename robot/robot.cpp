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

#define _USE_MATH_DEFINES 
#include <math.h>

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
    , m_fillSensingRange(false)
{
}

Robot::~Robot()
{
}

void Robot::setPosition(const QPointF& position, bool trackTrajectory)
{
    if (trackTrajectory) {
        if (m_trajectory.size() == 0)
            m_trajectory.append(m_position);
        m_trajectory.append(position);
    }

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

void Robot::setSensingRange(double sensingRange)
{
    m_sensingRange = sensingRange;
}

double Robot::sensingRange() const
{
    return m_sensingRange;
}

void Robot::setFillSensingRange(bool fill)
{
    m_fillSensingRange = fill;
}

bool Robot::fillSensingRange() const
{
    return m_fillSensingRange;
}

static QPainterPath circularPath(const QPointF& center, qreal radius)
{
    int segmentCount = static_cast<int>((radius + 1.0) * 20);

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

void Robot::drawTrajectory(QPainter& p)
{
    if (m_trajectory.size()) {
        p.save();
        p.setRenderHints(QPainter::Antialiasing, true);
        p.setPen(QPen(color(), map()->resolution() * 0.3, Qt::SolidLine));
        p.drawPolyline(&m_trajectory[0], m_trajectory.size());
        p.restore();
    }
}

void Robot::drawSensedArea(QPainter& p)
{
    QColor col(color());
    QPen pen(col, map()->resolution() * 0.3);
    p.setPen(pen);
    if (fillSensingRange()) {
        col.setAlpha(50);
        p.setBrush(col);
    } else {
        p.setBrush(Qt::NoBrush);
    }

    QPainterPath visiblePath = visibleArea(m_sensingRange);
    p.drawPath(visiblePath);
}

QVector<QPointF> Robot::previewTrajectory()
{
    QVector<QPointF> previewPath;
    previewPath.append(m_position);

    const QPoint index = scene()->map().worldToIndex(m_position);
    if (!scene()->map().isValidField(index)) return previewPath;

    const QPointF backupPos = m_position;
    double length = 0;

    do {
        m_position = previewPath.last();
        const QPointF& nextPos = m_position + scene()->toolHandler()->gradient(this, true) * scene()->map().resolution();
        previewPath.append(nextPos);
        const QPointF& cmpPos = previewPath[qMax(0, previewPath.size() - 5)];
        length = (nextPos - cmpPos).manhattanLength();
    } while (length >= scene()->map().resolution() &&
        scene()->map().isValidField(scene()->map().worldToIndex(previewPath.last())) &&
        !(scene()->map().cell(scene()->map().worldToIndex(previewPath.last())).state() & Cell::Frontier)
    );

    m_position = backupPos;

    return previewPath;
}

void Robot::drawPreviewTrajectory(QPainter& p)
{
    QVector<QPointF> previewPath = previewTrajectory();
    if (previewPath.size()) {
        p.save();
        p.setRenderHints(QPainter::Antialiasing, true);
        p.setPen(QPen(color(), 0.05, Qt::DashLine));
        p.drawPolyline(&previewPath[0], previewPath.size());
        p.restore();
    }
}

void Robot::clearTrajectory()
{
    m_trajectory.clear();
}

const QVector<QPointF>& Robot::trajectory() const
{
    return m_trajectory;
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

void Robot::load(QSettings& config)
{
    setPosition(config.value("position", QPointF(0.0, 0.0)).toPointF());
    setSensingRange(config.value("sensing-range", 3.0).toDouble());
    setFillSensingRange(config.value("fill-sensing-range", false).toBool());

    // load trajectory
    QList<QVariant> trajectory = config.value("trajectory", QList<QVariant>()).toList();
    m_trajectory.clear();
    foreach (const QVariant& p, trajectory) {
        m_trajectory.append(p.toPointF());
    }
}

void Robot::save(QSettings& config)
{
    config.setValue("position", position());
    config.setValue("sensing-range", sensingRange());
    config.setValue("fill-sensing-range", fillSensingRange());

    // save trajectory
    QList<QVariant> trajectory;
    foreach (const QPointF& p, m_trajectory) {
        trajectory.append(p);
    }
    config.setValue("trajectory", trajectory);
}


// kate: replace-tabs on; indent-width 4;
