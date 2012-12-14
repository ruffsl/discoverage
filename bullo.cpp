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
// #include "ui_discoveragewidget.h"

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
    , m_robotPosition(0, 0)
    , m_gradient(0, 0)
{
    toolHandlerActive(false);
}

DisCoverageBulloHandler::~DisCoverageBulloHandler()
{
//     delete m_ui;
}

void DisCoverageBulloHandler::toolHandlerActive(bool activated)
{
    dockWidget()->setVisible(activated);
}

QDockWidget* DisCoverageBulloHandler::dockWidget()
{
    if (!m_dock) {
        m_dock = new QDockWidget("DisCoverage", scene()->mainWindow());
        m_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
//         m_ui = new Ui::DisCoverageWidget();
//         QWidget* w = new QWidget();
//         m_ui->setupUi(w);
//         m_plotter = new OrientationPlotter(w);
//         m_ui->gbOptimization->layout()->addWidget(m_plotter);
//         m_dock->setWidget(w);
//         scene()->mainWindow()->addDockWidget(Qt::RightDockWidgetArea, m_dock);

//         connect(m_ui->chkShowVectorField, SIGNAL(toggled(bool)), this, SLOT(showVectorField(bool)));
//         connect(m_ui->sbVisionRaius, SIGNAL(valueChanged(double)), this, SLOT(updateParameters()));

        updateParameters();
    }
    return m_dock;
}

void DisCoverageBulloHandler::save(QSettings& config)
{
    ToolHandler::save(config);

    config.beginGroup("dis-coverage");
    config.setValue("robot-position", m_robotPosition);
    config.endGroup();
}

void DisCoverageBulloHandler::load(QSettings& config)
{
    ToolHandler::load(config);

    config.beginGroup("dis-coverage");
    m_robotPosition = config.value("robot-position", QPointF(0.0, 0.0)).toPointF();
    config.endGroup();

//     m_ui->sbVisionRaius->blockSignals(true);
// 
//     m_ui->sbVisionRaius->setValue(m_visionRadius);
// 
//     m_ui->sbVisionRaius->blockSignals(false);
}

void DisCoverageBulloHandler::updateParameters()
{
//     m_visionRadius = m_ui->sbVisionRaius->value();

    scene()->update();
}

// void DisCoverageBulloHandler::showVectorField(bool show)
// {
//     if (!show) {
//         m_vectorField.clear();
//     } else {
//         GridMap&m = scene()->map();
//         const QSize s = m.size();
//         const qreal resolution = m.resolution();
//         m_vectorField = QVector<QVector<QLineF> >(s.width(), QVector<QLineF>(s.height()));
// 
//         const QSet<Cell*>& frontiers = m.frontiers();
//         for (int a = 0; a < s.width(); ++a) {
//             for (int b = 0; b < s.height(); ++b) {
//                 if (m.cell(a, b).state() != (Cell::Free | Cell::Explored)) {
//                     continue;
//                 }
//                 qDebug() << "next one" << a << b;
//                 QPointF center(resolution / 2.0 + a * resolution,
//                                resolution / 2.0 + b * resolution);
// 
//                 QList<Path> allPaths;
//                 allPaths = m.frontierPaths(QPoint(a, b));
//                 for (int i = 0; i < allPaths.size(); ++i) {
//                     allPaths[i].beautify(m);
//                 }
// 
//                 double delta = -M_PI;
//                 double sMax = 0.0;
//                 double deltaMax = 0.0;
//                 while (delta < M_PI) {
//                     double s = 0;
//                     int i = 0;
//                     foreach (Cell* q, frontiers) {
//                         s += disCoverage(center, delta, q->rect().center(), allPaths[i]);
//                         ++i;
//                     }
//                     
//                     if (s > sMax) {
//                         sMax = s;
//                         deltaMax = delta;
//                     }
//                     delta += 0.1;
//                 }
//                 m_vectorField[a][b].setP1(center);
//                 m_vectorField[a][b].setP2(QPointF(center.x() + resolution, center.y()));
//                 m_vectorField[a][b].setAngle(-deltaMax * 180.0 / M_PI);
//             }
//         }
//     }
// }

void DisCoverageBulloHandler::draw(QPainter& p)
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
    p.setBrush(Qt::NoBrush);
    QPen dashPen(QColor(0, 0, 0, 196), m.resolution() * 0.3, Qt::DotLine);
    p.setPen(dashPen);
    p.drawEllipse(m_robotPosition, 0.5, 0.5);

    p.setPen(bluePen);

    if (m_trajectory.size()) p.drawPolyline(&m_trajectory[0], m_trajectory.size());
    if (m_previewPath.size()) p.drawPolyline(&m_previewPath[0], m_previewPath.size());

    p.setPen(QPen(Qt::red));
    p.drawLine(m_robotPosition, m_robotPosition + m_gradient);

    p.setRenderHints(rh, true);
    
    foreach (Cell* c, m_visibleCells) {
        p.drawEllipse(c->center(), 0.05, 0.05);
    }
    
    p.drawEllipse(g00, 0.05, 0.05);
    p.drawEllipse(g01, 0.05, 0.05);
    p.drawEllipse(g10, 0.05, 0.05);
    p.drawEllipse(g11, 0.05, 0.05);

}

void DisCoverageBulloHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);

    if (event->buttons() & Qt::LeftButton) {
        m_robotPosition = scene()->map().mapScreenToMap(event->posF());
    }

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
    m_trajectory.clear();
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

QPointF DisCoverageBulloHandler::gradient(const QPointF& robotPos)
{
    QVector<Cell*> visibleCells = scene()->map().visibleCells(robotPos, 0.5/*operationRadius()/2*/, Cell::Free);

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
    if (m_trajectory.size() == 0) {
        m_trajectory.append(m_robotPosition);
    }

    m_robotPosition += interpolatedGradient(m_robotPosition) * scene()->map().resolution();
//     m_robotPosition += gradient(m_robotPosition) * scene()->map().resolution();

    scene()->map().exploreInRadius(m_robotPosition, operationRadius(), Cell::Explored);

    m_trajectory.append(m_robotPosition);

    scene()->map().updateCellWeights();
//     m_gradient = gradient(m_robotPosition);
    
    m_visibleCells = scene()->map().visibleCells(m_robotPosition, 0.5/*operationRadius()/2*/, Cell::Free);
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
