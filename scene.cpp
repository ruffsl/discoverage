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
#include "mainwindow.h"
#include "ui_newscenedialog.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>

Scene* Scene::s_self = 0;

Scene::Scene(MainWindow* mainWindow, QWidget* parent)
    : QFrame(parent)
    , m_map(this, 30, 22.5, 0.2)
    , m_mainWindow(mainWindow)
    , m_robotHandler(this)
    , m_obstacleHandler(this)
    , m_explorationHandler(this)
    , m_discoverageHandler(this)
    , m_minDistHandler(this)
    , m_bulloHandler(this)
{
    s_self = this;

    setMouseTracking(true);
    QPixmap cursorPixmap(1, 1);
    cursorPixmap.fill();
    setCursor(cursorPixmap); // set 1x1 pixel cursor

    setFrameStyle(Panel | Sunken);

    m_toolHandler = &m_robotHandler;

    m_map.updateCache();
}

Scene::~Scene()
{
    s_self = 0;
}

void Scene::newScene()
{
    QDialog dlg;
    Ui::NewSceneDialog ui;
    ui.setupUi(&dlg);
    if (dlg.exec() == QDialog::Accepted) {
        const double res = ui.sbResolution->value();
        const double width = ui.sbWidth->value();
        const double height = ui.sbHeight->value();

        m_map = GridMap(this, width, height, res);
        mainWindow()->setStatusResolution(res);
        m_map.updateCache();
        setFixedSize(sizeHint());
        update();
    }
}

void Scene::load(QSettings& config)
{
    m_map.load(config);

    mainWindow()->setStatusResolution(m_map.resolution());
    m_map.updateCache();
    setFixedSize(sizeHint());

    m_minDistHandler.load(config);
    m_discoverageHandler.load(config);
    m_explorationHandler.load(config);
    m_obstacleHandler.load(config);
    m_bulloHandler.load(config);

    update();
}

void Scene::save(QSettings& config)
{
    m_map.save(config);

    m_obstacleHandler.save(config);
    m_explorationHandler.save(config);
    m_discoverageHandler.save(config);
    m_minDistHandler.save(config);
    m_bulloHandler.save(config);
}

void Scene::wheelEvent(QWheelEvent* event)
{
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        if (event->delta() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

QSize Scene::sizeHint() const
{
    return m_map.displaySize();
}

MainWindow* Scene::mainWindow() const
{
    return m_mainWindow;
}

void Scene::zoomIn()
{
    m_map.incScaleFactor();
    setFixedSize(sizeHint());
}

void Scene::zoomOut()
{
    m_map.decScaleFactor();
    setFixedSize(sizeHint());
}

void Scene::selectTool(int toolIndex)
{
    switch (toolIndex) {
        case 1:
        case 4:
            qWarning() << "Scene::selectTool() called with separator index" << toolIndex;
            break;
        case 0:
            m_toolHandler->toolHandlerActive(false);
            m_toolHandler = &m_robotHandler;
            m_toolHandler->toolHandlerActive(true);
            mainWindow()->statusBar()->showMessage("Click to place the robot.");
            break;
        case 2:
            m_toolHandler->toolHandlerActive(false);
            m_toolHandler = &m_obstacleHandler;
            m_toolHandler->toolHandlerActive(true);
            mainWindow()->statusBar()->showMessage("Use the CTRL modifier to remove obstacles.");
            break;
        case 3:
            m_toolHandler->toolHandlerActive(false);
            m_toolHandler = &m_explorationHandler;
            m_toolHandler->toolHandlerActive(true);
            mainWindow()->statusBar()->showMessage("Use the CTRL modifier to mark area as unexplored.");
            break;
        case 5:
            // TODO FIXME implement DisCoverage handler
            m_toolHandler->toolHandlerActive(false);
            m_toolHandler = &m_discoverageHandler;
            m_toolHandler->toolHandlerActive(true);
            mainWindow()->statusBar()->clearMessage();
            break;
        case 6:
            // TODO FIXME implement DisCoverage handler
            m_toolHandler->toolHandlerActive(false);
            m_toolHandler = &m_minDistHandler;
            m_toolHandler->toolHandlerActive(true);
            mainWindow()->statusBar()->clearMessage();
            break;
        case 7:
            // TODO FIXME implement DisCoverage handler
            m_toolHandler->toolHandlerActive(false);
            m_toolHandler = &m_bulloHandler;
            m_toolHandler->toolHandlerActive(true);
            mainWindow()->statusBar()->clearMessage();
            break;

        default:
            qWarning() << "Scene::selectTool() called with invalid index";
    }

    update();
}

void Scene::setOperationRadius(double radius)
{
    m_toolHandler->setOperationRadius(radius);
    update();
}

void Scene::draw(QPaintDevice* paintDevice)
{
    QPainter p(paintDevice);
    m_map.draw(p);

    // painter tool overlay
    m_toolHandler->draw(p);

    p.end();
}

void Scene::paintEvent(QPaintEvent* event)
{
    draw(this);

    QFrame::paintEvent(event);
}

void Scene::mouseMoveEvent(QMouseEvent* event)
{
    QPoint pos = event->pos();
    if (pos.x() < 0) pos.setX(0);
    if (pos.y() < 0) pos.setY(0);
    if (pos.x() >= width()) pos.setX(width() - 1);
    if (pos.y() >= height()) pos.setY(height() - 1);
    QMouseEvent constrainedEvent(event->type(), pos, event->button(), event->buttons(), event->modifiers());

    m_toolHandler->mouseMoveEvent(&constrainedEvent);
    update();
}

void Scene::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        grabMouse();
    }

    m_toolHandler->mousePressEvent(event);
    update();
}

void Scene::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        releaseMouse();
    }
}

GridMap& Scene::map()
{
    return m_map;
}

void Scene::reset()
{
    m_toolHandler->reset();
    update();
}

void Scene::tick()
{
    m_toolHandler->tick();

//     if (
}

// kate: replace-tabs on; indent-width 4;
