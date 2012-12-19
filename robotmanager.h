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

#ifndef DISCOVERAGE_ROBOT_MANAGER_H
#define DISCOVERAGE_ROBOT_MANAGER_H

#include <QPointF>
#include <QtCore/QVector>

class QSettings;
class QPainter;
class Robot;
class QTextStream;

class RobotManager
{
    public:
        RobotManager();
        ~RobotManager();

    //
    // manager functios
    //
    public:
        void addRobot();
        void removeRobot();

        void draw(QPainter& p);

    //
    // convenience functions for all robots
    //
    public:
        void setSensingRange(qreal sensingRange);
        void clearTrajectory();

    //
    // load/save & export functions
    //
    public:
        void load(QSettings& config);
        void save(QSettings& config);

        void exportToTikz(QTextStream& ts);

    private:
        QVector<Robot*> m_robots;
};

#endif // DISCOVERAGE_ROBOT_MANAGER_H

// kate: replace-tabs on; indent-width 4;
