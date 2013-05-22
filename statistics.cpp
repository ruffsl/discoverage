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

#include "statistics.h"
#include "mainwindow.h"
#include "scene.h"
#include "gridmap.h"
#include "robotmanager.h"
#include "tikzexport.h"

#include <math.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QClipboard>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QSpinBox>
#include <QtGui/QPushButton>
#include <QtAlgorithms>

int TestRun::iterationForPercentExplored(qreal percent)
{
    for (int i = 0; i < stats.size(); ++i) {
        if (stats[i].percentExplored >= percent)
            return i;
    }

    return stats.size();
}

Stats::Stats()
    : iteration(0)
    , percentExplored(0.0)
    , percentUnemployed(0.0)
{
}

TestRun::TestRun()
    : testRun(0)
    , stats()
{
}

BoxPlotItem::BoxPlotItem()
{
    minimum = 0;
    lowerQuartile = 0;
    median = 0;
    upperQuartile = 0;
    maximum = 0;
}

Statistics::Statistics(MainWindow* mainWindow, QWidget* parent)
    : QFrame(parent)
    , m_mainWindow(mainWindow)
{
    setFrameStyle(Panel | Sunken);

    QHBoxLayout * hl = new QHBoxLayout(this);
    hl->addStretch();
    QVBoxLayout * l = new QVBoxLayout();
    hl->addLayout(l);
    m_sbRuns = new QSpinBox(this);
    m_sbRuns->setRange(1, 1000);
    m_sbRuns->setValue(100);
    m_sbRuns->setSuffix(QString(" runs"));
    l->addWidget(m_sbRuns, Qt::AlignRight);

    m_btnStartStop = new QPushButton("Start", this);
    l->addWidget(m_btnStartStop, Qt::AlignRight);
    connect(m_btnStartStop, SIGNAL(clicked()), this, SLOT(startStopBatchProcess()));

    QPushButton* btnExport = new QPushButton("TeX Export", this);
    l->addWidget(btnExport, Qt::AlignRight);
    connect(btnExport, SIGNAL(clicked()), this, SLOT(exportStatistics()));
}

Statistics::~Statistics()
{
}

QSize Statistics::sizeHint() const
{
    return QSize(-1, 120);
}

MainWindow* Statistics::mainWindow() const
{
    return m_mainWindow;
}

void Statistics::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing, true);
    p.fillRect(rect(), Qt::white);

    // draw exploration progress
    p.drawText(QPoint(20, 20), QString("Progress :%1%").arg(mainWindow()->scene()->map().explorationProgress() * 100));
    p.drawText(QPoint(20, 40), QString("Iteration: %1").arg(m_progress.size()));

    // prepare for progress line
    p.save();
    p.scale(1.0, -1.0);
    p.translate(10, -height() + 10);
    p.scale(2.0, (height() - 20) / 100.0);

    p.drawLine(0, 0, 0, 100);
    p.drawLine(0, 0, 100, 0);

    for (int i = 1; i < m_progress.size(); ++i) {
        p.drawLine(QPointF((i - 1)/4.0, m_progress[i-1]),
                   QPointF( i     /4.0, m_progress[i]));
    }
    p.restore();

    // Now paint batch statistics
    p.drawText(QPoint(230, 20), QString("Run: %1").arg(m_testRuns.size()));

    // prepare for progress line
    p.save();
    p.scale(1.0, -1.0);
    p.translate(220, -height() + 10);
    p.scale(2.0, (height() - 20) / 100.0);

    p.drawLine(0, 0, 0, 100);
    p.drawLine(0, 0, 300, 0);

    m_meanProgress.clear();
    m_meanUnemployed.clear();
    for (int i = 0; i < 300; ++i) {
        p.setPen(Qt::red);
        qreal mp = meanProgress(i) * 100.0;
        p.drawPoint(i, mp);
        p.setPen(Qt::darkGreen);
        p.drawPoint(i, mp - sqrt(varianceProgress(i)) * 100.0);


        p.setPen(Qt::blue);
        mp = meanUnemployed(i) * 100.0;
        p.drawPoint(i, mp);
        p.setPen(Qt::gray);
        p.drawPoint(i, mp + sqrt(varianceUnemployed(i)) * 100.0);
    }

    QPainterPath minPath;
    QPainterPath maxPath;
    QPainterPath medianPath;
    QPainterPath lowerPath;
    QPainterPath upperPath;
    const int imax = m_boxPlot.size() - 1;
    for (int i = 0; i < m_boxPlot.size(); ++i) {
        if (i == 0) {
            minPath.moveTo(QPointF(i, 100*m_boxPlot[i].minimum));
            maxPath.moveTo(QPointF(imax - i, 100*m_boxPlot[imax - i].maximum));
            medianPath.moveTo(QPointF(i, 100*m_boxPlot[i].median));
            lowerPath.moveTo(QPointF(i, 100*m_boxPlot[i].lowerQuartile));
            upperPath.moveTo(QPointF(imax - i, 100*m_boxPlot[imax - i].upperQuartile));
        } else {
            minPath.lineTo(QPointF(i, 100*m_boxPlot[i].minimum));
            maxPath.lineTo(QPointF(imax - i, 100*m_boxPlot[imax - i].maximum));
            medianPath.lineTo(QPointF(i, 100*m_boxPlot[i].median));
            lowerPath.lineTo(QPointF(i, 100*m_boxPlot[i].lowerQuartile));
            upperPath.lineTo(QPointF(imax - i, 100*m_boxPlot[imax - i].upperQuartile));
        }
    }

    QPainterPath minMaxPath;
    minMaxPath.addPath(minPath);
    minMaxPath.connectPath(maxPath);
    QPainterPath lowUpPath;
    lowUpPath.addPath(lowerPath);
    lowUpPath.connectPath(upperPath);

    minMaxPath = minMaxPath.simplified();
    lowUpPath = lowUpPath.simplified();

    p.fillPath(minMaxPath, QColor(255, 225, 196));
    p.fillPath(lowUpPath, QColor(255, 210, 210));
    p.setPen(QColor(255, 128, 128));
    p.drawPath(minPath);
    p.setPen(QColor(128, 255, 128));
    p.drawPath(maxPath);
    p.setPen(QColor(128, 128, 255));
    p.drawPath(medianPath);
    p.setPen(QColor(180, 180, 180));
    p.drawPath(lowerPath);
    p.drawPath(upperPath);
    p.restore();

    // Now paint batch statistics
    qreal mean, sigma;
    statsForPercentExplored(0.9, mean, sigma);
    p.drawText(QPoint(830, 20), QString(" 90%: %1 (%2)").arg(mean).arg(sigma));
    statsForPercentExplored(0.95, mean, sigma);
    p.drawText(QPoint(830, 40), QString(" 95%: %1 (%2)").arg(mean).arg(sigma));
    statsForPercentExplored(0.98, mean, sigma);
    p.drawText(QPoint(830, 60), QString(" 98%: %1 (%2)").arg(mean).arg(sigma));
    statsForPercentExplored(1, mean, sigma);
    p.drawText(QPoint(830, 80), QString("100%: %1 (%2)").arg(mean).arg(sigma));

    p.end();

    QFrame::paintEvent(event);
}

int Statistics::iteration() const
{
    return m_progress.size();
}

void Statistics::reset()
{
    m_progress.clear();
    update();

    m_testRuns.append(TestRun());
    m_testRuns.last().testRun = m_testRuns.size();
}

void Statistics::tick()
{
    GridMap& m = mainWindow()->scene()->map();

    const qreal progress = m.explorationProgress();
    m_progress.append(progress * 100);
    update();

    if (m_testRuns.size()) {
        qreal unemployed;
        const int count = RobotManager::self()->count();
        for (int i = 0; i < count; ++i) {
            if (RobotManager::self()->robot(i)->stats().isUnemployed())
                unemployed += 1;
        }
        unemployed /= count;

        m_testRuns.last().stats.append(Stats());
        m_testRuns.last().stats.last().iteration = m_progress.size();
        m_testRuns.last().stats.last().percentExplored = progress;
        m_testRuns.last().stats.last().percentUnemployed = unemployed;
    }
}

void Statistics::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu m;
    QAction* a = m.addAction("Export to Clipboard");
    if (a == m.exec(event->globalPos())) {
        QString data = "\\draw ";
        for (int i = 0; i < m_progress.size(); ++i) {
            data += QString("(%1, %2)").arg(i).arg(m_progress[i]);
            if (i+1 < m_progress.size()) {
                data += " -- ";
            }
        }
        QApplication::clipboard()->setText(data);
    }
}

qreal Statistics::meanProgress(int iteration)
{
    const int N = m_testRuns.size();
    qreal meanExplored = 0.0;
    for (int i = 0; i < N; ++i) {
        qreal explored = 1.0;
        if (iteration < m_testRuns[i].stats.size()) {
            explored = m_testRuns[i].stats[iteration].percentExplored;
        }
        meanExplored += explored;
    }
    m_meanProgress.append(meanExplored / N);
    return meanExplored / N;
}

qreal Statistics::varianceProgress(int iteration)
{
//     qDebug () << m_meanProgress.size() << iter action;
    if (iteration >= m_meanProgress.size()) return 0;

    const qreal mean = m_meanProgress[iteration];

    const int N = m_testRuns.size();
    qreal variance = 0.0;
    for (int i = 0; i < N; ++i) {
        qreal explored = 1.0;
        if (iteration < m_testRuns[i].stats.size()) {
            explored = m_testRuns[i].stats[iteration].percentExplored;
        }
        variance += (explored - mean) * (explored - mean);
    }
    return variance / N;
}

qreal Statistics::meanUnemployed(int iteration)
{
    const int N = m_testRuns.size();
    qreal meanUnemployed= 0.0;
    for (int i = 0; i < N; ++i) {
        qreal unemployed = 1.0;
        if (iteration < m_testRuns[i].stats.size()) {
            unemployed = m_testRuns[i].stats[iteration].percentUnemployed;
        }
        meanUnemployed += unemployed;
    }
    m_meanUnemployed.append(meanUnemployed / N);
    return meanUnemployed / N;
}

qreal Statistics::varianceUnemployed(int iteration)
{
//     qDebug () << m_meanUnemployed.size() << iter action;
    if (iteration >= m_meanUnemployed.size()) return 0;

    const qreal mean = m_meanUnemployed[iteration];

    const int N = m_testRuns.size();
    qreal variance = 0.0;
    for (int i = 0; i < N; ++i) {
        qreal unemployed = 1.0;
        if (iteration < m_testRuns[i].stats.size()) {
            unemployed = m_testRuns[i].stats[iteration].percentUnemployed;
        }
        variance += (unemployed - mean) * (unemployed - mean);
    }
    return variance / N;
}

void Statistics::statsForPercentExplored(qreal percent, qreal & meanIteration, qreal & stdDeviation)
{
    meanIteration = 0;
    stdDeviation = 0;

    const int N = m_testRuns.size();

    if (N < 2) return;

    // calculate mean
    for (int i = 0; i < N; ++i) {
        meanIteration += m_testRuns[i].iterationForPercentExplored(percent);
    }
    meanIteration /= N;

    // calculate variance
    for (int i = 0; i < N; ++i) {
        qreal it = m_testRuns[i].iterationForPercentExplored(percent);
        stdDeviation += (it - meanIteration) * (it - meanIteration);
    }

    // empirical standard deviation
    stdDeviation =  sqrt(stdDeviation / (N - 1));
}

QPointF Statistics::randomRobotPos(int robot)
{
    const QSizeF worldSize = m_mainWindow->scene()->map().worldSize();
    while (true) {
        const QPointF worldPos((rand() * worldSize.width()) / RAND_MAX,
                               (rand() * worldSize.height()) / RAND_MAX);
        const QPoint cellIndex = m_mainWindow->scene()->map().worldToIndex(worldPos);

        // make sure cell is valid and no obstacle
        if (!m_mainWindow->scene()->map().isValidField(cellIndex) ||
            m_mainWindow->scene()->map().cell(cellIndex).isObstacle())
        {
            continue;
        }

        // make sure no other robots is in the same cell
        for (int i = 0; i < robot; ++i) {
            const QPoint robotIndex = m_mainWindow->scene()->map().worldToIndex(
                    RobotManager::self()->robot(i)->position());
            if (robotIndex == cellIndex)
                continue;
        }

        return worldPos;
    }
}

void Statistics::startStopBatchProcess()
{
    if (!m_batchProcessRunning) {
        m_batchProcessRunning = true;
        m_btnStartStop->setText("Stop");

        // prepare
        m_testRuns.clear();
        m_boxPlot.clear();

        // reproducible random numbers
        srand(1);

        int maxIterations = 0;

        int run = 0;
        while (m_batchProcessRunning && run < m_sbRuns->value()) {
            m_mainWindow->reloadScene();

            // randomize robot positions
            for (int i = 0; i < RobotManager::self()->count(); ++i) {
                RobotManager::self()->robot(i)->setPosition(randomRobotPos(i));
            }

            // do one run
            while (m_batchProcessRunning && m_mainWindow->scene()->map().explorationProgress() < 1.0) {
                m_mainWindow->tick();
                QApplication::processEvents();
            }
            if (m_progress.size() > maxIterations) {
                maxIterations = m_progress.size();
            }
            ++run;
        }

        // generate box plots:
        m_boxPlot.resize(maxIterations);
        for (int it = 0; it < maxIterations; ++it) {
            QVector<qreal> percentExploredList = percentList(it);
            const int count = percentExploredList.size();
            Q_ASSERT(count > 0);

            m_boxPlot[it].minimum = percentExploredList.first();
            m_boxPlot[it].maximum = percentExploredList.last();
            m_boxPlot[it].median = percentExploredList[count * 0.5];
            m_boxPlot[it].lowerQuartile = percentExploredList[count * 0.25];
            m_boxPlot[it].upperQuartile = percentExploredList[count * 0.75];
        }
    }

    if (m_batchProcessRunning) {
        m_batchProcessRunning = false;
        m_btnStartStop->setText("Start");
    }
}

QVector<qreal> Statistics::percentList(int iteration) const
{
    QVector<qreal> vec;
    for (int run = 0; run < m_testRuns.size(); ++run) {
        if (iteration < m_testRuns[run].stats.size()) {
            vec.append(m_testRuns[run].stats[iteration].percentExplored);
        } else {
            vec.append(1.0);
        }
    }
    qSort(vec.begin(), vec.end());
    return vec;
}

void Statistics::exportStatistics()
{
    // create paths to export
    QVector<QPointF> minPath;
    QVector<QPointF> maxPath;
    QVector<QPointF> medianPath;
    QVector<QPointF> lowerPath;
    QVector<QPointF> upperPath;
    for (int i = 0; i < m_boxPlot.size(); ++i) {
        minPath.append(QPointF(i, 100*m_boxPlot[i].minimum));
        maxPath.append(QPointF(i, 100*m_boxPlot[i].maximum));
        medianPath.append(QPointF(i, 100*m_boxPlot[i].median));
        lowerPath.append(QPointF(i, 100*m_boxPlot[i].lowerQuartile));
        upperPath.append(QPointF(i, 100*m_boxPlot[i].upperQuartile));
    }

    // create paths to fill
    QVector<QPointF> minMaxPath = maxPath;
    std::reverse(minMaxPath.begin(), minMaxPath.end());
    minMaxPath = minPath + minMaxPath;

    QVector<QPointF> lowUpPath = upperPath;
    std::reverse(lowUpPath.begin(), lowUpPath.end());
    lowUpPath = lowerPath + lowUpPath;

    // compute time-optimal-case
    const int totalCells = m_mainWindow->scene()->map().freeCellCount();
    const qreal res = m_mainWindow->scene()->map().resolution();
    const qreal range = RobotManager::self()->robot(0)->sensingRange();
    const qreal startCells = 100.0 * RobotManager::self()->count() * M_PI * range * range / (res * res * totalCells);
    const QPointF tocStart(0, startCells);
    const qreal cellsPerIteration = RobotManager::self()->count() * 2 * range / res;
    const QPointF tocEnd(totalCells / cellsPerIteration, 100);

    // export file name
    const QString numRobots = QString::number(RobotManager::self()->count());
    QString fileName = m_mainWindow->sceneBaseName()
        + "-" + mainWindow()->scene()->toolHandler()->name()
        + "-robots-" + numRobots
        + "-statistics.tikz";

    QFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream ts(&file);
        QTikzPicture tikzPicture;
        tikzPicture.setStream(&ts);

        tikzPicture.begin("yscale=0.05, xscale=0.1, thick");
        tikzPicture.line(minMaxPath, "draw=orange!50!black, fill=orange!50");
        tikzPicture.line(lowUpPath, "gray, densely dashed, fill=green!20");
//         tikzPicture.line(minPath, "green!50!black");
//         tikzPicture.line(maxPath, "darkRed");
        tikzPicture.line(medianPath, "blue");

        // time-optimal case
        tikzPicture.line(tocStart, tocEnd, "black");

        // axes
        tikzPicture.newline();
        tikzPicture.comment("axes");
        tikzPicture.line(QPointF(0, 0), QPointF(m_boxPlot.size() + 5, 0), "->, >=stealth'");
        tikzPicture.line(QPointF(0, 0), QPointF(0, 105), "->, >=stealth'");

        tikzPicture.end();

        file.close();
    }

//     // Now paint batch statistics
//     qreal mean, sigma;
//     statsForPercentExplored(0.9, mean, sigma);
//     p.drawText(QPoint(830, 20), QString(" 90%: %1 (%2)").arg(mean).arg(sigma));
//     statsForPercentExplored(0.95, mean, sigma);
//     p.drawText(QPoint(830, 40), QString(" 95%: %1 (%2)").arg(mean).arg(sigma));
//     statsForPercentExplored(0.98, mean, sigma);
//     p.drawText(QPoint(830, 60), QString(" 98%: %1 (%2)").arg(mean).arg(sigma));
//     statsForPercentExplored(1, mean, sigma);
//     p.drawText(QPoint(830, 80), QString("100%: %1 (%2)").arg(mean).arg(sigma));
}
// kate: replace-tabs on; indent-width 4;
