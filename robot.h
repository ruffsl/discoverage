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
#include <QtCore/QVector>
#include <QtGui/QColor>

class Scene;
class GridMap;
class QTextStream;
class QPainter;
class QSettings;

class Robot
{
    public:
        Robot(Scene *scene);
        virtual ~Robot();

    void setPosition(const QPointF& position);
    const QPointF& position() const;

    void setSensingRange(qreal sensingRange);
    qreal sensingRange() const;

    void clearTrajectory();

    //
    // environment information
    //
    public:
        Scene* scene() const;
        GridMap* map() const;

        QColor color();
        virtual void draw(QPainter& p);

    private:
        void drawRobot(QPainter& p);

    //
    // load/save & export functions
    //
    public:
        void load(QSettings& config);
        void save(QSettings& config);

        void exportToTikz(QTextStream& ts);

    private:
        Scene* m_scene;

        QPointF m_position;
        qreal m_sensingRange;
        QVector<QPointF> m_trajectory;
};

#endif // DISCOVERAGE_ROBOT_H

// kate: replace-tabs on; indent-width 4;
