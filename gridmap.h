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

#ifndef GRIDMAP_H
#define GRIDMAP_H

#include "cell.h"

#include <QtGui/QPixmap>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSet>
#include <QtCore/QSize>
#include <QtCore/QMap>

class QSettings;
class GridMap;
class Scene;
class QTikzPicture;

class Path
{
    public:
        Path() : m_cost(0.0f), m_length(0.0f) { }
        QList<QPoint> m_path;
        float m_cost;
        float m_length;

        void beautify(GridMap& gridMap, bool computeExactLength = true);
};

class GridMap : public QObject
{
    Q_OBJECT

    public:
        GridMap(Scene* scene, double width, double height, double resolution);
        virtual ~GridMap();

    //
    // load from / save to given config object, and export
    //
    public:
        void load(QSettings& config);
        void save(QSettings& config);

        void exportToTikz(QTikzPicture& tp);
        void exportToTikzOpt(QTikzPicture& tp);

        void exportLegend(QTikzPicture& tp);

    //
    // drawing
    //
    public slots:
        void updateCache();
        void updateCell(int xIndex, int yIndex);
        void updateCell(Cell& cell);
        void draw(QPainter& p);

    //
    // map properties
    //
    public:
        QSize displaySize() const;      // returns desired widget size in pixel, equals pixmap-cache size
        qreal scaleFactor() const;      // zoom factor for visualization
        void incScaleFactor();          // increase zoom factor
        void decScaleFactor();          // decrease zoom factor

        inline qreal resolution() const;// grid resolution. 0.2 means 0.2m x 0.2m
        QSize size() const;             // amount of cells in the grid map in
        QSizeF worldSize() const;       // returns the size in world/robot coordinates
        QPointF center() const;         // returns the center of the map in world coordinates
        double convexDiameter() const;  // returns the diameter (=diagonal of rectangular region)

        inline int screenToIndex(qreal screenPos) const {
            return screenPos / (scaleFactor() * resolution());
        }

        inline QPoint screenToIndex(const QPointF& screenPos) const {
            QPointF pt(screenPos / (scaleFactor() * resolution()));
            return QPoint(pt.x(), pt.y());
        }

        inline qreal screenToWorld(qreal screenPos) const {
            return screenPos / scaleFactor();
        }

        inline QPointF screenToWorld(const QPointF& screenPos) const {
            return screenPos / scaleFactor();
        }

        inline int worldToIndex(qreal mapPos) const {
            return mapPos / resolution();
        }

        inline QPoint worldToIndex(const QPointF& mapPos) const {
            return QPoint(mapPos.x() / resolution(), mapPos.y() / resolution());
        }


        inline qreal worldToScreen(qreal mapPos) const {
            return mapPos * scaleFactor();
        }

        inline QPointF worldToScreen(const QPointF& mapPos) const {
            return mapPos * scaleFactor();
        }

    //
    // cell accessors
    //
    public:
        Cell& cell(int xIndex, int yIndex);                     // cell accessor
        Cell& cell(const QPoint & cellIndex);                   // cell accessor
        inline bool isValidField(int xIndex, int yIndex) const; // index check for
        inline bool isValidField(const QPoint& cellIndex) const;// index check for

        bool setState(Cell& cell, Cell::State newState);        // modify cell state

    //
    // Exploration & Density
    //
    public:
        void computeDistanceTransform(Robot* robot = 0);
		bool cellInCentroid	(const Cell& cell, const QPointF& worldPos, double radius);
		bool cellInNetwork	(const Cell& cell, double radius);
		bool robotsInNetwork(const Cell& cell, double radius);
		void robotInRange(Robot* startRobot, QList<Robot*>* robots, double radius);
        void computeVoronoiPartition();
        void updateDensity();
        bool exploreInRadius(const QPointF& worldPos, double radius, bool markAsExplored);
        void unexploreAll();

        double explorationProgress() const;
        int freeCellCount() const;
        QVector<Cell*> visibleCells(const QPointF& worldPos, double radius);
        QVector<Cell*> visibleCells(Robot* robot, double radius);
        int numVisibleCellsUnrestricted(const QPointF& worldPos, double radius);
        void filterCells(QVector<Cell*> & cells, Robot* robot);

    //
    // Frontier caching for each robot
    //
    public:
        inline const QList<Cell*>& frontiers() const;           // cached list of all frontiers

        void updateRobotFrontierCache();
        QList<Cell*> frontiers(Robot* robot) const;             // frontiers for robot
        bool hasFrontiers(Robot* robot) const;
        QList<Cell*> frontiersForRobot(Robot* robot) const;

    private:
        QHash<Robot*, QList<Cell*> > m_robotFrontierCache;

    private:
        bool exploreCell(const QPoint& center, const QPoint& target, qreal radius, Cell::State targetState);

    //
    // path finding
    //
    public:
        bool pathVisible(const QPoint& from, const QPoint& to);
        bool pathVisibleUnrestricted(const QPoint& from, const QPoint& to);
        bool aaPathVisible(const QPoint& from, const QPoint& to);
        QList<Path> frontierPaths(const QPoint& start, const QList<Cell*>& frontiers);
        Path aStar(const QPoint& from, const QPoint& to);
        float heuristic(const QPoint& start, const QPoint& end);

    private:
        GridMap(); // disable default constructor

        Scene* m_scene;

        QVector<QVector<Cell> > m_map;
        QPixmap m_pixmapCache;
        QMap<Robot*, QPainterPath> m_partitionMap;

        qreal m_resolution;

        // track a list of frontiers for fast lookup/iteration
        QList<Cell*> m_frontierCache;
		int m_freeCellCount;
		int m_exploredCellCount;
		int m_oldexploredCellCount;
		bool m_isunemployed;
};

//
// inline methods
//

qreal GridMap::resolution() const
{
    return m_resolution;
}

bool GridMap::isValidField(int xIndex, int yIndex) const
{
    return xIndex >= 0 &&
           yIndex >= 0 &&
           xIndex < m_map.size() &&
           yIndex < m_map[0].size();
}

bool GridMap::isValidField(const QPoint& cellIndex) const
{
    return isValidField(cellIndex.x(), cellIndex.y());
}

const QList<Cell*>& GridMap::frontiers() const
{
    return m_frontierCache;
}

#endif // GRIDMAP_H

// kate: replace-tabs on; indent-width 4;
