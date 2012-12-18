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

    config.beginGroup("min-dist");
    config.setValue("robot-position", m_robotPosition);
    config.endGroup();
}

void MinDistHandler::load(QSettings& config)
{
    ToolHandler::load(config);

    config.beginGroup("dis-coverage");
    m_robotPosition = config.value("robot-position", QPointF(0.0, 0.0)).toPointF();
    config.endGroup();
}

void MinDistHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);

    GridMap &m = scene()->map();
    p.scale(m.scaleFactor(), m.scaleFactor());
    
    QPainter::RenderHints rh = p.renderHints();
    p.setRenderHints(QPainter::Antialiasing, true);

    QPen bluePen(QColor(0, 0, 255, 196), m.resolution() * 0.3);
    p.setPen(bluePen);
    p.setOpacity(0.2);
    p.setBrush(QBrush(Qt::blue));
    p.drawEllipse(m_robotPosition, operationRadius(), operationRadius());
    p.setOpacity(1.0);

    if (m_trajectory.size()) p.drawPolyline(&m_trajectory[0], m_trajectory.size());

    p.setPen(QPen(Qt::red, m.resolution() * 0.5));
    p.drawLine(m_robotPosition, m_robotPosition + QPointF(cos(m_orientation), sin(m_orientation)) * scene()->map().resolution());

    p.setRenderHints(rh, true);
}

void MinDistHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);

    if (event->buttons() & Qt::LeftButton) {
        m_robotPosition = scene()->map().mapScreenToMap(event->posF());
        updateMinDist(m_robotPosition);
    }
}

void MinDistHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void MinDistHandler::reset()
{
    m_trajectory.clear();
}

void MinDistHandler::tick()
{
    if (m_trajectory.size() == 0) {
        m_trajectory.append(m_robotPosition);
    }

    updateMinDist(m_robotPosition);
    m_robotPosition += QPointF(cos(m_orientation), sin(m_orientation)) * scene()->map().resolution();
    scene()->map().exploreInRadius(m_robotPosition, operationRadius(), true);

    m_trajectory.append(m_robotPosition);
}

void MinDistHandler::updateVectorField()
{
    qDebug() << "update vector field";
    const int dx = scene()->map().size().width();
    const int dy = scene()->map().size().height();

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
                QPointF gradient(0, 0);
                if (shortestPathIndex != -1 && paths[shortestPathIndex].m_path.size()) {
                    gradient = paths[shortestPathIndex].m_path[1] - paths[shortestPathIndex].m_path[0];
                    gradient /= sqrt(gradient.x()*gradient.x() + gradient.y()*gradient.y());
                }
                c.setGradient(gradient);
            }
        }
    }
}

void MinDistHandler::updateMinDist(const QPointF& robotPosition)
{
    GridMap& m = scene()->map();
    QPoint pt = m.mapMapToCell(robotPosition);
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

//     // prefer the current orientation
//     for (int i = 0; i < allPaths.size(); ++i) {
//         if (i == shortestPathIndex) continue;
// 
//         if (allPaths[i].m_length - 1.4142*m.resolution() < shortestPath) {
// 
//             const QPointF cellCenter = scene()->map().cell(allPaths[i].m_path[1]).rect().center();
// 
//             // pos is continuous robot position
//             // cellCenter is center of 2nd path cell
//             const double dx = cellCenter.x() - robotPosition.x();
//             const double dy = cellCenter.y() - robotPosition.y();
// 
//             double newOrientation = atan2(dy, dx);
//             double oDiff = (M_PI/4.0 - qMin(fabs(m_orientation - newOrientation), M_PI/4.0)) / (M_PI/4.0);
//             if (allPaths[i].m_length - oDiff * 1.4142 * m.resolution() < shortestPath) {
//                 shortestPath = allPaths[i].m_length;
//                 shortestPathIndex = i;
//             }
//         }
//     }

    if (shortestPathIndex < 0 || allPaths[shortestPathIndex].m_path.size() < 2) {
        return;
    }

    const QPointF cellCenter = scene()->map().cell(allPaths[shortestPathIndex].m_path[1]).rect().center();

    // pos is continuous robot position
    // cellCenter is center of 2nd path cell
    const double dx = cellCenter.x() - robotPosition.x();
    const double dy = cellCenter.y() - robotPosition.y();

    m_orientation = atan2(dy, dx);
}
//END MinDistHandler

// kate: replace-tabs on; indent-width 4;
