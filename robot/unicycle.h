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

#ifndef DISCOVERAGE_UNICYCLE_DYNAMICS_H
#define DISCOVERAGE_UNICYCLE_DYNAMICS_H

#include "robot.h"

#include <QtCore/QVector>
#include <QtCore/QPointer>

class UnicycleConfigWidget;

class Unicycle : public Robot
{
    public:
        Unicycle(Scene *scene);
        virtual ~Unicycle();

        virtual Dynamics type();

        virtual void tick();
        virtual void reset();

        virtual RobotConfigWidget* configWidget();

    //
    // Unicycle properties
    //
    public:
        void setOrientation(double radian);

        virtual bool hasOrientation() const;

        // orientation of last move in [-pi; pi]
        virtual qreal orientation() const;

        // orientation of last move as unit vector
        virtual QPointF orientationVector() const;


        void setSensingRange(qreal sensingRange);
        qreal sensingRange() const;

        void clearTrajectory();

        void setFillSensingRange(bool fill);
        bool fillSensingRange() const;

    //
    // environment information
    //
    public:
        virtual void draw(QPainter& p);
        void drawRobot(QPainter& p);

    private:
        void drawUnicycle(QPainter& p);
        void drawSensedArea(QPainter& p);

    //
    // load/save & export functions
    //
    public:
        virtual void load(QSettings& config);
        virtual void save(QSettings& config);

        virtual void exportToTikz(QTikzPicture& tp);

    private:
        QPointer<UnicycleConfigWidget> m_configWidget;

        double m_orientation;
        qreal m_sensingRange;
        QVector<QPointF> m_trajectory;

        bool m_fillSensingRange;
};

#endif // DISCOVERAGE_UNICYCLE_DYNAMICS_H

// kate: replace-tabs on; indent-width 4;
