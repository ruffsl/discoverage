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

#include "discoveragehandler.h"
#include "scene.h"
#include "mainwindow.h"
#include "ui_discoveragewidget.h"
#include "robot.h"
#include "robotmanager.h"
#include "config.h"
#include <qglobal.h> // qFuzzyCompare

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QDockWidget>

//BEGIN DisCoverageHandler
DisCoverageHandler::DisCoverageHandler(Scene* scene)
    : QObject()
    , ToolHandler(scene)
    , m_dock(0)
    , m_ui(0)
    , m_plotter(0)
{
    toolHandlerActive(false);
}

DisCoverageHandler::~DisCoverageHandler()
{
    delete m_ui;
}

void DisCoverageHandler::toolHandlerActive(bool activated)
{
    dockWidget()->setVisible(activated);
    if (activated) {
        connect(RobotManager::self(), SIGNAL(activeRobotChanged(Robot*)), m_plotter, SLOT(updatePlot(Robot*)));
    } else {
        disconnect(RobotManager::self(), 0, m_plotter, 0);
    }
}

QDockWidget* DisCoverageHandler::dockWidget()
{
    if (!m_dock) {
        m_dock = new QDockWidget("DisCoverage", scene()->mainWindow());
        m_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        m_ui = new Ui::DisCoverageWidget();
        QWidget* w = new QWidget();
        m_ui->setupUi(w);
        m_plotter = new OrientationPlotter(this, w);
        m_ui->gbOptimization->layout()->addWidget(m_plotter);
        m_dock->setWidget(w);
        scene()->mainWindow()->addDockWidget(Qt::LeftDockWidgetArea, m_dock);

        connect(m_ui->sbTheta, SIGNAL(valueChanged(double)), this, SLOT(updateParameters()));
        connect(m_ui->sbSigma, SIGNAL(valueChanged(double)), this, SLOT(updateParameters()));
        connect(m_ui->chkLocalOptimum, SIGNAL(toggled(bool)), this, SLOT(updateParameters()));

        updateParameters();
    }
    return m_dock;
}

void DisCoverageHandler::setOpeningAngleStdDeviation(double theta)
{
    m_ui->sbTheta->blockSignals(true);
    m_ui->sbTheta->setValue(theta);
    m_ui->sbTheta->blockSignals(false);
}

double DisCoverageHandler::openingAngleStdDeviation() const
{
    return m_ui->sbTheta->value();
}

void DisCoverageHandler::setAutoAdaptDistanceStdDeviation(bool autoAdapt)
{
    m_ui->chkAutoDist->blockSignals(true);
    m_ui->chkAutoDist->setChecked(autoAdapt);
    m_ui->chkAutoDist->blockSignals(false);
}

bool DisCoverageHandler::autoAdaptDistanceStdDeviation() const
{
    return m_ui->chkAutoDist->isChecked();
}

void DisCoverageHandler::setDistanceStdDeviation(double sigma)
{
    m_ui->sbSigma->blockSignals(true);
    m_ui->sbSigma->setValue(sigma);
    m_ui->sbSigma->blockSignals(false);
}

double DisCoverageHandler::distanceStdDeviation() const
{
    return m_ui->sbSigma->value();
}

void DisCoverageHandler::setFollowLocalOptimum(bool localOptimum)
{
    m_ui->chkLocalOptimum->blockSignals(true);
    m_ui->chkLocalOptimum->setChecked(localOptimum);
    m_ui->chkLocalOptimum->blockSignals(false);
}

bool DisCoverageHandler::followLocalOptimum() const
{
    return m_ui->chkLocalOptimum->isChecked();
}

void DisCoverageHandler::save(QSettings& config)
{
    ToolHandler::save(config);

    config.beginGroup("dis-coverage");
    config.setValue("theta", openingAngleStdDeviation());
    config.setValue("sigma", distanceStdDeviation());
    config.setValue("local-optimum", followLocalOptimum());
    config.setValue("auto-dist", autoAdaptDistanceStdDeviation());
    config.endGroup();
}

void DisCoverageHandler::load(QSettings& config)
{
    ToolHandler::load(config);

    config.beginGroup("dis-coverage");
    setOpeningAngleStdDeviation(config.value("theta", 0.5).toDouble());
    setDistanceStdDeviation(config.value("sigma", 2.0).toDouble());
    setFollowLocalOptimum(config.value("local-optimum", true).toBool());
    setAutoAdaptDistanceStdDeviation(config.value("auto-dist", false).toBool());
    config.endGroup();
}

void DisCoverageHandler::updateParameters()
{
    postProcess();
    scene()->update();
}

void DisCoverageHandler::updateVectorField()
{
    // FIXME: this is slow: compute for all explored free cells the shortest paths
    //        to all frontiers. Then pick the shortest one, and set the gradient
    //        according to direction of the first path segment
    const int count = RobotManager::self()->count();

    if (count > 0) {
        for (int r = 0; r < count; ++r) {
            updateVectorField(RobotManager::self()->robot(r));
        }
    } else {
        updateVectorField(0);
    }
}

void DisCoverageHandler::updateVectorField(Robot* robot)
{
    const int dx = scene()->map().size().width();
    const int dy = scene()->map().size().height();
    const QList<Cell*> frontiers = scene()->map().frontiers(robot);

    // iterate over all free explored cells
    for (int a = 0; a < dx; ++a) {
        for (int b = 0; b < dy; ++b) {
            Cell& c = scene()->map().cell(a, b);
            if (robot && robot != c.robot())
                continue;

            if (c.state() != (Cell::Explored | Cell::Free))
                continue;

            c.setGradient(gradient(robot, robot->position()));
        }
    }
}

void DisCoverageHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);
}

void DisCoverageHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);

    // update plot, if a robot is being moved
    if (event->buttons() & Qt::LeftButton) {
        m_plotter->updatePlot(RobotManager::self()->activeRobot());
    }
}

void DisCoverageHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void DisCoverageHandler::reset()
{
}

void DisCoverageHandler::tick()
{
}

void DisCoverageHandler::postProcess()
{
    // compute geodesic Voronoi partition
    scene()->map().computeVoronoiPartition();

    // update the frontier cache
    scene()->map().updateRobotFrontierCache();

    // compute vector field in each cell if needed
    if (Config::self()->showVectorField()) {
        updateVectorField();
    }

    // redraw pixmap cache
    scene()->map().updateCache();

    m_plotter->updatePlot(RobotManager::self()->activeRobot());
}

QPointF DisCoverageHandler::gradient(Robot* robot, bool interpolate)
{
    if (interpolate) {
        return interpolatedGradient(robot);
    } else {
        const bool allowAutoAdjustDistanceComponent = true;
        const double orientation = robot->orientation();
        return gradient(robot, robot->position(), robot->hasOrientation() ? &orientation : 0, allowAutoAdjustDistanceComponent);
    }
}

QPointF DisCoverageHandler::interpolatedGradient(Robot* robot)
{
    const QPointF robotPos = robot->position();
    GridMap& m = scene()->map();
    QPoint cellIndex(m.worldToIndex(robotPos));

    // generate 4 samplings points in adjacent cell centers
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

    // get robot orientation, if available
    double orientation = robot->orientation();
    double* pOrientation = robot->hasOrientation() ? (&orientation) : 0;

    // compute gradients in all 4 sampling points
    QPointF grad00(gradient(robot, g00, pOrientation, true)); // first time, allow to auto adjust distance component
    QPointF grad01(gradient(robot, g01, pOrientation));
    QPointF grad10(gradient(robot, g10, pOrientation));
    QPointF grad11(gradient(robot, g11, pOrientation));

    // interpolate: compute linear combination
    QPointF gradX0(diffx * grad00 + (1 - diffx) * grad01);
    QPointF gradX1(diffx * grad10 + (1 - diffx) * grad11);

    QPointF grad(diffy * gradX0 + (1 - diffy) * gradX1);

    // return gradient
    return grad;
}

QPointF DisCoverageHandler::gradient(Robot* robot, const QPointF& robotPos, const double* startOrientation, bool adjustDistanceComponent)
{
    const QList<Cell*> frontiers = Scene::self()->map().frontiers(robot);
    
    if (frontiers.size() == 0)
        return QPointF(0, 0);

    GridMap& m = Scene::self()->map();
    QPoint pt = m.worldToIndex(robotPos);
    QList<Path> allPaths = m.frontierPaths(pt, frontiers);

    double shortestPath = 1000000000.0;
    for (int i = 0; i < allPaths.size(); ++i) {
        allPaths[i].beautify(m);
        if (allPaths[i].m_length < shortestPath) {
            shortestPath = allPaths[i].m_length;
        }
    }

    if (autoAdaptDistanceStdDeviation() && adjustDistanceComponent) {
        setDistanceStdDeviation(shortestPath);
    }

    QVector<QPointF> deltaPoints;

    double delta = -M_PI;
    double sMax = 0.0;
    double deltaMax = 0.0;
    while (delta < M_PI) {
        double s = 0;
        int i = 0;
        foreach (Cell* q, frontiers) {
            s += disCoverage(robotPos, delta, q->rect().center(), allPaths[i]);
            ++i;
        }

        deltaPoints.append(QPointF(delta, s));

        if (s > sMax) {
            sMax = s;
            deltaMax = delta;
        }
        delta += 0.1;
    }

    // follow local optimum
    if (followLocalOptimum() && startOrientation) {
        int i;
        for (i = 0; i < deltaPoints.size(); ++i) {
            if (qFuzzyCompare(deltaPoints[i].x(), *startOrientation))
                break;
        }

        // first time m_delta == 0.0, so no index found
        if (deltaPoints.size() == i) {
//             qDebug() << "found no correct orientation!";
            i = 0;
        }

        const int n = deltaPoints.size();
        int c = 0; // avoid infinite loop
        while (deltaPoints[i].y() <= deltaPoints[(i + 1) % n].y() && c < n) {
            i = (i + 1) % n;
            ++c;
        }

        c = 0;
        while (deltaPoints[i].y() <= deltaPoints[(i - 1 + n) % n].y() && c < n) {
            i = (i - 1 + n) % n;
            ++c;
        }

        deltaMax = deltaPoints[i].x();
    }

    return QPointF(cos(deltaMax), sin(deltaMax));
}

double DisCoverageHandler::disCoverage(const QPointF& pos, double delta, const QPointF& q, const Path& path)
{
    if (path.m_path.size() < 2) {
        return 0.0f;
    }

    const double theta = openingAngleStdDeviation();
    const double sigma = distanceStdDeviation();

    const QPointF cellCenter = scene()->map().cell(path.m_path[1]).rect().center();

    // pos is continuous robot position
    // cellCenter is center of 2nd path cell
    const double dx = cellCenter.x() - pos.x();
    const double dy = cellCenter.y() - pos.y();

    float alpha = - delta + atan2(dy, dx);

    if (alpha > M_PI) alpha -= 2 * M_PI;
    else if (alpha < -M_PI) alpha += 2 * M_PI;

    const double len = path.m_length;

    return exp(- alpha*alpha/(2.0*theta*theta)
               - len*len/(2.0*sigma*sigma));
}
//END DisCoverageHandler





OrientationPlotter::OrientationPlotter(DisCoverageHandler* handler, QWidget* parent)
    : QFrame(parent)
    , m_handler(handler)
{
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setFixedHeight(100);
}

OrientationPlotter::~OrientationPlotter()
{
}

void OrientationPlotter::updatePlot(Robot* robot)
{
    if (!robot) return;

    const QList<Cell*> frontiers = Scene::self()->map().frontiers(robot);
    GridMap& m = Scene::self()->map();
    QPoint pt = m.worldToIndex(robot->position());
    QList<Path> allPaths = m.frontierPaths(pt, frontiers);

    double shortestPath = 1000000000.0;
    for (int i = 0; i < allPaths.size(); ++i) {
        allPaths[i].beautify(m);
        if (allPaths[i].m_length < shortestPath) {
            shortestPath = allPaths[i].m_length;
        }
    }

    if (m_handler->autoAdaptDistanceStdDeviation()) {
        m_handler->setDistanceStdDeviation(shortestPath);
    }

    QVector<QPointF> deltaPoints;

    double delta = -M_PI;
    double sMax = 0.0;
    double deltaMax = 0.0;
    while (delta < M_PI) {
        double s = 0;
        int i = 0;
        foreach (Cell* q, frontiers) {
            s += m_handler->disCoverage(robot->position(), delta, q->rect().center(), allPaths[i]);
            ++i;
        }

        deltaPoints.append(QPointF(delta, s));

        if (s > sMax) {
            sMax = s;
            deltaMax = delta;
        }
        delta += 0.1;
    }

    m_data = deltaPoints;

    // follow local optimum
    if (m_handler->followLocalOptimum()) {
        int i;
        double robotOrientation = robot->orientation();
        for (i = 0; i < deltaPoints.size(); ++i) {
            if (qFuzzyCompare(deltaPoints[i].x(), robotOrientation))
                break;
        }

        // first time m_delta == 0.0, so no index found
        if (deltaPoints.size() == i) {
            i = 0;
        }

        const int n = deltaPoints.size();
        int c = 0; // avoid infinite loop
        while (deltaPoints[i].y() <= deltaPoints[(i + 1) % n].y() && c < n) {
            i = (i + 1) % n;
            ++c;
        }

        c = 0;
        while (deltaPoints[i].y() <= deltaPoints[(i - 1 + n) % n].y() && c < n) {
            i = (i - 1 + n) % n;
            ++c;
        }

        setCurrentOrientation(deltaPoints[i]);
    } else {
        // always global optimum
        setCurrentOrientation(QPointF(deltaMax, sMax));
    }

    update();
}

void OrientationPlotter::setCurrentOrientation(const QPointF& currentOrientation)
{
    m_currentOrientation = currentOrientation;
    update();
}

void OrientationPlotter::paintEvent(QPaintEvent* event)
{
    qreal maxY = 0.0;
    foreach (const QPointF& p, m_data) {
        if (maxY < p.y()) {
            maxY = p.y();
        }
    }

    QPainter p(this);
    p.fillRect(rect(), Qt::white);
    p.end();
    QFrame::paintEvent(event);
    p.begin(this);

    if (maxY == 0.0) {
        return;
    }

    const qreal scaleX = (width() / 2.0) / 3.5;
    const qreal scaleY = (height() - 5.0) / maxY;

    p.translate(width() / 2.0, height() - 2);
    p.scale(-scaleX, -scaleY);

    p.setRenderHints(QPainter::Antialiasing, true);

    p.drawLine(QPointF(-3.4, 0), QPointF(3.4, 0));
    p.drawLine(QPointF(0.0, 0.0), QPointF(0, maxY));

    for (int i = 0; i < m_data.size() - 1; ++i) {
        p.drawLine(m_data[i], m_data[i+1]);
    }

    // mark chosen orientation with a circle, filled by the robot color
    p.setPen(Qt::black);
    if (Robot* robot = RobotManager::self()->activeRobot()) {
        p.setBrush(robot->color());
    } else {
        p.setBrush(QColor(255, 0, 0, 128));
    }
    p.drawEllipse(m_currentOrientation, 3.0 / scaleX, 3.0 / scaleY);
}

// kate: replace-tabs on; indent-width 4;
