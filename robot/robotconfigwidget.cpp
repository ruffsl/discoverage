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

    QLabel* lblPixmap = new QLabel(this);
    lblPixmap->setFixedSize(16, 16);
    lblPixmap->setPixmap(pixmap(robot->color()));

    QLabel* lblRobot = new QLabel(robotName, this);

    header->addWidget(lblPixmap);
    header->addWidget(lblRobot);
    header->addWidget(btnRemoveRobot);

    connect(btnRemoveRobot, SIGNAL(clicked()), this, SLOT(removeRobot()));
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
}

// kate: replace-tabs on; indent-width 4;
