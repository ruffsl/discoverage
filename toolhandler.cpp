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

#include "toolhandler.h"
#include "scene.h"
#include "mainwindow.h"
#include "robotmanager.h"
#include "robot.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

#include <math.h>

//BEGIN ToolHandler
QPoint ToolHandler::s_currentCell = QPoint(0, 0);
QPoint ToolHandler::s_mousePosition = QPoint(0, 0);
qreal ToolHandler::s_operationRadius = 1.0;

ToolHandler::ToolHandler(Scene* scene)
    : m_scene(scene)
{
}

ToolHandler::~ToolHandler()
{
}

Scene* ToolHandler::scene() const
{
    return m_scene;
}

QPoint ToolHandler::cellForMousePosition(const QPoint& mousePosition)
{
    return Scene::self()->map().screenToIndex(mousePosition);
}

void ToolHandler::setCurrentCell(const QPoint& cellIndex)
{
    s_currentCell = cellIndex;
    Scene::self()->mainWindow()->setStatusPosition(cellIndex + QPoint(1, 1));
}

void ToolHandler::updateCurrentCell(const QPoint& mousePos)
{
    const QPoint cellIndex = cellForMousePosition(mousePos);
    if (Scene::self()->map().isValidField(cellIndex.x(), cellIndex.y())) {
        setCurrentCell(cellIndex);
        s_mousePosition = mousePos;
    }
}

QPoint ToolHandler::currentCell() const
{
    return s_currentCell;
}

QPoint ToolHandler::mousePosition() const
{
    return s_mousePosition;
}

void ToolHandler::setOperationRadius(double radius)
{
    s_operationRadius = radius;
}

double ToolHandler::operationRadius()
{
    return s_operationRadius;
}

void ToolHandler::draw(QPainter& p)
{
}

void ToolHandler::mousePressEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        if (Robot* robot = RobotManager::self()->activeRobot()) {
            QPointF pos = scene()->map().screenToWorld(event->posF());
            robot->setPosition(pos);
        }
    }
}

void ToolHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        if (Robot* robot = RobotManager::self()->activeRobot()) {
            QPointF pos = scene()->map().screenToWorld(event->posF());
            robot->setPosition(pos);
        }
    }
}

void ToolHandler::mouseReleaseEvent(QMouseEvent* event)
{
}

void ToolHandler::keyPressEvent(QKeyEvent* event)
{
    int index = event->key() - Qt::Key_1;

    if (index >= 0 && index < RobotManager::self()->count()) {
        RobotManager::self()->setActiveRobot(RobotManager::self()->robot(index));
    }
}

void ToolHandler::toolHandlerActive(bool activated)
{
}

void ToolHandler::reset()
{
}

void ToolHandler::tick()
{
}

void ToolHandler::postProcess()
{
}

void ToolHandler::save(QSettings& config)
{
    config.beginGroup("tool-handler");
    config.setValue("current-cell", s_currentCell);
    config.setValue("mouse-position", s_mousePosition);
    config.setValue("operation radius", s_operationRadius);
    config.endGroup();
}

void ToolHandler::load(QSettings& config)
{
    config.beginGroup("tool-handler");
    setCurrentCell(config.value("current-cell", QPoint(0, 0)).toPoint());
    s_mousePosition = config.value("mouse-position", QPoint(0, 0)).toPoint();
    s_operationRadius = config.value("operation radius", 2.0).toDouble();
    config.endGroup();
}

void ToolHandler::exportToTikz(QTikzPicture& tp)
{
}

void ToolHandler::drawOperationRadius(QPainter& p)
{
    p.setOpacity(0.2);
    QPainter::RenderHints rh = p.renderHints();
    p.setRenderHints(QPainter::Antialiasing, true);
    p.setBrush(QBrush(Qt::blue));
    p.drawEllipse(scene()->map().screenToWorld(mousePosition()), operationRadius(), operationRadius());
    p.setRenderHints(rh, true);
    p.setOpacity(1.0);
}

void ToolHandler::highlightCurrentCell(QPainter& p)
{
    if (s_currentCell.x() >= 0 && s_currentCell.x() < scene()->map().size().width() &&
        s_currentCell.y() >= 0 && s_currentCell.y() < scene()->map().size().height())
    {
        Cell& cell = scene()->map().cell(s_currentCell.x(), s_currentCell.y());

        // scale by 2
        p.save();
        p.translate(-cell.center());
        p.scale(2, 2);
        p.setOpacity(0.7);

        // draw cell in double size
        cell.draw(p, true, true);
        p.setPen(Qt::black);
        p.drawRect(cell.rect());

        p.restore();
    }
}

QPointF ToolHandler::gradient(Robot* /*robot*/, bool /*interpolate*/)
{
    return QPointF();
}
//END ToolHandler





//BEGIN RobotHandler
RobotHandler::RobotHandler(Scene* scene)
    : ToolHandler(scene)
{
}

RobotHandler::~RobotHandler()
{
}

void RobotHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);

    GridMap &m = scene()->map();

    QPainter::RenderHints rh = p.renderHints();
    p.setRenderHints(QPainter::Antialiasing, true);
    foreach (const Path& path, m_allPaths) {

        for (int i = 0; i < path.m_path.size() - 1; ++i) {
            const QPoint& a = path.m_path[i];
            const QPoint& b = path.m_path[i+1];
            p.drawLine(m.cell(a.x(), a.y()).rect().center(), m.cell(b.x(), b.y()).rect().center());
        }
    }

    p.setRenderHints(rh, true);
}
//END RobotHandler





//BEGIN ObstacleHandler
ObstacleHandler::ObstacleHandler(Scene* scene)
    : ToolHandler(scene)
{
}

ObstacleHandler::~ObstacleHandler()
{
}

void ObstacleHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    p.setOpacity(0.2);
    QRectF rect(scene()->map().screenToWorld(mousePosition()), 2 * QSizeF(1, 1) * scene()->map().resolution());
    rect.moveTo(rect.topLeft() - scene()->map().resolution() * QPointF(1, 1));
    p.fillRect(rect, Qt::black);
    p.setOpacity(1.0);
}

void ObstacleHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        updateObstacles();
    }
}

void ObstacleHandler::mousePressEvent(QMouseEvent* event)
{
}

void ObstacleHandler::updateObstacles()
{
    Cell::State destState = Cell::Obstacle;
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        destState = Cell::Free;
    }

    qreal x = scene()->map().screenToWorld(mousePosition().x());
    qreal y = scene()->map().screenToWorld(mousePosition().y());

    const qreal res = scene()->map().resolution();
    QRectF rect(QPointF(x - res, y - res), 2 * res * QSizeF(1, 1));

    GridMap& m = scene()->map();

    int xStart = qMax(0, (int)(rect.left() / res /*- 1*/));
    int xEnd = qMin(m.size().width() - 1, (int)(rect.right() / res /*+ 1*/));

    int yStart = qMax(0, (int)(rect.top() / res /*- 1*/));
    int yEnd = qMin(m.size().height() - 1, (int)(rect.bottom() / res /*+ 1*/));
    for (int a = xStart; a <= xEnd; ++a) {
        for (int b = yStart; b <= yEnd; ++b) {
            Cell& c = m.cell(a, b);
            if (!(c.state() & destState)) {
                if (rect.contains(c.center())) {
                    m.setState(c, destState);
                    m.updateCell(a, b);
                }
            }
        }
    }
}
//END ObstacleHandler




//BEGIN ExplorationHandler
ExplorationHandler::ExplorationHandler(Scene* scene)
    : ToolHandler(scene)
{
}

ExplorationHandler::~ExplorationHandler()
{
}

void ExplorationHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);
    highlightCurrentCell(p);
    drawOperationRadius(p);

}

void ExplorationHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        updateExploredState();
    }
}

void ExplorationHandler::mousePressEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        updateExploredState();
    }
}

void ExplorationHandler::updateExploredState()
{
    const bool markAsExplored = !(QApplication::keyboardModifiers() & Qt::ControlModifier);

    QPointF pos = scene()->map().screenToWorld(mousePosition());
    if (scene()->map().isValidField(pos.x(), pos.y())) {
        scene()->map().exploreInRadius(pos, operationRadius(), markAsExplored);
    }
}
//END ExplorationHandler

// kate: replace-tabs on; indent-width 4;
