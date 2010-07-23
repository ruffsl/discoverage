#ifndef TOOL_HANDLER_H
#define TOOL_HANDLER_H

#include <QtCore/QPoint>
#include "cell.h"
#include "gridmap.h"

class QMouseEvent;
class QPainter;
class Scene;

class ToolHandler
{
    public:
        ToolHandler(Scene* scene);
        virtual ~ToolHandler();

        Scene* scene() const;
        QPoint cellForMousePosition(const QPoint& mousePosition) const;

        QPoint currentCell() const;
        QPoint mousePosition() const;

        void setOperationRadius(double radius);
        qreal operationRadius() const;

        int mapToCell(qreal screenPos) const;
        qreal mapToMap(qreal screenPos) const;
        qreal mapToScreen(qreal mapPos) const;

        void drawOperationRadius(QPainter& p);
        void highlightCurrentCell(QPainter& p);

    public:
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event) = 0;

    private:
        void setCurrentCell(const QPoint& cell);

    private:
        Scene* m_scene;
        static QPoint s_currentCell;
        static QPoint s_mousePosition;
        static qreal s_operationRadius;
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
        
        void setDesiredState(Cell::State desiredState);

    public:
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);

    private:
        void updateObstacles();
        
        Cell::State m_desiredState;
};


class ExplorationHandler : public ToolHandler
{
    public:
        ExplorationHandler(Scene* scene);
        virtual ~ExplorationHandler();

        void setDesiredState(Cell::State desiredState);

    public:
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);

    private:
        void updateExploredState();
        Cell::State m_desiredState;
};


#endif // TOOL_HANDLER_H

// kate: replace-tabs on; indent-width 4;
