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
class QLabel;

/**
 * Base class for Robot config widgets.
 */
class RobotConfigWidget : public QWidget
{
    Q_OBJECT

    public:
        /**
         * Constructor.
         * The robot is the robot that this config widget configures.
         * The robot name is displayed in the header of the config widget.
         */
        RobotConfigWidget(Robot* robot, const QString& robotName);
        virtual ~RobotConfigWidget();

        /**
         * the associated robot this config widget belongs to.
         */
        Robot* robot() const;

        /**
         * Hook your gui into a widget and set it with this function as
         * main config widget.
         */
        void setConfigWidget(QWidget* widget);

        /**
         * @internal called by RobotListView.
         */
        void updatePixmap();

        virtual QPixmap pixmap() = 0;

    protected slots:
        // mark robot() as active robot.
        void setRobotActive();

    private slots:
        void removeRobot();

    signals:
        // emitted when the "remove" button is clicked. Connected to the RobotManager.
        void removeRobot(Robot* robot);

    protected:
        // reimplemented to mark robot as active
        virtual void mousePressEvent(QMouseEvent * event);

    private:
        Robot *m_robot;
        QLabel* m_lblPixmap;
};

#endif // DISCOVERAGE_ROBOT_CONFIG_WIDGET_H

// kate: replace-tabs on; indent-width 4;
