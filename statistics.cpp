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

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QClipboard>

Statistics::Statistics(MainWindow* mainWindow, QWidget* parent)
    : QFrame(parent)
    , m_mainWindow(mainWindow)
{
    setFrameStyle(Panel | Sunken);
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
    p.scale(1.0, -1.0);
    p.translate(10, -height() + 10);
    p.scale(2.0, (height() - 20) / 100.0);

    p.drawLine(0, 0, 0, 100);
    p.drawLine(0, 0, 100, 0);
    
    for (int i = 1; i < m_progress.size(); ++i) {
        p.drawLine(QPointF((i - 1)/4.0, m_progress[i-1]),
                   QPointF( i     /4.0, m_progress[i]));
    }

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
}

void Statistics::tick()
{
    GridMap& m = mainWindow()->scene()->map();

    m_progress.append(m.explorationProgress() * 100);
    update();
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

// kate: replace-tabs on; indent-width 4;
