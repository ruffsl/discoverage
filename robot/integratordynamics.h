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

#ifndef DISCOVERAGE_INTEGRATOR_DYNAMICS_H
#define DISCOVERAGE_INTEGRATOR_DYNAMICS_H

#include "robot.h"

#include <QtCore/QVector>
#include <QtCore/QPointer>

class IntegratorDynamics : public Robot
{
    public:
        IntegratorDynamics(Scene *scene);
        virtual ~IntegratorDynamics();

        virtual Dynamics type();

        virtual void tick();
        virtual void reset();

        virtual RobotConfigWidget* configWidget();

    //
    // IntegratorDynamics properties
    //
    public:
        virtual bool hasOrientation() const;

        // orientation of last move in [-pi; pi]
        virtual qreal orientation() const;

        // orientation of last move as unit vector
        virtual QPointF orientationVector() const;

    //
    // environment information
    //
    public:
        virtual void draw(QPainter& p);
        void drawRobot(QPainter& p);

    private:
        void drawIntegratorDynamics(QPainter& p);

    //
    // load/save & export functions
    //
    public:
        virtual void load(QSettings& config);
        virtual void save(QSettings& config);

        virtual void exportToTikz(QTikzPicture& tp);

    private:
        QPointer<RobotConfigWidget> m_configWidget;
};

#endif // DISCOVERAGE_INTEGRATOR_DYNAMICS_H

// kate: replace-tabs on; indent-width 4;
