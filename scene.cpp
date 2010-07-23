#include "scene.h"
#include "mainwindow.h"
#include "ui_newscenedialog.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>

Scene::Scene(MainWindow* mainWindow, QWidget* parent)
    : QFrame(parent)
    , m_map(30.0, 22.5, 0.2)
    , m_mainWindow(mainWindow)
    , m_robotHandler(this)
    , m_obstacleHandler(this)
    , m_explorationHandler(this)
{
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

        m_map = GridMap(width, height, res);
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
    update();
}

void Scene::save(QSettings& config)
{
    m_map.save(config);
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
    update();
}

void Scene::zoomOut()
{
    m_map.decScaleFactor();
    setFixedSize(sizeHint());
    update();
}

void Scene::selectTool(int toolIndex)
{
    switch (toolIndex) {
        case 0:
            m_toolHandler = &m_robotHandler;
            break;
        case 1:
            m_toolHandler = &m_obstacleHandler;
            m_obstacleHandler.setDesiredState(Cell::Obstacle);
            break;
        case 2:
            m_toolHandler = &m_obstacleHandler;
            m_obstacleHandler.setDesiredState(Cell::Free);
            break;
        case 3:
            m_toolHandler = &m_explorationHandler;
            m_explorationHandler.setDesiredState(Cell::Explored);
            break;
        case 4:
            m_toolHandler = &m_explorationHandler;
            m_explorationHandler.setDesiredState(Cell::Unknown);
            break;
        default:
            qDebug() << "Scene::selectTool() called with invalid index";
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
    m_toolHandler->mouseMoveEvent(event);
    update();
}

void Scene::mousePressEvent(QMouseEvent* event)
{
    m_toolHandler->mousePressEvent(event);
    update();
}

GridMap& Scene::map()
{
    return m_map;
}

// kate: replace-tabs on; indent-width 4;
