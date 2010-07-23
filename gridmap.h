#ifndef GRIDMAP_H
#define GRIDMAP_H

#include "cell.h"

#include <QtGui/QPixmap>
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/QSet>

class QSettings;

class Path
{
    public:
        Path() : m_cost(0.0f), m_length(0.0f) { }
        QList<QPoint> m_path;
        float m_cost;
        float m_length;
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
