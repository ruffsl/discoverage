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
#include <QTime>
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
    for (int i = 0; i < m_boxPlot.size(); ++i) {
        p.setPen(Qt::red);
        qreal mp = meanProgress(i) * 100.0;
        p.drawPoint(i, mp);
        p.setPen(Qt::darkGreen);
        p.drawPoint(i, mp - sqrt(varianceProgress(i)) * 100.0);


        p.setPen(Qt::blue);
        mp = 100 - meanUnemployed(i) * 100.0;
        p.drawPoint(i, mp);
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
    p.fillPath(lowUpPath, QColor(210, 255, 210));
    p.setPen(QColor(255, 128, 0));
    p.drawPath(minPath);
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
        qreal unemployed = 0.0;
        const int count = RobotManager::self()->count();
        if (progress < 1.0) { // only count as unemployed, if exploration is not finished
            for (int i = 0; i < count; ++i) {
                if (RobotManager::self()->robot(i)->stats().isUnemployed())
                    unemployed += 1;
            }
            unemployed /= count;
        }

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
    qreal meanUnemployed = 0.0;
    for (int i = 0; i < N; ++i) {
        qreal unemployed = 0.0;
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

        QTime tStart;
        tStart.start();

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

            // status info
            QTime tAll = tStart.addMSecs(((m_sbRuns->value() * tStart.elapsed()) / run));
            qDebug().nospace() << "[INFO] completed run " << run << " of " << m_sbRuns->value()
                               << ", completion at: " << tAll.toString("hh:mm");
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
    if (!m_boxPlot.size()) return;

    // create paths to export
    QVector<QPointF> minPath;
    QVector<QPointF> maxPath;
    QVector<QPointF> medianPath;
    QVector<QPointF> lowerPath;
    QVector<QPointF> upperPath;
    QVector<QPointF> unemployedPath;
    for (int i = 0; i < m_boxPlot.size(); ++i) {
        minPath.append(QPointF(i, 100*m_boxPlot[i].minimum));
        maxPath.append(QPointF(i, 100*m_boxPlot[i].maximum));
        medianPath.append(QPointF(i, 100*m_boxPlot[i].median));
        lowerPath.append(QPointF(i, 100*m_boxPlot[i].lowerQuartile));
        upperPath.append(QPointF(i, 100*m_boxPlot[i].upperQuartile));
        unemployedPath.append(QPointF(i, meanUnemployed(i) * 100.0));
    }

    // create paths to fill
    QVector<QPointF> minMaxPath = maxPath;
    std::reverse(minMaxPath.begin(), minMaxPath.end());
    minMaxPath = minPath + minMaxPath;

    QVector<QPointF> lowUpPath = upperPath;
    std::reverse(lowUpPath.begin(), lowUpPath.end());
    lowUpPath = lowerPath + lowUpPath;

    // compute time-optimal-case
    const qreal res = m_mainWindow->scene()->map().resolution();
    // (-res/1.75): cells must lie inside the circle, hence this approximate correction
    const qreal range = RobotManager::self()->robot(0)->sensingRange() - res / 1.75;
    const int totalCells = m_mainWindow->scene()->map().freeCellCount();
    const int startCells = RobotManager::self()->count() * M_PI * range * range / (res * res);
    const int restCells = totalCells - startCells;
    const QPointF tocStart(0, (100.0 * startCells) / totalCells);
    const qreal cellsPerIteration = RobotManager::self()->count() * 2.0 * range / res;
    const qreal percentPerIteration = cellsPerIteration / restCells;
    const QPointF tocEnd((1.0 - tocStart.y() / 100.0) / percentPerIteration, 100);

    // export file name
    const QString numRobots = QString::number(RobotManager::self()->count());
    QString fileName = m_mainWindow->sceneBaseName()
        + "-" + mainWindow()->scene()->toolHandler()->name()
        + "-robots-" + numRobots
        + "-range-" + QString::number(RobotManager::self()->robot(0)->sensingRange())
        + "-runs-" + QString::number(m_testRuns.size())
        + "-statistics.tikz";

    QFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream ts(&file);
        QTikzPicture tikzPicture;
        tikzPicture.setStream(&ts);

        tikzPicture.begin("thick");

        // plot data, scaled to 8cm
        tikzPicture.newline(2);
        tikzPicture.comment("plot data, scaled to 8cm");
        tikzPicture.beginScope(QString("yscale=0.05, xscale=%1").arg(8.0 / m_boxPlot.size()));

            tikzPicture.line(minMaxPath, "draw=orange, fill=orange!50");
            tikzPicture.line(lowUpPath, "gray, densely dashed, fill=green!20");
            tikzPicture.line(medianPath, "blue");
            tikzPicture.line(unemployedPath, "densely dotted, magenta");

            // time-optimal case
            tikzPicture.line(tocStart, tocEnd, "black");

            // draw grid
            tikzPicture << QString("\\draw[densely dashed, thin, black, ystep=20, xstep=10, opacity=0.3] (0, 0) grid (%1, 100);").arg(m_boxPlot.size());

            // axes
            tikzPicture.newline();
            tikzPicture.comment("axis lables");
            for (int i = 0; i <= m_boxPlot.size(); i += m_boxPlot.size() >= 100 ? 20 : 10) {
                tikzPicture << QString("\\node[below] at (%1, 0) {%1};\n").arg(i);
            }

            // mean and variance of 90%, 95% and 98% explored
            qreal mean, sigma;
            statsForPercentExplored(0.9, mean, sigma);
            tikzPicture << QString("\\draw[|-|] (%1, 90) -- (%2, 90);\n").arg(mean - sigma).arg(mean + sigma);
            tikzPicture << QString("\\node[draw, circle, fill=white, inner sep=0mm, minimum size=1mm] at (%1, 90) {};\n").arg(mean);
            tikzPicture.comment(QString("90: %1  +-  %2").arg(mean).arg(sigma));
            statsForPercentExplored(0.95, mean, sigma);
            tikzPicture << QString("\\draw[|-|] (%1, 95) -- (%2, 95);\n").arg(mean - sigma).arg(mean + sigma);
            tikzPicture << QString("\\node[draw, circle, fill=white, inner sep=0mm, minimum size=1mm] at (%1, 95) {};\n").arg(mean);
            tikzPicture.comment(QString("95: %1  +-  %2").arg(mean).arg(sigma));
            statsForPercentExplored(0.98, mean, sigma);
            tikzPicture << QString("\\draw[|-|] (%1, 98) -- (%2, 98);\n").arg(mean - sigma).arg(mean + sigma);
            tikzPicture << QString("\\node[draw, circle, fill=white, inner sep=0mm, minimum size=1mm] at (%1, 98) {};\n").arg(mean);
            tikzPicture.comment(QString("98: %1  +-  %2").arg(mean).arg(sigma));
            statsForPercentExplored(1.0, mean, sigma);
            tikzPicture << QString("\\draw[|-|] (%1, 100) -- (%2, 100);\n").arg(mean - sigma).arg(mean + sigma);
            tikzPicture << QString("\\node[draw, circle, fill=white, inner sep=0mm, minimum size=1mm] at (%1, 100) {};\n").arg(mean);
            tikzPicture.comment(QString("100: %1  +-  %2").arg(mean).arg(sigma));

        tikzPicture.endScope();

        // axes lables
        tikzPicture.newline(2);
        tikzPicture.comment("axes lables");
        tikzPicture.beginScope("yscale=0.05");

            // y axis lables
            tikzPicture.line(QPointF(0, 0), QPointF(0, 100));
            tikzPicture << "\\node[left] at (0, 20) {20};\n";
            tikzPicture << "\\node[left] at (0, 40) {40};\n";
            tikzPicture << "\\node[left] at (0, 60) {60};\n";
            tikzPicture << "\\node[left] at (0, 80) {80};\n";
            tikzPicture << "\\node[left] at (0, 100) {100};\n";

            tikzPicture << "\\node[rotate=90] at (-0.8, 50) {exploration progress in \\%};\n";

            // x axis lables
            tikzPicture.line(QPointF(0, 0), QPointF(8.5, 0), "->, >=stealth'");
            tikzPicture << "\\node[below] at (8.5, 0) {it};\n";

        tikzPicture.endScope();

        // legend
        tikzPicture.newline(2);
        tikzPicture.comment("legend");
        tikzPicture.beginScope("xshift=6cm, yshift=2.5cm");

            tikzPicture << "\\draw[semithick, fill=white, fill opacity=0.8] (0, -0.6) rectangle +(2.5, 2.6);\n";
            tikzPicture << "\\scriptsize\n";

            tikzPicture << "\\draw[semithick,|-|] (0.2, -.4) -- +(0.28, 0) node[right, black] {$\\text{mean} \\pm \\sqrt{\\text{var}}$};\n";
            tikzPicture << "\\node[semithick,draw, circle, fill=white, inner sep=0mm, minimum size=1mm] at (0.34, -0.4) {};\n";
            tikzPicture << "\\draw (0.2, -.1) -- +(0.28, 0) node[right, black] {time-opt. case};\n";
            tikzPicture << "\\draw[magenta, densely dotted] (0.2, 0.2) -- +(0.28, 0) node[right, black] {unemployed};\n";

            tikzPicture << "\\fill[orange!50] (0.2, 0.5) rectangle +(0.28, 1.2);\n";
            tikzPicture << "\\fill[green!20] (0.2, 0.8) rectangle +(0.28, 0.6);\n";

            tikzPicture << "\\draw[orange] (0.2, 1.7) -- +(0.28, 0) node[right, black] {best case};\n";
            tikzPicture << "\\draw[gray, densely dashed] (0.2, 1.4) -- +(0.28, 0) node[right, black] {median $+25\\%$};\n";
            tikzPicture << "\\draw[blue] (0.2, 1.1) -- +(0.28, 0) node[right, black] {median};\n";
            tikzPicture << "\\draw[gray, densely dashed] (0.2, 0.8) -- +(0.28, 0) node[right, black] {median $-25\\%$};\n";
            tikzPicture << "\\draw[orange] (0.2, 0.5) -- +(0.28, 0) node[right, black] {worst case};\n";

        tikzPicture.endScope();

        tikzPicture.end();

        file.close();
    }
}
// kate: replace-tabs on; indent-width 4;
