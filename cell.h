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

#ifndef CELL_H
#define CELL_H

#include <QRectF>

#include <QDebug>

class QDataStream;
class QPainter;
class QTextStream;
class Robot;

class Cell
{
    friend class GridMap;

    // 0 0 0 | x x x | x x
    public:
        enum State {
            Obstacle = 1 << 0, // 1
            Free     = 1 << 1, // 2
            // --------------------
            Unknown  = 1 << 2, // 4
            Frontier = 1 << 3, // 8
            Explored = 1 << 4  // 16
        };

    public:
        Cell();
        Cell(const QRectF& rect);

        const QPoint& index() const;

        void setRect(const QRectF& rect);

        void draw(QPainter& p, bool showDensity, bool showGradient);

        const QRectF& rect() const;
        const QPointF center() const;

        inline bool isObstacle() const
        { return m_state & Obstacle; }

        inline State state() const
        { return m_state; }

        // get density
        inline float density() const
        { return m_density; }

        // set density
        inline void setDensity(float density)
        { m_density = density; }

        // get gradient
        inline QPointF gradient()  const
        { return m_gradient; }

        // set gradient
        inline void setGradient(const QPointF& gradient)
        { m_gradient = gradient; }

        inline float frontierDist() const
        { return m_frontierDist; }

        inline void setFrontierDist(float dist)
        { m_frontierDist = dist; }

        inline Robot* robot() const
        { return m_robot; }

        inline void setRobot(Robot* robot)
        { m_robot = robot; }

        inline float robotDist() const
        { return m_robotDist; }

        inline void setRobotDist(float dist)
        { m_robotDist = dist; }

    //
    // load & save + export
    //
    public:
        QDataStream& load(QDataStream& ds);
        QDataStream& save(QDataStream& ds);

        void exportToTikz(QTextStream& ts, bool fillDensity, bool exportGradient);

    //
    // path planning
    //
    public:
        enum PathState {
            PathNone = 0,
            PathOpen,
            PathClose
        };
        inline PathState pathState() const { return m_pathState; }
        inline void setPathState(PathState state) { m_pathState = state; }
        inline float cellCost() const {
            static int s_pathCost[] = {
                0,  //  0: 0 0 0 | 0 0 ---
                0,  //  1: 0 0 0 | 0 1 ---
                0,  //  2: 0 0 0 | 1 0 ---
                0,  //  3: 0 0 0 | 1 1 ---
                0,  //  4: 0 0 1 | 0 0 ---
                1000,  //  5: 0 0 1 | 0 1 unknown | obstacle
                100,  //  6: 0 0 1 | 1 0 unknown | free
                0,  //  7: 0 0 1 | 1 1 ---
                0,  //  8: 0 1 0 | 0 0 ---
                1,  //  9: 0 1 0 | 0 1 frontier | obstacle
                1,  // 10: 0 1 0 | 1 0 frontier | free
                0,  // 11: 0 1 0 | 1 1 ---
                0,  // 12: 0 1 1 | 0 0 ---
                0,  // 13: 0 1 1 | 0 1 ---
                0,  // 14: 0 1 1 | 1 0 ---
                0,  // 15: 0 1 1 | 1 1 ---
                0,  // 16: 1 0 0 | 0 0 ---
                10000,  // 17: 1 0 0 | 0 1 explored | obstacle
                2,  // 18: 1 0 0 | 1 0 explored | free
                0,  // 19: 1 0 0 | 1 1 ---
                0,  // 20: 1 0 1 | 0 0 ---
                0,  // 21: 1 0 1 | 0 1 ---
                0,  // 22: 1 0 1 | 1 0 ---
                0,  // 23: 1 0 1 | 1 1 ---
                0,  // 24: 1 1 0 | 0 0 ---
                0,  // 25: 1 1 0 | 0 1 ---
                0,  // 26: 1 1 0 | 1 0 ---
                0,  // 27: 1 1 0 | 1 1 ---
                0,  // 28: 1 1 1 | 0 0 ---
                0,  // 29: 1 1 1 | 0 1 ---
                0,  // 30: 1 1 1 | 1 0 ---
                0   // 31: 1 1 1 | 1 1 ---
            };
//             qDebug() << m_state << s_pathCost[qint8(m_state)];
            return s_pathCost[qint8(m_state)];
        }

    private:
        PathState m_pathState;
        int m_pathParent;

    private:
        void setState(State newState);
        void setIndex(const QPoint& index);

    private:
        QRectF m_rect;
        State m_state;
        QPoint m_index;
        QPointF m_gradient;
        Robot* m_robot;

    public:
        float m_costF;
        float m_costG;
        float m_density;
        float m_frontierDist;
        float m_robotDist;
};

inline Cell::State operator|(Cell::State state, int value)
{
    return static_cast<Cell::State>(int(state) | value);
}

inline Cell::State operator&(Cell::State state, int value)
{
    return static_cast<Cell::State>(int(state) & value);
}

#endif // CELL_H

// kate: replace-tabs on; indent-width 4;
