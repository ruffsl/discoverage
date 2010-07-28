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

#include "gridmap.h"

#include <QPainter>
#include <QPoint>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QTime>

#include <math.h>
#include <set>

void Path::beautify(GridMap& gridMap)
{
// Complexity: O(n)
//     int start = 0;
// 
//     while (start < m_path.size() - 2) {
//         int end = start + 1;
//         while (end + 1 < m_path.size() && 
//             gridMap.pathVisible(m_path[start], m_path[end + 1]))
//         {
//             ++end;
//         }
// 
//         if (start + 1 < end) {
//             m_path.erase(m_path.begin() + start + 1, m_path.begin() + end);
//         }
//         ++start;
//     }


    // Complexity: O(log(n))
    int start = 0;
    while (start < m_path.size() - 2) {
        const int end = m_path.size() - 1;
        int diff = (end - start);
        int mid = end;

        while (mid != start + 1) {
            bool wasVisible = false;
            if (gridMap.pathVisible(m_path[start], m_path[mid])) {
                if (mid == end) {
                    break;
                }
                if (diff == 1) wasVisible = true;
                diff = qMax(diff / 2, 1);
                mid += diff;
            }

            if (!gridMap.pathVisible(m_path[start], m_path[mid])) {
                if (wasVisible && diff == 1) { mid -= 1; break; }
                if (end - start == 1) {
                    break;
                }
                diff = qMax(diff / 2, 1);
                mid -= diff;
            }
        }

        if (mid - start > 1) {
            m_path.erase(m_path.begin() + start + 1, m_path.begin() + mid);
            mid -= (mid - start) - 1;
        }
        start = mid;
        mid = m_path.size() - 1;
    }
}


GridMap::GridMap()
    : m_scaleFactor(4.0)
    , m_resolution(0.2)
{
    m_map = QVector<QVector<Cell> >(100, QVector<Cell>(100));

    for (int a = 0; a < m_map.size(); ++a) {
        QVector<Cell>& row = m_map[a];
        for (int b = 0; b < row.size(); ++b) {
            row[b].setRect(QRectF(a * m_resolution, b * m_resolution, m_resolution, m_resolution));
            row[b].setIndex(QPoint(a, b));
        }
    }

    m_frontierCache.clear();
}

GridMap::GridMap(double width, double height, double resolution)
    : m_scaleFactor(4.0)
    , m_resolution(resolution)
{
    const int xCellCount = ceil(width / resolution);
    const int yCellCount = ceil(height / resolution);

    m_map = QVector<QVector<Cell> >(xCellCount, QVector<Cell>(yCellCount));

    for (int a = 0; a < xCellCount; ++a) {
        QVector<Cell>& row = m_map[a];
        for (int b = 0; b < yCellCount; ++b) {
            row[b].setRect(QRectF(a * m_resolution, b * m_resolution, m_resolution, m_resolution));
            row[b].setIndex(QPoint(a, b));
        }
    }

    m_frontierCache.clear();
}

GridMap::~GridMap()
{
}

void GridMap::load(QSettings& config)
{
    config.beginGroup("scene");
    m_resolution = config.value("resolution",  0.2).toDouble();
    m_scaleFactor =  config.value("scale-factor", 4.0).toDouble();
    const int width = config.value("map-width", 0).toInt();
    const int height = config.value("map-height", 0).toInt();
    QByteArray ba = config.value("map", QByteArray()).toByteArray();
    config.endGroup();

    m_map = QVector<QVector<Cell> >(width, QVector<Cell>(height));

    QDataStream ds(&ba, QIODevice::ReadOnly);

    m_frontierCache.clear();

    for (int a = 0; a < width; ++a) {
        QVector<Cell>& row = m_map[a];
        for (int b = 0; b < height; ++b) {
            Cell& cell = row[b];
            cell.load(ds);
            cell.setIndex(QPoint(a, b));
            if (cell.state() & Cell::Frontier)
                m_frontierCache.insert(&cell);
        }
    }
}

void GridMap::save(QSettings& config)
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    ds.setVersion(QDataStream::Qt_4_5);

    // write map size
    const QSize s = size();
    for (int a = 0; a < s.width(); ++a) {
        QVector<Cell>& row = m_map[a];
        for (int b = 0; b < s.height(); ++b) {
            row[b].save(ds);
        }
    }

    config.beginGroup("scene");
    config.setValue("resolution", m_resolution);
    config.setValue("scale-factor", m_scaleFactor);
    config.setValue("map-width", s.width());
    config.setValue("map-height", s.height());
    config.setValue("map", ba);
    config.endGroup();
}

QSize GridMap::displaySize() const
{
    return m_pixmapCache.size();
}

QSize GridMap::size() const
{
    if (m_map.size() && m_map[0].size()) {
        return QSize(m_map.size(), m_map[0].size());
    }

    return QSize(0, 0);
}

Cell& GridMap::cell(int xIndex, int yIndex)
{
    // assert on index-out-of-range
    Q_ASSERT(xIndex >= 0 && yIndex >= 0 && xIndex < size().width() && yIndex < size().height());
    return m_map[xIndex][yIndex];
}

qreal GridMap::scaleFactor() const
{
    return m_scaleFactor / m_resolution;
}

void GridMap::incScaleFactor()
{
    m_scaleFactor += 1.0;
    updateCache();
}

void GridMap::decScaleFactor()
{
    if (m_scaleFactor > 1.0) {
        m_scaleFactor -= 1.0;
        updateCache();
    }
}

qreal GridMap::resolution() const
{
    return m_resolution;
}

void GridMap::updateCache()
{
    const int sizex = m_map.size();
    const int sizey = sizex > 0 ? m_map[0].size() : 0;

    if (sizex == 0 || sizey == 0) {
        m_pixmapCache = QPixmap();
        return;
    }

    const qreal w = m_resolution;
    const qreal scale = scaleFactor();

    m_pixmapCache = QPixmap(scale * sizex * w + 1, scale * sizey * w + 1);
    m_pixmapCache.fill();

    // finally draw to pixmap cache
    QPainter p(&m_pixmapCache);
    p.scale(scaleFactor(), scaleFactor());
    for (int a = 0; a < m_map.size(); ++a) {
        QVector<Cell>& row = m_map[a];
        for (int b = 0; b < row.size(); ++b) {
            row[b].draw(p);
        }
    }
}

void GridMap::draw(QPainter& p)
{
    p.drawPixmap(0, 0, m_pixmapCache);
//     qDebug() << m_frontierCache.size();
}

void GridMap::updateCell(int xIndex, int yIndex)
{
    updateCell(m_map[xIndex][yIndex]);
}

void GridMap::updateCell(Cell& cell)
{
    QPainter p(&m_pixmapCache);
    p.scale(scaleFactor(), scaleFactor());
    p.setPen(Qt::lightGray);
    cell.draw(p);
}

const QSet<Cell*>& GridMap::frontiers() const
{
    return m_frontierCache;
}

void GridMap::setState(Cell& cell, Cell::State newState)
{
    const bool wasFrontier = cell.state() & Cell::Frontier;
    cell.setState(newState);
    const bool isFrontier = cell.state() & Cell::Frontier;

    if (wasFrontier && !isFrontier) {
        m_frontierCache.remove(&cell);
    } else if (!wasFrontier && isFrontier) {
        m_frontierCache.insert(&cell);
    }
}

bool GridMap::isValidField(int xIndex, int yIndex) const
{
    return xIndex >= 0 &&
           yIndex >= 0 &&
           xIndex < m_map.size() &&
           yIndex < m_map[0].size();
}

static bool inCircle(qreal x, qreal y, qreal radius, qreal px, qreal py)
{
    qreal dx = x - px;
    qreal dy = y - py;

    return (dx*dx + dy*dy) < radius*radius;
}

void GridMap::explore(const QPointF& mapPos, double radius, Cell::State destState)
{
    const qreal x = mapPos.x();
    const qreal y = mapPos.y();

    const int cellRadius = ceil(radius / resolution());

    int cellX = mapPos.x() / resolution();
    int cellY = mapPos.y() / resolution();

    int xStart = qMax(0, cellX - cellRadius);
    int xEnd = qMin(size().width() - 1, cellX + cellRadius);

    int yStart = qMax(0, cellY - cellRadius - 1);
    int yEnd = qMin(size().height() - 1, cellY + cellRadius);

    for (int a = xStart; a <= xEnd; ++a) {
        for (int b = yStart; b <= yEnd; ++b) {
            Cell& c = m_map[a][b];
            if (!(c.state() & destState) && pathVisible(QPoint(cellX, cellY), QPoint(a, b))) {
                const QRectF& r = c.rect();
                const qreal x1 = r.left();
                const qreal x2 = r.right();
                const qreal y1 = r.top();
                const qreal y2 = r.bottom();

                int count = 0;
                if (inCircle(x, y, radius, x1, y1)) ++count;
                if (inCircle(x, y, radius, x1, y2)) ++count;
                if (inCircle(x, y, radius, x2, y1)) ++count;
                if (inCircle(x, y, radius, x2, y2)) ++count;

                if (count == 4) {
                    setState(c, destState);
                    updateCell(a, b);
                } else if (count > 0) {
                    setState(c, Cell::Frontier);
                    updateCell(a, b);
                }
            }
        }
    }
}



class PathField
{
public:
    PathField(const QPoint& field, Cell *cell)
    {
        x = field.x();
        y = field.y();
        this->cell = cell;
    }

    inline friend bool operator < (const PathField& lhs, const PathField& rhs)
    {
        return lhs.cell->m_costF < rhs.cell->m_costF;
    }

    inline friend bool operator == (const PathField& lhs, const PathField& rhs)
    {
        return lhs.cell->m_costF == rhs.cell->m_costF;
    }

    int x, y;
    Cell *cell;
};


QList<Path> GridMap::frontierPaths(const QPoint& start)
{
    static int directionMap[8][2] = {
        // x   y
        {  0, -1},           // oben
        {  1,  0},           // rechts
        {  0,  1},           // unten
        { -1,  0},           // links
        {  1, -1},           // rechts-oben
        {  1,  1},           // rechts-unten
        { -1,  1},           // links-unten
        { -1, -1}            // links-oben
    };

    QTime time;
    time.start();

    // Multiset sowie ein Iterator
    std::multiset<PathField> queue;
    std::multiset<PathField>::iterator itr;

    // Add starting square
    Cell *pCell = &m_map[start.x()][start.y()];
    pCell->m_costG = 0;
    pCell->m_costF = 0;
    pCell->setPathState(Cell::PathOpen);
    queue.insert(PathField(start, pCell));

    while (!queue.empty())
    {
        // Knoten mit den niedrigsten Kosten aus der Liste holen
        PathField f = *queue.begin();
        queue.erase(queue.begin());

        f.cell->setPathState(Cell::PathClose);  // Jetzt geschlossen
        int x = f.x, y = f.y;

        // Alle angrenzenden Felder bearbeiten
        for (int i = 0; i < 8; ++i) {
            // Nachbarzelle
            int ax = x + directionMap[i][0];
            int ay = y + directionMap[i][1];

            // Testen ob neue x/y-Position g�ltig ist ( Rand ist ausgenommen )
            if (!isValidField(ax, ay))
                continue;

            pCell = &m_map[ax][ay];

            // Kosten um zu diesem Feld zu gelangen:
            const float factor = i > 3 ? 1.41421356f : 1.0f;
            float G = f.cell->m_costG + factor * pCell->cellCost();   // Vorherige + aktuelle Kosten vom Start

            // Ignorieren wenn Knoten geschlossen ist und bessere Kosten hat
            if (pCell->pathState() == Cell::PathClose && pCell->m_costG < G)
                continue;

            // Cell ist bereits in der Queue, nur ersetzen wenn Kosten besser
            if (pCell->pathState() == Cell::PathOpen)
            {
                if (pCell->m_costG < G)
                    continue;

                // Alten Eintrag aus der Queue entfernen
                itr = queue.find(PathField(QPoint( ax, ay), pCell));
                if (itr != queue.end())
                {
                    // Es k�nnen mehrere Eintr�ge mit den gleichen Kosten vorhanden sein
                    // wir m�ssen den richtigen suchen
                    while ((*itr).cell != pCell)
                        itr++;

                    queue.erase(itr);
                }
            }

            // Knoten berechnen
            pCell->m_costG  = G;
            pCell->m_costF  = G + 0;
            pCell->m_pathParent = i;

            // Zu OPEN hinzuf�gen
            pCell->setPathState(Cell::PathOpen);
            queue.insert(PathField( QPoint( ax, ay ), pCell));
        }
    }

    QList<Path> m_frontierPaths;
    foreach (Cell* frontier, m_frontierCache) {
        // den Weg vom Ziel zum Start zurueckverfolgen und markieren
        int x = frontier->index().x();
        int y = frontier->index().y();
        int nParent;

        Path path;
        while (true) {
            pCell = &m_map[x][y];
            nParent = pCell->m_pathParent;
            path.m_path.prepend(QPoint(x, y));

            // Abbrechen wenn wir am Startknoten angekommen sind
            if( nParent == -1 )
                break;

            x -= directionMap[nParent][0];
            y -= directionMap[nParent][1];

            path.m_cost += pCell->cellCost();
            path.m_length += nParent < 4 ? 1.0f : 1.41421356f;
        }
        m_frontierPaths.append(path);
    }

//     qDebug() << "reconstruction" << time.elapsed();

    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[0].size(); ++b) {
            Cell& cell = m_map[a][b];
            cell.m_costF = 0.0f;
            cell.m_costG = 0.0f;
            cell.m_pathParent = -1;
            cell.setPathState(Cell::PathNone);
        }
    }
//     qDebug() << "cleanup" << time.elapsed();

    return m_frontierPaths;
}


Path GridMap::aStar(const QPoint& from, const QPoint& to)
{
    QList<Cell*> dirtyCells;
    static int directionMap[8][2] = {
        // x   y
        {  0, -1},           // oben
        {  1,  0},           // rechts
        {  0,  1},           // unten
        { -1,  0},           // links
        {  1, -1},           // rechts-oben
        {  1,  1},           // rechts-unten
        { -1,  1},           // links-unten
        { -1, -1}            // links-oben
    };

    QTime time;
    time.start();

    // Multiset sowie ein Iterator
    std::multiset<PathField> queue;
    std::multiset<PathField>::iterator itr;

    // 1.Add starting square
    Cell *pCell = &m_map[from.x()][from.y()];
    pCell->m_costG = 0;
    pCell->m_costF = heuristic(from, to);
    pCell->setPathState(Cell::PathOpen);
    queue.insert(PathField(from, pCell));

    bool success = false;

    while (!queue.empty())
    {
        // Knoten mit den niedrigsten Kosten aus der Liste holen
        PathField f = *queue.begin();
        queue.erase(queue.begin());
        dirtyCells.append(f.cell);

        f.cell->setPathState(Cell::PathClose);  // Jetzt geschlossen
        int x = f.x, y = f.y;

        // Wenn Ziel sind wir fertig
        if (x == to.x() && y == to.y()) {
            success = true;
            break;
        }

        // Alle angrenzenden Felder bearbeiten
        for (int i = 0; i < 8; ++i) {
            // Nachbarzelle
            int ax = x + directionMap[i][0];
            int ay = y + directionMap[i][1];

            // Testen ob neue x/y-Position g�ltig ist ( Rand ist ausgenommen )
            if (!isValidField(ax, ay))
                continue;

            pCell = &m_map[ax][ay];

            // Kosten um zu diesem Feld zu gelangen:
            const float factor = i > 3 ? 1.41421356f : 1.0f;
            float G = f.cell->m_costG + factor * pCell->cellCost();   // Vorherige + aktuelle Kosten vom Start

            // Ignorieren wenn Knoten geschlossen ist und bessere Kosten hat
            if (pCell->pathState() == Cell::PathClose && pCell->m_costG < G)
                continue;

            // Cell ist bereits in der Queue, nur ersetzen wenn Kosten besser
            if (pCell->pathState() == Cell::PathOpen)
            {
                if (pCell->m_costG < G)
                    continue;

                // Alten Eintrag aus der Queue entfernen
                itr = queue.find(PathField(QPoint( ax, ay), pCell));
                if (itr != queue.end())
                {
                    // Es k�nnen mehrere Eintr�ge mit den gleichen Kosten vorhanden sein
                    // wir m�ssen den richtigen suchen
                    while ((*itr).cell != pCell)
                        itr++;

                    queue.erase(itr);
                }
            }

            // Knoten berechnen
            pCell->m_costG  = G;
            pCell->m_costF  = G + heuristic(QPoint(ax, ay), to); // Kosten vom Start + Kosten zum Ziel
            pCell->m_pathParent = i;

            // Zu OPEN hinzuf�gen
            pCell->setPathState(Cell::PathOpen);
            queue.insert(PathField( QPoint( ax, ay ), pCell));
        }
    }

//     qDebug() << "A-Star" << time.elapsed();

    Path path;

    if (success) {
        // den Weg vom Ziel zum Start zurueckverfolgen und markieren
        Cell *pCell = &m_map[to.x()][to.y()];
        int x = to.x();
        int y = to.y();
        int nParent;

        while (true) {
            pCell = &m_map[x][y];
            nParent = pCell->m_pathParent;
            path.m_path.prepend(QPoint(x, y));

            // Abbrechen wenn wir am Startknoten angekommen sind
            if( nParent == -1 )
                break;

            x -= directionMap[nParent][0];
            y -= directionMap[nParent][1];

            path.m_cost += pCell->cellCost();
            path.m_length += nParent < 4 ? 1.0f : 1.41421356f;
        }
    }

//     qDebug() << "reconstruction" << time.elapsed();

    foreach (Cell* cell, dirtyCells) {
        cell->m_costF = 0.0f;
        cell->m_costG = 0.0f;
        cell->m_pathParent = -1;
        cell->setPathState(Cell::PathNone);
    }
//     qDebug() << "cleanup" << time.elapsed();

    qDebug() << "dirty cells:" << dirtyCells.size();

    return path;
}

float GridMap::heuristic(const QPoint& start, const QPoint& end)
{
        int dx = abs( start.x() - end.x() );
        int dy = abs( start.y() - end.y() );

        // the bigger the const value, the faster the path finding
        // the smaller the const value, the more accurate is the result
//        return 1 * qMax(dx, dy);

        // better approximation for distance
        return qMin(dx, dy) * 1.4 + (qMax(dx, dy) - qMin(dx, dy));
}

bool GridMap::pathVisible(const QPoint& from, const QPoint& to)
{
    int ystep, xstep;    // the step on y and x axis
    int error;           // the error accumulated during the increment
    int y = from.y();
    int x = from.x();    // the line points
    int ddy, ddx;        // compulsory variables: the double values of dy and dx
    int dx = to.x() - from.x();
    int dy = to.y() - from.y();
//     result.append( start );  // first point
    // NB the last point can't be here, because of its previous point (which has to be verified)
    if (dy < 0) {
        ystep = -1;
        dy = -dy;
    } else {
        ystep = 1;
    }

    if (dx < 0) {
        xstep = -1;
        dx = -dx;
    } else {
        xstep = 1;
    }

    ddy = 2 * dy;  // work with double values for full precision
    ddx = 2 * dx;
    if (ddx >= ddy) {  // first octant (0 <= slope <= 1)
        // compulsory initialization (even for errorprev, needed when dx==dy)
        error = dx;  // start in the middle of the square
        for( int i = 0 ; i < dx ; ++i )
        {  // do not use the first point (already done)
            x += xstep;
            error += ddy;
            if (error > ddx) {  // increment y if AFTER the middle ( > )
                y += ystep;
                error -= ddx;
            }
            if (!m_map[x][y].isPassable())
                return false;
//             result.append( QPoint( x, y ) );
        }
    } else {  // the same as above
        error = dy;
        for( int i = 0 ; i < dy ; ++i)
        {
            y += ystep;
            error += ddx;
            if (error > ddy) {
                x += xstep;
                error -= ddy;
            }
            if (!m_map[x][y].isPassable())
                return false;
//             result.append( QPoint( x, y ) );
        }
    }
    return true;
}

// kate: replace-tabs on; indent-width 4;
