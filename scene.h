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

#ifndef SCENE_H
#define SCENE_H

#include <QtGui/QFrame>
#include <QtGui/QPixmap>

#include "cell.h"
#include "gridmap.h"
#include "toolhandler.h"
#include "discoveragehandler.h"
#include "mindisthandler.h"
#include "bullo.h"

class QPaintEvent;
class QMouseEvent;
class MainWindow;
class QEvent;
class QWheelEvent;
class QSettings;

class Scene : public QFrame
{
    Q_OBJECT

    static Scene* s_self;

    public:
        Scene(MainWindow* mainWindow, QWidget* parent = 0);
        virtual ~Scene();

        static Scene* self();

        GridMap& map();
        MainWindow* mainWindow() const;

        void load(QSettings& config);
        void save(QSettings& config);

        void draw(QPaintDevice* paintDevice);

        ToolHandler* toolHandler()
        { return m_toolHandler; }

    public slots:
        void newScene();
        void zoomIn();
        void zoomOut();

        void configChanged();

        void selectTool(int toolIndex);
        void setOperationRadius(double radius);

        void reset();
        void tick();

    public:
        virtual QSize sizeHint() const;

    protected:
        virtual void paintEvent(QPaintEvent* event);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseReleaseEvent(QMouseEvent* event);
        virtual void wheelEvent(QWheelEvent* event);

        void drawMap(QPainter& p);

    private:
        GridMap m_map;

        MainWindow* m_mainWindow;

        ToolHandler* m_toolHandler;
        RobotHandler m_robotHandler;
        ObstacleHandler m_obstacleHandler;
        ExplorationHandler m_explorationHandler;
        DisCoverageHandler m_discoverageHandler;
        MinDistHandler m_minDistHandler;
        DisCoverageBulloHandler m_bulloHandler;
};

#endif // SCENE_H

// kate: replace-tabs on; indent-width 4;
