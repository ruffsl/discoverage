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
#include <QtCore/QObject>

class QSettings;
class QPainter;
class Robot;
class QTikzPicture;

class RobotManager : public QObject
{
    Q_OBJECT

        static RobotManager* s_self;

    public:
        RobotManager(QObject* parent);
        virtual ~RobotManager();

        static RobotManager* self();

        void draw(QPainter& p);

    //
    // manage robots
    //
    public slots:
        void addRobot();
        void removeRobot();
        bool removeRobot(Robot* robot);
        void setActiveRobot(Robot* robot);
        Robot* activeRobot();

        int count() const;
        int indexOf(Robot* robot) const;
        Robot* robot(int index) const;

    signals:
        // whenever a robot is added or deleted, changed() is emitted.
        void robotCountChanged();

        // emitted, whenever the active robot changes
        void activeRobotChanged(Robot* robot);

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

        void exportToTikz(QTikzPicture& tp);

    private:
        QVector<Robot*> m_robots;
        Robot* m_activeRobot;
};

#endif // DISCOVERAGE_ROBOT_MANAGER_H

// kate: replace-tabs on; indent-width 4;
