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

class ToolHandler
{
    public:
        ToolHandler(Scene* scene);
        virtual ~ToolHandler();

        Scene* scene() const;
        QPoint cellForMousePosition(const QPoint& mousePosition) const;

        QPoint currentCell() const;
        QPoint mousePosition() const;

        static void setOperationRadius(double radius);
        static qreal operationRadius();

        int mapToCell(qreal screenPos) const;
        qreal mapToMap(qreal screenPos) const;
        qreal mapToScreen(qreal mapPos) const;

        void drawOperationRadius(QPainter& p);
        void highlightCurrentCell(QPainter& p);

    public:
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event) = 0;
        virtual void toolHandlerActive(bool activated);
        virtual void tick();
        virtual void save(QSettings& config);
        virtual void load(QSettings& config);

    private:
        void setCurrentCell(const QPoint& cell);

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
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);

    private:
        void updateDisCoverage();
        float disCoverage(const QPointF& pos, float delta, const QPointF& q, const Path& path);

    private:
        QList<Path> m_allPaths;
        double m_delta;
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
