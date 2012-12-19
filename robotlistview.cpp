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
#include "robot.h"

#include <QtCore/QDebug>

RobotListView::RobotListView(Robot* robot, QWidget* parent)
    : QWidget(parent)
    , Ui::RobotListView()
    , m_robot(robot)
{
    setupUi(this);
    
    connect(btnRemoveRobot, SIGNAL(clicked()), this, SLOT(removeRobot()));
}

RobotListView::~RobotListView()
{
}

void RobotListView::addRobot()
{
}

void RobotListView::removeRobot()
{
    RobotManager::self()->removeRobot(m_robot);

    emit removed(this);
}

// kate: replace-tabs on; indent-width 4;
