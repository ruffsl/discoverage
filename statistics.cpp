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
    p.drawText(QPoint(200, 20), QString("Run: %1").arg(m_testRuns.size()));

    // prepare for progress line
    p.save();
    p.scale(1.0, -1.0);
    p.translate(220, -height() + 10);
    p.scale(2.0, (height() - 20) / 100.0);

    p.drawLine(0, 0, 0, 100);
    p.drawLine(0, 0, 300, 0);

    m_meanProgress.clear();
    for (int i = 0; i < 300; ++i) {
        p.setPen(Qt::red);
        qreal mp = meanProgress(i) * 100.0;
        p.drawPoint(i, mp);
        p.setPen(Qt::darkGreen);
        p.drawPoint(i, mp - sqrt(varianceProgress(i)) * 100.0);
    }
    p.restore();

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
//     qDebug () << m_meanProgress.size() << iter ation;
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

void Statistics::startStopBatchProcess()
{
    if (!m_batchProcessRunning) {
        m_batchProcessRunning = true;
        m_btnStartStop->setText("Stop");

        // prepare
        m_testRuns.clear();

        int run = 0;
        while (m_batchProcessRunning && run < m_sbRuns->value()) {
            m_mainWindow->reloadScene();

            while (m_batchProcessRunning && m_mainWindow->scene()->map().explorationProgress() < 1.0) {
                m_mainWindow->tick();
                QApplication::processEvents();
            }
            ++run;
        }
    }

    if (m_batchProcessRunning) {
        m_batchProcessRunning = false;
        m_btnStartStop->setText("Start");
    }
}

// kate: replace-tabs on; indent-width 4;
