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

#include "mindisthandler.h"
#include "scene.h"
#include "mainwindow.h"
#include "ui_discoveragewidget.h"
#include "robot.h"
#include "robotmanager.h"
#include "config.h"
#include "bullo.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <math.h>

//BEGIN MinDistHandler
MinDistHandler::MinDistHandler(Scene* scene, DisCoverageBulloHandler* centroidalSearch)
    : QObject()
    , ToolHandler(scene)
    , m_centroidalSearch(centroidalSearch)
{
    toolHandlerActive(false);
}

MinDistHandler::~MinDistHandler()
{
}

QString MinDistHandler::name() const
{
    return QString("MinDist");
}

void MinDistHandler::toolHandlerActive(bool activated)
{
}

void MinDistHandler::save(QSettings& config)
{
    ToolHandler::save(config);
}

void MinDistHandler::load(QSettings& config)
{
    ToolHandler::load(config);
}

void MinDistHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);
}

void MinDistHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);
}

void MinDistHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void MinDistHandler::reset()
{
}

void MinDistHandler::tick()
{
}

void MinDistHandler::postProcess()
{
    // compute geodesic Voronoi partition
    scene()->map().computeVoronoiPartition();

    // update the frontier cache
    scene()->map().updateRobotFrontierCache();

    // show density if wanted, needs distance transform
    if (Config::self()->showDensity()) {
        for (int i = 0; i < RobotManager::self()->count(); ++i)
            scene()->map().computeDistanceTransform(RobotManager::self()->robot(i));
        scene()->map().updateDensity();
    }

    // compute vector field in each cell if needed
    if (Config::self()->showVectorField()) {
        updateVectorField();
    }

    // redraw pixmap cache
    scene()->map().updateCache();
}

void MinDistHandler::updateVectorField()
{
    // FIXME: this is slow: compute for all explored free cells the shortest paths
    //        to all frontiers. Then pick the shortest one, and set the gradient
    //        according to direction of the first path segment
    const int count = RobotManager::self()->count();
    
    if (count >= 1) {
        for (int r = 0; r < count; ++r) {
            updateVectorField(RobotManager::self()->robot(r));
        }
    } else {
        updateVectorField(0);
    }
}

void MinDistHandler::updateVectorField(Robot* robot)
{
    // fallback to centroidal search if no frontiers exist
    if (robot && !scene()->map().hasFrontiers(robot)) {
        m_centroidalSearch->updateVectorField(robot);
        return;
    }

    const int dx = scene()->map().size().width();
    const int dy = scene()->map().size().height();
    const QList<Cell*> frontiers = scene()->map().frontiers(robot);
    for (int a = 0; a < dx; ++a) {
        for (int b = 0; b < dy; ++b) {
            Cell& c = scene()->map().cell(a, b);
            if (robot && robot != c.robot())
                continue;

            if (c.state() != (Cell::Explored | Cell::Free))
                continue;

            QList<Path> paths = scene()->map().frontierPaths(c.index(), frontiers);
            int shortestPathIndex = -1;
            qreal shortestPathLength = 1000000;
            for (int i = 0; i < paths.size(); ++i) {
                if (paths[i].m_length < shortestPathLength) {
                    paths[i].beautify(scene()->map());
                    shortestPathLength = paths[i].m_length;
                    shortestPathIndex = i;
                }
            }
            QPointF grad(0, 0);
            if (shortestPathIndex != -1 && paths[shortestPathIndex].m_path.size()) {
                grad = paths[shortestPathIndex].m_path[1] - paths[shortestPathIndex].m_path[0];
                grad /= sqrt(grad.x()*grad.x() + grad.y()*grad.y());
            }
            c.setGradient(grad);
        }
    }
}

QPointF MinDistHandler::gradient(Robot* robot, bool interpolate)
{
    // no frontiers: fallback to centroidal search-based DisCoverage
    if (!scene()->map().hasFrontiers(robot)) {
        return m_centroidalSearch->gradient(robot, interpolate);
    }

    if (interpolate) {
        return interpolatedGradient(robot->position(), robot);
    } else {
        const QList<Cell*> frontiers = scene()->map().frontiers(robot);
        return gradient(robot->position(), frontiers);
    }
}

QPointF MinDistHandler::gradient(const QPointF& robotPos, const QList<Cell*>& frontiers)
{
    GridMap& m = scene()->map();
    QPoint startIndex = m.worldToIndex(robotPos);
    QList<Path> allPaths = m.frontierPaths(startIndex, frontiers);
    double shortestPath = 1000000000.0;
    int shortestPathIndex = -1;
    for (int i = 0; i < allPaths.size(); ++i) {
        allPaths[i].beautify(m);
        if (allPaths[i].m_length < shortestPath) {
            shortestPath = allPaths[i].m_length;
            shortestPathIndex = i;
        }
    }

    if (shortestPathIndex < 0 || allPaths[shortestPathIndex].m_path.size() < 2) {
        return QPointF(0, 0);
    }

    const QPointF cellCenter = scene()->map().cell(allPaths[shortestPathIndex].m_path[1]).rect().center();

    // pos is continuous robot position
    // cellCenter is center of 2nd path cell
    const double dx = cellCenter.x() - robotPos.x();
    const double dy = cellCenter.y() - robotPos.y();

    QPointF grad(dx, dy);
    if (!grad.isNull()) {
        grad /= sqrt(grad.x()*grad.x() + grad.y()*grad.y());
    }

    return grad;
}

QPointF MinDistHandler::interpolatedGradient(const QPointF& robotPos, Robot* robot)
{
    GridMap& m = scene()->map();
    QPoint cellIndex(m.worldToIndex(robotPos));

    const double diffx = 1.0 - fabs(robotPos.x() - m.cell(cellIndex).center().x()) / scene()->map().resolution();
    const double diffy = 1.0 - fabs(robotPos.y() - m.cell(cellIndex).center().y()) / scene()->map().resolution();

    const int dx = (robotPos.x() < m.cell(cellIndex).center().x()) ? -1 : 1;
    const int dy = (robotPos.y() < m.cell(cellIndex).center().y()) ? -1 : 1;

    QPointF g00(m.cell(cellIndex).center());
    QPointF g01(g00);
    QPointF g10(g00);
    QPointF g11(g00);

    if (m.isValidField(cellIndex + QPoint(dx, 0))) g01 = (m.cell(cellIndex + QPoint(dx, 0)).center());
    if (m.isValidField(cellIndex + QPoint(0, dy))) g10 = (m.cell(cellIndex + QPoint(0, dy)).center());
    if (m.isValidField(cellIndex + QPoint(dx,dy))) g11 = (m.cell(cellIndex + QPoint(dx, dy)).center());

    QList<Cell*> frontiers = m.frontiers(robot);

    QPointF grad00(gradient(g00, frontiers));
    QPointF grad01(gradient(g01, frontiers));
    QPointF grad10(gradient(g10, frontiers));
    QPointF grad11(gradient(g11, frontiers));

    QPointF gradX0(diffx * grad00 + (1 - diffx) * grad01);
    QPointF gradX1(diffx * grad10 + (1 - diffx) * grad11);

    QPointF grad(diffy * gradX0 + (1 - diffy) * gradX1);

    return grad;
}
//END MinDistHandler

// kate: replace-tabs on; indent-width 4;
