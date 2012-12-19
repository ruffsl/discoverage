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

#include "robotwidget.h"
#include "robot.h"
#include "robotmanager.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>

RobotWidget::RobotWidget(Robot* robot, QWidget* parent)
    : QWidget(parent)
    , Ui::RobotWidget()
    , m_robot(robot)
{
    setAutoFillBackground(true);
    setupUi(this);
    btnRemoveRobot->setIcon(QIcon(":/icons/icons/remove.png"));

    setRobot(m_robot);

    connect(btnRemoveRobot, SIGNAL(clicked()), this, SLOT(removeRobot()), Qt::QueuedConnection);
    connect(sbSensingRange, SIGNAL(valueChanged(double)), this, SLOT(setSensingRange(double)));

    connect(sbSensingRange, SIGNAL(valueChanged(double)), this, SLOT(activateRobot()));
}

RobotWidget::~RobotWidget()
{
}

Robot* RobotWidget::robot() const
{
    return m_robot;
}

static QPixmap pixmap(const QColor& color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(color);
    QPainter p(&pixmap);
    QPen blackPen(Qt::black, 1);
    p.setPen(blackPen);
    p.drawRect(0, 0, 15, 15);
    p.end();

    return pixmap;
}

void RobotWidget::setRobot(Robot* robot)
{
    m_robot = robot;

    lblPixmap->setPixmap(pixmap(robot->color()));
    sbSensingRange->setValue(robot->sensingRange());
}

void RobotWidget::setSensingRange(double range)
{
    m_robot->setSensingRange(range);
}

void RobotWidget::removeRobot()
{
    RobotManager::self()->removeRobot(m_robot);
}

void RobotWidget::mousePressEvent(QMouseEvent * event)
{
    activateRobot();
}

void RobotWidget::activateRobot()
{
    RobotManager::self()->setActiveRobot(m_robot);
}
// kate: replace-tabs on; indent-width 4;
