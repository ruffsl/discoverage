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

#ifndef DISCOVERAGE_ROBOT_CONFIG_WIDGET_H
#define DISCOVERAGE_ROBOT_CONFIG_WIDGET_H

#include <QtGui/QWidget>

class Robot;

class RobotConfigWidget : public QWidget
{
    Q_OBJECT

    public:
        RobotConfigWidget(Robot* robot, const QString& robotName);
        virtual ~RobotConfigWidget();

        Robot* robot() const;

        void setConfigWidget(QWidget* widget);

    public slots:
        void removeRobot();
        void setRobotActive();

    signals:
        void removeRobot(Robot* robot);

    protected:
        virtual void mousePressEvent(QMouseEvent * event);

    private:
        Robot *m_robot;
};

#endif // DISCOVERAGE_ROBOT_CONFIG_WIDGET_H

// kate: replace-tabs on; indent-width 4;