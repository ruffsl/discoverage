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

#include "discoveragehandler.h"
#include "scene.h"
#include "mainwindow.h"
#include "ui_discoveragewidget.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>
#include <QtGui/QDockWidget>

#include <math.h>

//BEGIN DisCoverageHandler
DisCoverageHandler::DisCoverageHandler(Scene* scene)
    : QObject()
    , ToolHandler(scene)
    , m_dock(0)
    , m_ui(0)
    , m_plotter(0)
{
}

DisCoverageHandler::~DisCoverageHandler()
{
    delete m_ui;
}

void DisCoverageHandler::toolHandlerActive(bool activated)
{
    dockWidget()->setVisible(activated);
}

QDockWidget* DisCoverageHandler::dockWidget()
{
    if (!m_dock) {
        m_dock = new QDockWidget("DisCoverage", scene()->mainWindow());
        m_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        m_ui = new Ui::DisCoverageWidget();
        QWidget* w = new QWidget();
        m_ui->setupUi(w);
        m_plotter = new OrientationPlotter(w);
        w->layout()->addWidget(m_plotter);
        m_dock->setWidget(w);
        scene()->mainWindow()->addDockWidget(Qt::RightDockWidgetArea, m_dock);
        
        connect(m_ui->chkShowVectorField, SIGNAL(toggled(bool)), this, SLOT(showVectorField(bool)));
    }
    return m_dock;
}

void DisCoverageHandler::showVectorField(bool show)
{
    if (!show) {
        m_vectorField.clear();
    } else {
        GridMap&m = scene()->map();
        const QSize s = m.size();
        const qreal resolution = m.resolution();
        m_vectorField = QVector<QVector<QLineF> >(s.width(), QVector<QLineF>(s.height()));

        const QSet<Cell*>& frontiers = m.frontiers();
        for (int a = 0; a < s.width(); ++a) {
            for (int b = 0; b < s.height(); ++b) {
                if (m.cell(a, b).state() != (Cell::Free | Cell::Explored)) {
                    continue;
                }
                qDebug() << "next one" << a << b;
                QPointF center(resolution / 2.0 + a * resolution,
                               resolution / 2.0 + b * resolution);

                QList<Path> allPaths;
                allPaths = m.frontierPaths(QPoint(a, b));
                for (int i = 0; i < allPaths.size(); ++i) {
                    allPaths[i].beautify(m);
                }

                float delta = -M_PI;
                float sMax = 0.0f;
                float deltaMax = 0.0f;
                while (delta < M_PI) {
                    float s = 0;
                    int i = 0;
                    foreach (Cell* q, frontiers) {
                        s += disCoverage(center, delta, q->rect().center(), allPaths[i]);
                        ++i;
                    }
                    
                    if (s > sMax) {
                        sMax = s;
                        deltaMax = delta;
                    }
                    delta += 0.1f;
                }
                m_vectorField[a][b].setP1(center);
                m_vectorField[a][b].setP2(QPointF(center.x() + resolution, center.y()));
                m_vectorField[a][b].setAngle(-deltaMax * 180.0 / M_PI);
            }
        }
    }
}

void DisCoverageHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);

    GridMap &m = scene()->map();
    p.scale(m.scaleFactor(), m.scaleFactor());
    
    QPainter::RenderHints rh = p.renderHints();
    p.setRenderHints(QPainter::Antialiasing, true);
    
    for (int a = 0; a < m_vectorField.size(); ++a) {
        const int s = m_vectorField[0].size();
        for (int b = 0; b < s; ++b) {
            p.drawLine(m_vectorField[a][b]);
        }
    }

//     foreach (const Path& path, m_allPaths) {
// 
//         for (int i = 0; i < path.m_path.size() - 1; ++i) {
//             const QPoint& a = path.m_path[i];
//             const QPoint& b = path.m_path[i+1];
//             p.drawLine(m.cell(a.x(), a.y()).rect().center(), m.cell(b.x(), b.y()).rect().center());
//         }
//     }

    p.setPen(QPen(Qt::red, 0.2));
    p.drawLine(m_robotPosition, m_robotPosition + QPointF(cos(m_delta), sin(m_delta)));

    p.setRenderHints(rh, true);
}

void DisCoverageHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);

    if (event->buttons() & Qt::LeftButton) {
        m_robotPosition = scene()->map().mapScreenToMap(event->posF());
        updateDisCoverage(m_robotPosition);
    }
}

void DisCoverageHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void DisCoverageHandler::tick()
{
    updateDisCoverage(m_robotPosition);
    m_robotPosition += QPointF(cos(m_delta), sin(m_delta)) * scene()->map().resolution();
    scene()->map().explore(m_robotPosition, 2.0, Cell::Explored);
    scene()->update();
}

void DisCoverageHandler::updateDisCoverage(const QPointF& robotPosition)
{
    const QSet<Cell*>& frontiers = scene()->map().frontiers();
    GridMap& m = scene()->map();
    QPoint pt = m.mapMapToCell(robotPosition);
    QList<Path> allPaths = m.frontierPaths(pt);
    for (int i = 0; i < allPaths.size(); ++i) {
        allPaths[i].beautify(m);
    }

    QVector<QPointF> deltaPoints;

    float delta = -M_PI;
    float sMax = 0.0f;
    float deltaMax = 0.0f;
    while (delta < M_PI) {
        float s = 0;
        int i = 0;
        foreach (Cell* q, frontiers) {
            s += disCoverage(robotPosition, delta, q->rect().center(), allPaths[i]);
            ++i;
        }

        deltaPoints.append(QPointF(delta, s));

        if (s > sMax) {
            sMax = s;
            deltaMax = delta;
        }
        delta += 0.1f;
    }

    m_plotter->setData(deltaPoints);

#if 1
    int i;
    for (i = 0; i < deltaPoints.size(); ++i) {
        if (deltaPoints[i].x() == m_delta)
            break;
    }

    // first time m_delta == 0.0, so no index found
    if (deltaPoints.size() == i) {
        m_delta = deltaMax;
        return;
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
    
    m_delta = deltaPoints[i].x();
#else
    m_delta = deltaMax;
#endif
}

float DisCoverageHandler::disCoverage(const QPointF& pos, float delta, const QPointF& q, const Path& path)
{
    if (path.m_path.size() < 2) {
        return 0.0f;
    }

    const float theta = 0.5;
    const float sigma = 2.0;

    const QPointF cellCenter = scene()->map().cell(path.m_path[1]).rect().center();

    // pos is continuous robot position
    // cellCenter is center of 2nd path cell
    const float dx = cellCenter.x() - pos.x();
    const float dy = cellCenter.y() - pos.y();

    float alpha = - delta + atan2(dy, dx);

    if (alpha > M_PI) alpha -= 2 * M_PI;
    else if (alpha < -M_PI) alpha += 2 * M_PI;

    float len = path.m_length * scene()->map().resolution();

    return exp(- alpha*alpha/(2.0*theta*theta)
               - len*len/(2.0*sigma*sigma));
}

#if 0
float DisCoverageHandler::disCoverage(const QPointF& pos, float delta, const QPointF& q, const Path& path)
{
    const float theta = 0.5;
    const float sigma = 2.0;

    const float dx = q.x() - pos.x();
    const float dy = q.y() - pos.y();

    float alpha = - delta + atan2(dy, dx);

    if (alpha > M_PI) alpha -= 2 * M_PI;
    else if (alpha < -M_PI) alpha += 2 * M_PI;

    float len = path.m_length*0.2;

    return exp(- alpha*alpha/(2.0*theta*theta)
               - len*len/(2.0*sigma*sigma));
}
#endif
//END DisCoverageHandler





OrientationPlotter::OrientationPlotter(QWidget* parent)
    : QFrame(parent)
{
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setFixedHeight(100);
}

OrientationPlotter::~OrientationPlotter()
{
}

void OrientationPlotter::setData(const QVector<QPointF>& data)
{
    m_data = data;
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
    QFrame::paintEvent(event);

    if (maxY == 0.0) {
        return;
    }

    p.translate(width() / 2.0, height() - 2);
    p.scale(-(width() / 2.0) / 3.5, - height() / maxY);

    p.drawLine(QPointF(-3.4, 0), QPointF(3.4, 0));
    p.drawLine(QPointF(0.0, 0.0), QPointF(0, maxY));

    for (int i = 0; i < m_data.size() - 1; ++i) {
        p.drawLine(m_data[i], m_data[i+1]);
    }
}

// kate: replace-tabs on; indent-width 4;
