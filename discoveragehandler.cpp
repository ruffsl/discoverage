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
#include <QtCore/QSettings>
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
    toolHandlerActive(false);
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
        m_ui->gbOptimization->layout()->addWidget(m_plotter);
        m_dock->setWidget(w);
        scene()->mainWindow()->addDockWidget(Qt::RightDockWidgetArea, m_dock);
        
        connect(m_ui->chkShowVectorField, SIGNAL(toggled(bool)), this, SLOT(showVectorField(bool)));
        connect(m_ui->sbVisionRaius, SIGNAL(valueChanged(double)), this, SLOT(updateParameters()));
        connect(m_ui->sbTheta, SIGNAL(valueChanged(double)), this, SLOT(updateParameters()));
        connect(m_ui->sbSigma, SIGNAL(valueChanged(double)), this, SLOT(updateParameters()));
        connect(m_ui->chkLocalOptimum, SIGNAL(toggled(bool)), this, SLOT(updateParameters()));

        updateParameters();
    }
    return m_dock;
}

void DisCoverageHandler::save(QSettings& config)
{
    ToolHandler::save(config);

    config.beginGroup("dis-coverage");
    config.setValue("vision-radius", m_visionRadius);
    config.setValue("theta", m_theta);
    config.setValue("sigma", m_sigma);
    config.setValue("delta", m_delta);
    config.setValue("robot-position", m_robotPosition);
    config.setValue("local-optimum", m_ui->chkLocalOptimum->isChecked());
    config.setValue("auto-dist", m_ui->chkAutoDist->isChecked());
    config.endGroup();
}

void DisCoverageHandler::load(QSettings& config)
{
    ToolHandler::load(config);

    config.beginGroup("dis-coverage");
    m_visionRadius = config.value("vision-radius", 2.0).toDouble();
    m_theta = config.value("theta", 0.5).toDouble();
    m_sigma = config.value("sigma", 2.0).toDouble();
    m_delta = config.value("delta", 0.0).toDouble();
    m_robotPosition = config.value("robot-position", QPointF(0.0, 0.0)).toPointF();
    const bool localOptimum = config.value("local-optimum", true).toBool();
    const bool autoDist = config.value("auto-dist", false).toBool();
    config.endGroup();
    

    m_ui->sbVisionRaius->blockSignals(true);
    m_ui->sbTheta->blockSignals(true);
    m_ui->sbSigma->blockSignals(true);
    m_ui->chkLocalOptimum->blockSignals(true);

    m_ui->sbVisionRaius->setValue(m_visionRadius);
    m_ui->sbTheta->setValue(m_theta);
    m_ui->sbSigma->setValue(m_sigma);
    m_ui->chkLocalOptimum->setChecked(localOptimum);
    m_ui->chkAutoDist->setChecked(autoDist);

    m_ui->sbVisionRaius->blockSignals(false);
    m_ui->sbTheta->blockSignals(false);
    m_ui->sbSigma->blockSignals(false);
    m_ui->chkLocalOptimum->blockSignals(false);
}

void DisCoverageHandler::updateParameters()
{
    m_visionRadius = m_ui->sbVisionRaius->value();
    m_theta = m_ui->sbTheta->value();
    m_sigma = m_ui->sbSigma->value();

    updateDisCoverage(m_robotPosition);
    scene()->update();
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

                double delta = -M_PI;
                double sMax = 0.0;
                double deltaMax = 0.0;
                while (delta < M_PI) {
                    double s = 0;
                    int i = 0;
                    foreach (Cell* q, frontiers) {
                        s += disCoverage(center, delta, q->rect().center(), allPaths[i]);
                        ++i;
                    }
                    
                    if (s > sMax) {
                        sMax = s;
                        deltaMax = delta;
                    }
                    delta += 0.1;
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

    QPen bluePen(QColor(0, 0, 255, 196), m.resolution() * 0.3);
    p.setPen(bluePen);
    for (int a = 0; a < m_vectorField.size(); ++a) {
        const int s = m_vectorField[0].size();
        for (int b = 0; b < s; ++b) {
            p.drawLine(m_vectorField[a][b]);
        }
    }


    p.setOpacity(0.2);
    p.setBrush(QBrush(Qt::blue));
    p.drawEllipse(m_robotPosition, m_visionRadius, m_visionRadius);
    p.setOpacity(1.0);

    if (m_trajectory.size()) p.drawPolyline(&m_trajectory[0], m_trajectory.size());

    p.setPen(QPen(Qt::red, m.resolution() * 0.5));
    p.drawLine(m_robotPosition, m_robotPosition + QPointF(cos(m_delta), sin(m_delta)) * scene()->map().resolution());

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

void DisCoverageHandler::reset()
{
    m_trajectory.clear();
}

void DisCoverageHandler::tick()
{
    if (m_trajectory.size() == 0) {
        m_trajectory.append(m_robotPosition);
    }

    updateDisCoverage(m_robotPosition);
    m_robotPosition += QPointF(cos(m_delta), sin(m_delta)) * scene()->map().resolution();
    scene()->map().explore(m_robotPosition, m_visionRadius, Cell::Explored);

    m_trajectory.append(m_robotPosition);
    
    scene()->update();
}

void DisCoverageHandler::updateDisCoverage(const QPointF& robotPosition)
{
    const QSet<Cell*>& frontiers = scene()->map().frontiers();
    GridMap& m = scene()->map();
    QPoint pt = m.mapMapToCell(robotPosition);
    QList<Path> allPaths = m.frontierPaths(pt);
    double shortestPath = 1000000000.0;
    for (int i = 0; i < allPaths.size(); ++i) {
        allPaths[i].beautify(m);
        if (allPaths[i].m_length < shortestPath) {
            shortestPath = allPaths[i].m_length;
        }
    }

    if (m_ui->chkAutoDist->isChecked()) {
        m_sigma = shortestPath;
        m_ui->sbSigma->blockSignals(true);
        m_ui->sbSigma->setValue(shortestPath);
        m_ui->sbSigma->blockSignals(false);
    }

    QVector<QPointF> deltaPoints;

    double delta = -M_PI;
    double sMax = 0.0;
    double deltaMax = 0.0;
    while (delta < M_PI) {
        double s = 0;
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
        delta += 0.1;
    }

    m_plotter->setData(deltaPoints);

    // follow local optimum
    if (m_ui->chkLocalOptimum->isChecked()) {
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
        m_plotter->setCurrentOrientation(deltaPoints[i]);
    } else {
        // always global optimum
        m_delta = deltaMax;
        m_plotter->setCurrentOrientation(QPointF(m_delta, sMax));
    }
}

double DisCoverageHandler::disCoverage(const QPointF& pos, double delta, const QPointF& q, const Path& path)
{
    if (path.m_path.size() < 2) {
        return 0.0f;
    }

    const double theta = m_theta;
    const double sigma = m_sigma;

    const QPointF cellCenter = scene()->map().cell(path.m_path[1]).rect().center();

    // pos is continuous robot position
    // cellCenter is center of 2nd path cell
    const double dx = cellCenter.x() - pos.x();
    const double dy = cellCenter.y() - pos.y();

    float alpha = - delta + atan2(dy, dx);

    if (alpha > M_PI) alpha -= 2 * M_PI;
    else if (alpha < -M_PI) alpha += 2 * M_PI;

    double len = path.m_length;

    return exp(- alpha*alpha/(2.0*theta*theta)
               - len*len/(2.0*sigma*sigma));
}
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
    QFrame::paintEvent(event);

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
    
    p.setPen(Qt::red);
    p.setBrush(QBrush(QColor(255, 0, 0, 128)));
    p.drawEllipse(m_currentOrientation, 3.0 / scaleX, 3.0 / scaleY);
}

// kate: replace-tabs on; indent-width 4;