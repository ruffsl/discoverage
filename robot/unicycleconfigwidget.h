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

#ifndef DISCOVERAGE_UNICYCLE_DYNAMICS_CONFIG_WIDGET_H
#define DISCOVERAGE_UNICYCLE_DYNAMICS_CONFIG_WIDGET_H

#include "robotconfigwidget.h"

class Unicycle;

namespace Ui {
    class UnicycleConfigWidget;
}

class UnicycleConfigWidget : public RobotConfigWidget
{
    Q_OBJECT

    public:
        UnicycleConfigWidget(Unicycle* robot);
        virtual ~UnicycleConfigWidget();

    public slots:
        void setOrientation(double value);
        void setDialOrientation(int value);

        void setSensingRange(double range);
        void setFillSensingRange(bool fill);

    private:
        Ui::UnicycleConfigWidget *m_ui;
};

#endif // DISCOVERAGE_UNICYCLE_DYNAMICS_CONFIG_WIDGET_H

// kate: replace-tabs on; indent-width 4;
