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
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/QSet>

class QSettings;
class GridMap;
class Scene;

class Path
{
    public:
        Path() : m_cost(0.0f), m_length(0.0f) { }
        QList<QPoint> m_path;
        float m_cost;
        float m_length;
        
        void beautify(GridMap& gridMap);
};

class GridMap
{
    public:
        GridMap(Scene* scene, double width, double height, double resolution);
        ~GridMap();
        
    //
    // load from / save to given config object
    //
    public:
        void load(QSettings& config);
        void save(QSettings& config);

    //
    // drawing
    //
    public:
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

        qreal resolution() const;       // grid resolution. 0.2 means 0.2m x 0.2m
        QSize size() const;             // amount of cells in the grid map in

        inline int mapScreenToCell(qreal screenPos) const {
            return screenPos / (scaleFactor() * resolution());
        }
        
        QPoint mapScreenToCell(const QPointF& screenPos) const {
            return (screenPos / (scaleFactor() * resolution())).toPoint();
        }

        qreal mapScreenToMap(qreal screenPos) const {
            return screenPos / scaleFactor();
        }
        
        QPointF mapScreenToMap(const QPointF& screenPos) const {
            return screenPos / scaleFactor();
        }
        
        int mapMapToCell(qreal mapPos) const {
            return mapPos / resolution();
        }
        
        QPoint mapMapToCell(const QPointF& mapPos) const {
            return (mapPos / resolution()).toPoint();
        }


        qreal mapMapToScreen(qreal mapPos) const {
            return mapPos * scaleFactor();
        }

        QPointF mapMapToScreen(const QPointF& mapPos) const {
            return mapPos * scaleFactor();
        }

    //
    // cell accessors
    //
    public:
        Cell& cell(int xIndex, int yIndex);                     // cell accessor
        Cell& cell(const QPointF & index);                      // cell accessor
        bool isValidField(int xIndex, int yIndex) const;        // index check for 
        void setState(Cell& cell, Cell::State newState);        // modify cell state
        const QSet<Cell*>& frontiers() const;                   // cached list of all frontiers

    //
    // Exploration & Density
    //
    public:
        void computeDistanceTransform();
        void updateCellWeights();
        void explore(const QPointF& robotPos, double radius, Cell::State destState);
        
        double explorationProgress() const;
        QVector<Cell*> visibleCells(const QPointF& robotPos, double radius,
                                    Cell::State cellState = Cell::Free | Cell::Explored);

    //
    // path finding
    //
    public:
        bool pathVisible(const QPoint& from, const QPoint& to);
        bool aaPathVisible(const QPoint& from, const QPoint& to);
        bool DrawWuLine (const QPoint& from, const QPoint& to);
        QList<Path> frontierPaths(const QPoint& start);
        Path aStar(const QPoint& from, const QPoint& to);
        float heuristic(const QPoint& start, const QPoint& end);

    private:
        GridMap(); // disable default constructor

        Scene* m_scene;

        QVector<QVector<Cell> > m_map;
        QPixmap m_pixmapCache;

        qreal m_resolution;

        // track a list of frontiers for fast lookup/iteration
        QSet<Cell*> m_frontierCache;
        int m_freeCellCount;
        int m_exploredCellCount;
};

#endif // GRIDMAP_H

// kate: replace-tabs on; indent-width 4;
