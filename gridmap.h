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
        GridMap();
        GridMap(double width, double height, double resolution);
        ~GridMap();

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
        // returns desired widget size in pixel
        QSize displaySize() const;

        qreal scaleFactor() const;
        qreal resolution() const;

        void incScaleFactor();
        void decScaleFactor();

        QSize size() const;

        Cell& cell(int xIndex, int yIndex);

        bool isValidField(int xIndex, int yIndex) const;

        void setState(Cell& cell, Cell::State newState);

        const QSet<Cell*>& frontiers() const;

        /**
         * load and save grid map from config object
         */
        void load(QSettings& config);
        void save(QSettings& config);

    //
    // path finding
    //
    public:
        bool pathVisible(const QPoint& from, const QPoint& to);
        QList<Path> frontierPaths(const QPoint& start);
        Path aStar(const QPoint& from, const QPoint& to);
        float heuristic(const QPoint& start, const QPoint& end);

    private:
        QVector<QVector<Cell> > m_map;
        QPixmap m_pixmapCache;

        qreal m_scaleFactor;
        qreal m_resolution;

        // track a list of frontiers for fast lookup/iteration
        QSet<Cell*> m_frontierCache;
};

#endif // GRIDMAP_H

// kate: replace-tabs on; indent-width 4;
