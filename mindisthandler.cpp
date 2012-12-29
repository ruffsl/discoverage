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

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <math.h>

//BEGIN MinDistHandler
MinDistHandler::MinDistHandler(Scene* scene)
    : QObject()
    , ToolHandler(scene)
{
    toolHandlerActive(false);
}

MinDistHandler::~MinDistHandler()
{
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
    scene()->map().computeVoronoiPartition();

    if (Config::self()->showDensity()) {
        for (int i = 0; i < RobotManager::self()->count(); ++i)
            scene()->map().computeDistanceTransform(RobotManager::self()->robot(i));
        scene()->map().updateDensity();
    }

    if (Config::self()->showVectorField()) {
        updateVectorField();
    }

    scene()->map().updateCache();
}

void MinDistHandler::updateVectorField()
{
    const int dx = scene()->map().size().width();
    const int dy = scene()->map().size().height();

    // FIXME: this is slow: compute for all explored free cells the shortest paths
    //        to all frontiers. Then pick the shortest one, and set the gradient
    //        according to direction of the first path segment
    for (int a = 0; a < dx; ++a) {
        for (int b = 0; b < dy; ++b) {
            Cell& c = scene()->map().cell(a, b);
            if (c.state() == (Cell::Explored | Cell::Free)) {
                QList<Path> paths = scene()->map().frontierPaths(c.index());
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
}

QPointF MinDistHandler::gradient(Robot* robot, bool /*interpolate*/)
{
    GridMap& m = scene()->map();
    QPoint pt = m.worldToIndex(robot->position());
    QList<Path> allPaths = m.frontierPaths(pt);
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
    const double dx = cellCenter.x() - robot->position().x();
    const double dy = cellCenter.y() - robot->position().y();

    QPointF grad(dx, dy);
    if (!grad.isNull()) {
        grad /= sqrt(grad.x()*grad.x() + grad.y()*grad.y());
    }

    return grad;
}
//END MinDistHandler

// kate: replace-tabs on; indent-width 4;
