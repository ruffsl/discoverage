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

#ifndef DISCOVERAGE_ROBOT_H
#define DISCOVERAGE_ROBOT_H

#include <QPointF>
#include <QtGui/QColor>
#include <QPainterPath>
#include <QVector>
#include <QPixmap>

class Scene;
class GridMap;
class QTikzPicture;
class QPainter;
class QSettings;
class RobotConfigWidget;

/**
 * Base class for Robots.
 * Inherit this class and reimplement tick() with your own dynamics.
 */
class Robot
{
    public:
        enum Dynamics {
            IntegratorDynamics = 0,
            Unicycle
        };

    public:
        Robot(Scene *scene);
        virtual ~Robot();

        virtual Dynamics type() = 0;

        bool isActive() const;

        virtual void tick();
        virtual void reset();

        virtual RobotConfigWidget* configWidget() = 0;

    //
    // Robot properties
    //
    public:
        virtual void setPosition(const QPointF& position, bool trackTrajectory = false);
        virtual const QPointF& position() const;

        virtual bool hasOrientation() const;

        // orientation of last move in [-pi; pi]
        virtual qreal orientation() const;
        // orientation of last move as unit vector
        virtual QPointF orientationVector() const;

        // trajectory manipulation
        void clearTrajectory();
        const QVector<QPointF>& trajectory() const;

        void setSensingRange(double sensingRange);
        double sensingRange() const;

        void setFillSensingRange(bool fill);
        bool fillSensingRange() const;

    //
    // environment information
    //
    public:
        Scene* scene() const;
        GridMap* map() const;

        QColor color();
        virtual void draw(QPainter& p);
        virtual void drawTrajectory(QPainter& p);
        virtual void drawSensedArea(QPainter& p);
        virtual QPainterPath visibleArea(double radius);

    //
    // load/save & export functions
    //
    public:
        virtual void load(QSettings& config);
        virtual void save(QSettings& config);

        virtual void exportToTikz(QTikzPicture& tp);

    private:
        Scene* m_scene;
        QPointF m_position;
        QVector<QPointF> m_trajectory;

        double m_sensingRange;
        bool m_fillSensingRange;
};

#endif // DISCOVERAGE_ROBOT_H

// kate: replace-tabs on; indent-width 4;
