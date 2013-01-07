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

#include "integratordynamicsconfigwidget.h"
#include "integratordynamics.h"
#include "robotmanager.h"
#include "scene.h"
#include "ui_integratordynamicsconfigwidget.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QLabel>

IntegratorDynamicsConfigWidget::IntegratorDynamicsConfigWidget(IntegratorDynamics* robot)
    : RobotConfigWidget(robot, "Integrator Dynamics")
{
    m_ui = new Ui::IntegratorDynamicsConfigWidget();
    QWidget* top = new QWidget();
    m_ui->setupUi(top);
    setConfigWidget(top);
}

IntegratorDynamicsConfigWidget::~IntegratorDynamicsConfigWidget()
{
    delete m_ui;
}

void IntegratorDynamicsConfigWidget::setSensingRange(double range)
{
    IntegratorDynamics* r = static_cast<IntegratorDynamics*>(robot());
    r->setSensingRange(range);

    Scene::self()->update();
}

void IntegratorDynamicsConfigWidget::setFillSensingRange(bool fill)
{
    IntegratorDynamics* r = static_cast<IntegratorDynamics*>(robot());
    r->setFillSensingRange(fill);

    Scene::self()->update();
}

// kate: replace-tabs on; indent-width 4;
