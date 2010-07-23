#ifndef SCENE_H
#define SCENE_H

#include <QtGui/QFrame>
#include <QtGui/QPixmap>

#include "cell.h"
#include "gridmap.h"
#include "toolhandler.h"

class QPaintEvent;
class QMouseEvent;
class MainWindow;
class QEvent;
class QWheelEvent;
class QSettings;

class Scene : public QFrame
{
    Q_OBJECT

    public:
        Scene(MainWindow* mainWindow, QWidget* parent = 0);
        virtual ~Scene();
        
        GridMap& map();
        MainWindow* mainWindow() const;
        
        void load(QSettings& config);
        void save(QSettings& config);
        
        void draw(QPaintDevice* paintDevice);

    public slots:
        void newScene();
        void zoomIn();
        void zoomOut();
        
        void selectTool(int toolIndex);
        void setOperationRadius(double radius);

    public:
        virtual QSize sizeHint() const;

    protected:
        virtual void paintEvent(QPaintEvent* event);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void wheelEvent(QWheelEvent* event);

        void drawMap(QPainter& p);
        
    private:
        GridMap m_map;
        
        MainWindow* m_mainWindow;
        
        ToolHandler* m_toolHandler;
        RobotHandler m_robotHandler;
        ObstacleHandler m_obstacleHandler;
        ExplorationHandler m_explorationHandler;
};

#endif // SCENE_H

// kate: replace-tabs on; indent-width 4;
