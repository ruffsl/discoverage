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

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QDockWidget>

#include <math.h>

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
//     delete m_ui;
}

void DisCoverageBulloHandler::toolHandlerActive(bool activated)
{
//     dockWidget()->setVisible(activated);
}

QDockWidget* DisCoverageBulloHandler::dockWidget()
{
    if (!m_dock) {
//         m_dock = new QDockWidget("DisCoverage", scene()->mainWindow());
//         m_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

//         m_ui = new Ui::DisCoverageWidget();
//         QWidget* w = new QWidget();
//         m_ui->setupUi(w);
//         m_plotter = new OrientationPlotter(w);
//         m_ui->gbOptimization->layout()->addWidget(m_plotter);
//         m_dock->setWidget(w);
//         scene()->mainWindow()->addDockWidget(Qt::RightDockWidgetArea, m_dock);

//         connect(m_ui->sbVisionRaius, SIGNAL(valueChanged(double)), this, SLOT(updateParameters()));

//         updateParameters();
    }
    return m_dock;
}

void DisCoverageBulloHandler::save(QSettings& config)
{
    ToolHandler::save(config);
}

void DisCoverageBulloHandler::load(QSettings& config)
{
    ToolHandler::load(config);
}

void DisCoverageBulloHandler::exportToTikz(QTextStream& ts)
{
}

void DisCoverageBulloHandler::updateParameters()
{
//     m_visionRadius = m_ui->sbVisionRaius->value();

    scene()->update();
}

void DisCoverageBulloHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);

    p.scale(scene()->map().scaleFactor(), scene()->map().scaleFactor());

    // draw trajectories
    p.setRenderHints(QPainter::Antialiasing, true);
    p.setPen(Qt::blue);
    if (m_previewPath.size()) p.drawPolyline(&m_previewPath[0], m_previewPath.size());

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
        QPointF robotPos = scene()->map().mapScreenToMap(event->posF());
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
//     qDebug() << sum;
    return sum;
}

QPointF DisCoverageBulloHandler::gradient(Robot* robot, bool interpolate)
{
    if (interpolate) {
        return interpolatedGradient(robot->position());
    } else {
        return gradient(robot->position());
    }
}

QPointF DisCoverageBulloHandler::gradient(const QPointF& robotPos)
{
    QVector<Cell*> visibleCells = scene()->map().visibleCells(robotPos, 0.5);

    const qreal dx = 0.005;
    const qreal dy = 0.005;
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

QPointF DisCoverageBulloHandler::interpolatedGradient(const QPointF& robotPos)
{
    GridMap& m = scene()->map();
    QPoint cellIndex(m.mapMapToCell(robotPos));

    const double diffx = 1.0 - fabs(robotPos.x() - m.cell(cellIndex).center().x()) / scene()->map().resolution();
    const double diffy = 1.0 - fabs(robotPos.y() - m.cell(cellIndex).center().y()) / scene()->map().resolution();

    const int dx = (robotPos.x() < m.cell(cellIndex).center().x()) ? -1 : 1;
    const int dy = (robotPos.y() < m.cell(cellIndex).center().y()) ? -1 : 1;

//     if (cellIndex.x() == 0 || cellIndex.y() == 0 ||
    
    g00 = (m.cell(cellIndex).center());
    g01 = (m.cell(cellIndex + QPoint(dx, 0)).center());
    g10 = (m.cell(cellIndex + QPoint(0, dy)).center());
    g11 = (m.cell(cellIndex + QPoint(dx, dy)).center());

    QPointF grad00(gradient(g00));
    QPointF grad01(gradient(g01));
    QPointF grad10(gradient(g10));
    QPointF grad11(gradient(g11));

    QPointF gradX0(diffx * grad00 + (1 - diffx) * grad01);
    QPointF gradX1(diffx * grad10 + (1 - diffx) * grad11);

    QPointF grad(diffy * gradX0 + (1 - diffy) * gradX1);

    return grad;
}

void DisCoverageBulloHandler::tick()
{
}

void DisCoverageBulloHandler::updateVectorField()
{
    qDebug() << "update vector field";
    const int dx = scene()->map().size().width();
    const int dy = scene()->map().size().height();

    for (int a = 0; a < dx; ++a) {
        for (int b = 0; b < dy; ++b) {
            Cell& c = scene()->map().cell(a, b);
            if (c.state() == (Cell::Explored | Cell::Free)) {
                QPointF grad(gradient(c.center()));
                c.setGradient(grad);
            }
        }
    }
}

void DisCoverageBulloHandler::updatePreviewTrajectory(const QPointF& robotPos)
{
    m_previewPath.clear();

    m_previewPath.append(robotPos);
    
    double length = 0;

    do {
        const QPointF& lastPos = m_previewPath.last();
        const QPointF& nextPos = lastPos + interpolatedGradient(lastPos) * scene()->map().resolution();
        m_previewPath.append(nextPos);
        const QPointF& cmpPos = m_previewPath[qMax(0, m_previewPath.size() - 5)];
        length = (nextPos - cmpPos).manhattanLength();
    } while (length >= scene()->map().resolution() &&
        !(scene()->map().cell(scene()->map().mapMapToCell(m_previewPath.last())).state() & Cell::Frontier)
    );
}

//END DisCoverageBulloHandler

// kate: replace-tabs on; indent-width 4;
