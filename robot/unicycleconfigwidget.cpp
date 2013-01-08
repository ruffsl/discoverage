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

#include "unicycleconfigwidget.h"
#include "unicycle.h"
#include "robotmanager.h"
#include "scene.h"
#include "ui_unicycleconfigwidget.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QLabel>

#include <math.h>

UnicycleConfigWidget::UnicycleConfigWidget(Unicycle* robot)
    : RobotConfigWidget(robot, "Unicycle")
{
    m_ui = new Ui::UnicycleConfigWidget();
    QWidget* top = new QWidget();
    m_ui->setupUi(top);
    setConfigWidget(top);

    m_ui->sbSensingRange->setValue(robot->sensingRange());
    m_ui->chkFillSensingRange->setChecked(robot->fillSensingRange());

    double orientation = robot->orientation() * 180 / M_PI;
    if (orientation < 0) orientation += 360;
    m_ui->sbOrientation->setValue(orientation);
    m_ui->dialOrientation->setValue(orientation - 90);

    connect(m_ui->sbOrientation, SIGNAL(valueChanged(double)), this, SLOT(setOrientation(double)));
    connect(m_ui->dialOrientation, SIGNAL(valueChanged(int)), this, SLOT(setDialOrientation(int)));
    connect(m_ui->sbSensingRange, SIGNAL(valueChanged(double)), this, SLOT(setSensingRange(double)));
    connect(m_ui->chkFillSensingRange, SIGNAL(toggled(bool)), this, SLOT(setFillSensingRange(bool)));

    connect(m_ui->sbSensingRange, SIGNAL(valueChanged(double)), this, SLOT(setRobotActive()));
    connect(m_ui->chkFillSensingRange, SIGNAL(toggled(bool)), this, SLOT(setRobotActive()));
}

UnicycleConfigWidget::~UnicycleConfigWidget()
{
    delete m_ui;
}

void UnicycleConfigWidget::setOrientationFromRobot(double value)
{
    value = value * 180 / M_PI;
    if (value < 0) value += 360;
    
    m_ui->sbOrientation->blockSignals(true);
    m_ui->sbOrientation->setValue(360 - value);
    m_ui->sbOrientation->blockSignals(false);

    value -= 90;
    if (value < 0) value += 360;
    m_ui->dialOrientation->blockSignals(true);
    m_ui->dialOrientation->setValue(value);
    m_ui->dialOrientation->blockSignals(false);
}

void UnicycleConfigWidget::setOrientation(double value)
{
    Unicycle* r = static_cast<Unicycle*>(robot());
    r->setOrientation(2*M_PI - value * M_PI / 180);

    m_ui->dialOrientation->blockSignals(true);
    m_ui->dialOrientation->setValue(360 - (value - 270));
    m_ui->dialOrientation->blockSignals(false);

    Scene::self()->update();
}

void UnicycleConfigWidget::setDialOrientation(int value)
{
    value = value + 90;
    if (value > 360) value -= 360;
    Unicycle* r = static_cast<Unicycle*>(robot());
    r->setOrientation(value * M_PI / 180.0);

    m_ui->sbOrientation->blockSignals(true);
    m_ui->sbOrientation->setValue(360 - value);
    m_ui->sbOrientation->blockSignals(false);

    Scene::self()->update();
}

void UnicycleConfigWidget::setSensingRange(double range)
{
    Unicycle* r = static_cast<Unicycle*>(robot());
    r->setSensingRange(range);

    Scene::self()->update();
}

void UnicycleConfigWidget::setFillSensingRange(bool fill)
{
    Unicycle* r = static_cast<Unicycle*>(robot());
    r->setFillSensingRange(fill);

    Scene::self()->update();
}

// kate: replace-tabs on; indent-width 4;
