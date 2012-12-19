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

#include "robotlistview.h"
#include "robotmanager.h"
#include "robotwidget.h"
#include "robot.h"

#include <QtCore/QDebug>
// #include <QtGui/QResizeEvent>
#include <QtGui/QPushButton>

RobotListView::RobotListView(QWidget* parent)
    : QScrollArea(parent)
{
    setWidget(new QWidget(this));

    QVBoxLayout* l = new QVBoxLayout(widget());

    QPushButton* newRobot = new QPushButton("Add robot", widget());
    l->addWidget(newRobot);

    updateList();

    connect(newRobot, SIGNAL(clicked()), RobotManager::self(), SLOT(addRobot()));
    connect(RobotManager::self(), SIGNAL(robotCountChanged()), this, SLOT(updateList()), Qt::QueuedConnection);
}

RobotListView::~RobotListView()
{
}

void RobotListView::updateList()
{
    for (int i = 0; i < RobotManager::self()->count(); ++i) {
        if (i < m_robotItems.count()) {
            m_robotItems[i]->setRobot(RobotManager::self()->robot(i));
        } else {
            RobotWidget* rw = new RobotWidget(RobotManager::self()->robot(i), widget());
            m_robotItems.append(rw);
        }

        m_robotItems[i]->setBackgroundRole((i % 2) ? QPalette::AlternateBase : QPalette::Base);
    }

    int diff = m_robotItems.count() > RobotManager::self()->count();
    while (diff > 0) {
        delete m_robotItems.last();
        m_robotItems.pop_back();
        --diff;
    }
}

// void RobotListView::resizeEvent(QResizeEvent* event)
// {
//   QScrollArea::resizeEvent(event);
// 
//   // calculate sum of all editor heights
//   int listHeight = 0;
//   foreach (QWidget* w, m_robotItems) {
//     listHeight += w->sizeHint().height();
//   }
// 
//   // resize scroll area widget
//   widget()->resize(event->size().width(), listHeight);
// 
//   // set client geometries correctly
//   int h = 0;
//   foreach (QWidget* w, m_robotItems) {
//     w->setGeometry(0, h, widget()->width(), w->sizeHint().height());
//     h += w->sizeHint().height();
//   }
// }

// kate: replace-tabs on; indent-width 4;
