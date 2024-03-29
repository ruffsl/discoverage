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

#include "robotconfigwidget.h"
#include "robot.h"
#include "robotmanager.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QLabel>

RobotConfigWidget::RobotConfigWidget(Robot* robot, const QString& robotName)
    : QWidget()
    , m_robot(robot)
{
    setAutoFillBackground(true);

    // vertical top layout
    QVBoxLayout* topLayout = new QVBoxLayout();
    setLayout(topLayout);

    // horizontal header layout
    QHBoxLayout* header = new QHBoxLayout();
    topLayout->addLayout(header);

    QToolButton* btnRemoveRobot = new QToolButton(this);
    btnRemoveRobot->setIcon(QIcon(":/icons/icons/remove.png"));
    btnRemoveRobot->setToolTip("Remove robot");

    m_lblPixmap= new QLabel(this);
    m_lblPixmap->setFixedSize(16, 16);

    QLabel* lblRobot = new QLabel(robotName, this);

    header->addWidget(m_lblPixmap);
    header->addWidget(lblRobot);
    header->addWidget(btnRemoveRobot);

    connect(btnRemoveRobot, SIGNAL(clicked()), this, SLOT(removeRobot()));
    connect(this, SIGNAL(removeRobot(Robot*)), RobotManager::self(), SLOT(removeRobot(Robot*)), Qt::QueuedConnection);
}

RobotConfigWidget::~RobotConfigWidget()
{
}

Robot* RobotConfigWidget::robot() const
{
    return m_robot;
}

void RobotConfigWidget::removeRobot()
{
    emit removeRobot(m_robot);
}

void RobotConfigWidget::mousePressEvent(QMouseEvent * event)
{
    QWidget::mousePressEvent(event);
    setRobotActive();
}

void RobotConfigWidget::setRobotActive()
{
    RobotManager::self()->setActiveRobot(m_robot);
}

void RobotConfigWidget::setConfigWidget(QWidget* widget)
{
    layout()->addWidget(widget);
    if (widget->layout()) {
        widget->layout()->setMargin(0);
    }
}

void RobotConfigWidget::updatePixmap()
{
    m_lblPixmap->setPixmap(pixmap());
}

// kate: replace-tabs on; indent-width 4;
