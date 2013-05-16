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

#ifndef STATISTICS_H
#define STATISTICS_H

#include <QtGui/QFrame>

class QSpinBox;
class QPushButton;
class QPaintEvent;
class MainWindow;
class QContextMenuEvent;

class Stats
{
    public:
        Stats();

        int iteration;
        qreal percentExplored; // [0; 1]
        qreal percentUnemployed; // [0; 1]
};

class TestRun
{
    public:
        TestRun();
        int testRun;
        QVector<Stats> stats;
};

class Statistics : public QFrame
{
    Q_OBJECT

    public:
        Statistics(MainWindow* mainWindow, QWidget* parent = 0);
        virtual ~Statistics();

        MainWindow* mainWindow() const;
        
        int iteration() const;

    public slots:
        void reset();
        void tick();

    public:
        virtual QSize sizeHint() const;

    protected:
        virtual void paintEvent(QPaintEvent* event);
        virtual void contextMenuEvent(QContextMenuEvent* event);

    //
    // batch statistics
    //
    public:
        qreal meanProgress(int iteration);
        qreal varianceProgress(int iteration);
    protected Q_SLOTS:
        void startStopBatchProcess();
    private:
        bool m_batchProcessRunning;
        QPushButton * m_btnStartStop;
        QSpinBox* m_sbRuns;

    private:
        MainWindow* m_mainWindow;
        
        QVector<double> m_progress;

        QVector<TestRun> m_testRuns;
        QVector<qreal> m_meanProgress;
};

#endif // STATISTICS_H

// kate: replace-tabs on; indent-width 4;
