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

#include "bullo.h"
#include "scene.h"
#include "mainwindow.h"
#include "tikzexport.h"
#include "robot.h"
#include "robotmanager.h"
#include "config.h"

#include "ui_discoveragefrontierwidget.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QDockWidget>

#include <math.h>

//BEGIN DisCoverageBulloHandler
DisCoverageBulloHandler::DisCoverageBulloHandler(Scene* scene)
    : QObject()
    , ToolHandler(scene)
    , m_dock(0)
    , m_ui(0)
{
    toolHandlerActive(false);
}

DisCoverageBulloHandler::~DisCoverageBulloHandler()
{
    delete m_ui;
}

void DisCoverageBulloHandler::toolHandlerActive(bool activated)
{
    dockWidget()->setVisible(activated);
}

QDockWidget* DisCoverageBulloHandler::dockWidget()
{
    if (!m_dock) {
        m_dock = new QDockWidget("DisCoverage (Frontier)", scene()->mainWindow());
        m_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

        m_ui = new Ui::DisCoverageFrontierWidget();
        QWidget* w = new QWidget();
        m_ui->setupUi(w);
        m_dock->setWidget(w);
        scene()->mainWindow()->addDockWidget(Qt::LeftDockWidgetArea, m_dock);

        connect(m_ui->sbIntegrationRange, SIGNAL(valueChanged(double)), this, SLOT(updateParameters()));

        updateParameters();
    }
    return m_dock;
}

void DisCoverageBulloHandler::setIntegrationRange(double range)
{
    m_ui->sbIntegrationRange->blockSignals(true);
    m_ui->sbIntegrationRange->setValue(range);
    m_ui->sbIntegrationRange->blockSignals(false);
}

double DisCoverageBulloHandler::integrationRange() const
{
    return m_ui->sbIntegrationRange->value();
}

void DisCoverageBulloHandler::save(QSettings& config)
{
    ToolHandler::save(config);

    config.beginGroup("dis-coverage-frontier-weights");
    config.setValue("integration-range", integrationRange());
    config.endGroup();
}

void DisCoverageBulloHandler::load(QSettings& config)
{
    ToolHandler::load(config);

    config.beginGroup("dis-coverage-frontier-weights");
    setIntegrationRange(config.value("integration-range", 0.5).toDouble());
    config.endGroup();
}

void DisCoverageBulloHandler::exportToTikz(QTikzPicture& tp)
{
    for (int i = 0; i < RobotManager::self()->count(); ++i)
        tp.circle(RobotManager::self()->robot(i)->position(), integrationRange(), "dashed, thick");
}

void DisCoverageBulloHandler::updateParameters()
{
    postProcess();
    scene()->update();
}

void DisCoverageBulloHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);

    // draw trajectories
    p.setRenderHints(QPainter::Antialiasing, true);
    p.setPen(QPen(Qt::blue, scene()->map().resolution() * 0.3));
    if (m_previewPath.size()) p.drawPolyline(&m_previewPath[0], m_previewPath.size());

    p.setPen(QPen(QColor(0, 0, 0, 196), scene()->map().resolution() * 0.3, Qt::DotLine));
    p.setBrush(Qt::NoBrush);
    for (int i = 0; i < RobotManager::self()->count(); ++i)
        p.drawEllipse(RobotManager::self()->robot(i)->position(), integrationRange(), integrationRange());


    // debug: show gradient interpolation nodes
    p.drawEllipse(g00, 0.05, 0.05);
    p.drawEllipse(g01, 0.05, 0.05);
    p.drawEllipse(g10, 0.05, 0.05);
    p.drawEllipse(g11, 0.05, 0.05);

    p.setRenderHints(QPainter::Antialiasing, false);
}

void DisCoverageBulloHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);

    if (event->buttons() & Qt::RightButton) {
        QPointF robotPos = scene()->map().screenToWorld(event->posF());
        updatePreviewTrajectory(robotPos);
    }
}

void DisCoverageBulloHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void DisCoverageBulloHandler::mouseReleaseEvent(QMouseEvent* event)
{
    ToolHandler::mouseReleaseEvent(event);

    if (event->buttons() ^ Qt::RightButton)
        m_previewPath.clear();
}

void DisCoverageBulloHandler::reset()
{
}

qreal DisCoverageBulloHandler::performance(const QPointF& p, const QPointF& q)
{
    const qreal dx = p.x() - q.x();
    const qreal dy = p.y() - q.y();
    const qreal squareDist = (dx * dx + dy * dy);
    return -squareDist;
}

qreal DisCoverageBulloHandler::fitness(const QPointF& robotPos, const QVector<Cell*>& cells)
{
    qreal sum = 0;
    foreach (Cell* cell, cells) {
        sum += performance(robotPos, cell->center()) * cell->density();
    }
    return sum;
}

QPointF DisCoverageBulloHandler::gradient(Robot* robot, bool interpolate)
{
    if (interpolate) {
        return interpolatedGradient(robot->position(), robot);
    } else {
        QVector<Cell*> visibleCells = scene()->map().visibleCells(robot, integrationRange());
        return gradient(robot->position(), visibleCells);
    }
}

QPointF DisCoverageBulloHandler::gradient(const QPointF& robotPos, const QVector<Cell*>& visibleCells)
{
    static const qreal dx = 0.005;
    static const qreal dy = 0.005;
    const qreal x1 = fitness(robotPos - QPointF(dx, 0.0), visibleCells);
    const qreal x2 = fitness(robotPos + QPointF(dx, 0.0), visibleCells);
    const qreal y1 = fitness(robotPos - QPointF(0.0, dy), visibleCells);
    const qreal y2 = fitness(robotPos + QPointF(0.0, dy), visibleCells);

    QPointF grad ((x2 - x1) / (2*dx), (y2 - y1) / (2*dy));
    if (!grad.isNull()) {
        // normalize vector (1 sqrt)
        grad /= sqrt(grad.x() * grad.x() + grad.y() * grad.y());
    }
    return grad;
}

QPointF DisCoverageBulloHandler::interpolatedGradient(const QPointF& robotPos, Robot* robot)
{
    GridMap& m = scene()->map();
    QPoint cellIndex(m.worldToIndex(robotPos));

    const double diffx = 1.0 - fabs(robotPos.x() - m.cell(cellIndex).center().x()) / scene()->map().resolution();
    const double diffy = 1.0 - fabs(robotPos.y() - m.cell(cellIndex).center().y()) / scene()->map().resolution();

    const int dx = (robotPos.x() < m.cell(cellIndex).center().x()) ? -1 : 1;
    const int dy = (robotPos.y() < m.cell(cellIndex).center().y()) ? -1 : 1;

    g00 = (m.cell(cellIndex).center());
    g01 = g00;
    g10 = g00;
    g11 = g00;
    if (m.isValidField(cellIndex + QPoint(dx, 0))) g01 = (m.cell(cellIndex + QPoint(dx, 0)).center());
    if (m.isValidField(cellIndex + QPoint(0, dy))) g10 = (m.cell(cellIndex + QPoint(0, dy)).center());
    if (m.isValidField(cellIndex + QPoint(dx,dy))) g11 = (m.cell(cellIndex + QPoint(dx, dy)).center());

    QVector<Cell*> c00 = m.visibleCells(g00, integrationRange());
    QVector<Cell*> c01 = m.visibleCells(g01, integrationRange());
    QVector<Cell*> c10 = m.visibleCells(g10, integrationRange());
    QVector<Cell*> c11 = m.visibleCells(g11, integrationRange());
    if (robot) {
        m.filterCells(c00, robot);
        m.filterCells(c01, robot);
        m.filterCells(c10, robot);
        m.filterCells(c11, robot);
    }

    QPointF grad00(gradient(g00, c00));
    QPointF grad01(gradient(g01, c01));
    QPointF grad10(gradient(g10, c10));
    QPointF grad11(gradient(g11, c11));

    QPointF gradX0(diffx * grad00 + (1 - diffx) * grad01);
    QPointF gradX1(diffx * grad10 + (1 - diffx) * grad11);

    QPointF grad(diffy * gradX0 + (1 - diffy) * gradX1);

    return grad;
}

void DisCoverageBulloHandler::tick()
{
}

void DisCoverageBulloHandler::postProcess()
{
    scene()->map().computeVoronoiPartition();
    for (int i = 0; i < RobotManager::self()->count(); ++i)
        scene()->map().computeDistanceTransform(RobotManager::self()->robot(i));
    scene()->map().updateDensity();

    if (Config::self()->showVectorField()) {
        updateVectorField();
    }

    scene()->map().updateCache();
}

void DisCoverageBulloHandler::updateVectorField()
{
    const int dx = scene()->map().size().width();
    const int dy = scene()->map().size().height();

    const bool hasPartition = RobotManager::self()->count() > 1;

    QVector<Cell*> visibleCells;
    QPointF grad;

    for (int a = 0; a < dx; ++a) {
        for (int b = 0; b < dy; ++b) {
            Cell& c = scene()->map().cell(a, b);
            if (c.state() == (Cell::Explored | Cell::Free)) {
                visibleCells = scene()->map().visibleCells(c.center(), integrationRange());
                if (hasPartition && c.robot() != 0) {
                    // honor Voronoi partition
                    scene()->map().filterCells(visibleCells, c.robot());
                }
                grad = gradient(c.center(), visibleCells);
                c.setGradient(grad);
            }
        }
    }
}

void DisCoverageBulloHandler::updatePreviewTrajectory(const QPointF& robotPos)
{
    m_previewPath.clear();
    m_previewPath.append(robotPos);

    const QPoint index = scene()->map().worldToIndex(robotPos);
    if (!scene()->map().isValidField(index)) return;

    Robot* robot = RobotManager::self()->count() > 1 ? scene()->map().cell(index).robot() : 0;
    double length = 0;

    do {
        const QPointF& lastPos = m_previewPath.last();
        const QPointF& nextPos = lastPos + interpolatedGradient(lastPos, robot) * scene()->map().resolution();
        m_previewPath.append(nextPos);
        const QPointF& cmpPos = m_previewPath[qMax(0, m_previewPath.size() - 5)];
        length = (nextPos - cmpPos).manhattanLength();
    } while (length >= scene()->map().resolution() &&
        !(scene()->map().cell(scene()->map().worldToIndex(m_previewPath.last())).state() & Cell::Frontier)
    );
}

//END DisCoverageBulloHandler

// kate: replace-tabs on; indent-width 4;
