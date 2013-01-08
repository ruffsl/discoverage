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

#ifndef TOOL_HANDLER_H
#define TOOL_HANDLER_H

#include <QtCore/QPoint>
#include "cell.h"
#include "gridmap.h"

class QMouseEvent;
class QPainter;
class Scene;
class QSettings;
class QTextStream;
class QKeyEvent;
class Robot;

class ToolHandler
{
    public:
        static void setCurrentCell(const QPoint& cellIndex);
        static void updateCurrentCell(const QPoint& mousePos);
        static QPoint cellForMousePosition(const QPoint& mousePosition);

        static void setOperationRadius(double radius);
        static qreal operationRadius();

    public:
        ToolHandler(Scene* scene);
        virtual ~ToolHandler();

        Scene* scene() const;

        QPoint currentCell() const;
        QPoint mousePosition() const;

        void drawOperationRadius(QPainter& p);
        void highlightCurrentCell(QPainter& p);

    //
    // event handling
    //
    public:
        /**
         * Overwrite this function to draw an overlay over the map.
         */
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseReleaseEvent(QMouseEvent* event);
        virtual void keyPressEvent(QKeyEvent* event);
        virtual void toolHandlerActive(bool activated);

    //
    // strategy specifics
    //
    public:
        /**
         * This function is called when the "Reset" action is triggered.
         * Usually, cleanup routines are called here, such as cleaning trajectories.
         *
         * The default implementation does nothing.
         */
        virtual void reset();

        /**
         * This function is called each iteration and is basically one step
         * of a discrete-time implementation of the algorithm.
         *
         * To move the robots, use the Robot::setPosition() and Robot::position().
         *
         * The default implementation does nothing.
         *
         * @see RobotManager, Robot::position(), Robot::setPosition()
         */
        virtual void tick();

        virtual void postProcess();

        /**
         * TODO
         */
        virtual QPointF gradient(Robot* robot, bool interpolate);

    //
    // load & save, and export
    //
    public:
        virtual void save(QSettings& config);
        virtual void load(QSettings& config);

        virtual void exportToTikz(QTikzPicture& tp);

    private:
        Scene* m_scene;
        static QPoint s_currentCell;
        static QPoint s_mousePosition;
        static double s_operationRadius;
};

class RobotHandler : public ToolHandler
{
    public:
        RobotHandler(Scene* scene);
        virtual ~RobotHandler();

    public:
        virtual void draw(QPainter& p);

    private:
        QList<Path> m_allPaths;
};

class ObstacleHandler : public ToolHandler
{
    public:
        ObstacleHandler(Scene* scene);
        virtual ~ObstacleHandler();

    public:
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);

    private:
        void updateObstacles();
};


class ExplorationHandler : public ToolHandler
{
    public:
        ExplorationHandler(Scene* scene);
        virtual ~ExplorationHandler();

    public:
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);

    private:
        void updateExploredState();
};


#endif // TOOL_HANDLER_H

// kate: replace-tabs on; indent-width 4;
