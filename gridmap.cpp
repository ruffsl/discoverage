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
#include <QtCore/QBitArray>

#include <math.h>
#include <set>

static int directionMap[16][2] = {
    // x   y
    {  0, -1},           // oben
    {  1,  0},           // rechts
    {  0,  1},           // unten
    { -1,  0},           // links
    {  1, -1},           // rechts-oben
    {  1,  1},           // rechts-unten
    { -1,  1},           // links-unten
    { -1, -1},           // links-oben
    {  2, -1},           // rechts-oben
    {  2,  1},           // rechts-unten
    { -2,  1},           // links-unten
    { -2, -1},           // links-oben
    {  1, -2},           // rechts-oben
    {  1,  2},           // rechts-unten
    { -1,  2},           // links-unten
    { -1, -2}            // links-oben
};

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

// return;
    // Complexity: O(log(n))
    int start = 0;
    while (start < m_path.size() - 2) {
        const int end = m_path.size() - 1;
        int diff = (end - start);
        int mid = end;

        while (mid != start + 1) {
            bool wasVisible = false;
            if (gridMap.aaPathVisible(m_path[start], m_path[mid])) {
                if (mid == end) {
                    break;
                }
                if (diff == 1) wasVisible = true;
                diff = qMax(diff / 2, 1);
                mid += diff;
            }

            if (!gridMap.aaPathVisible(m_path[start], m_path[mid])) {
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
    m_exploredCellCount = 0;
    m_freeCellCount = xCellCount * yCellCount;
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
    m_exploredCellCount = 0;
    m_freeCellCount = width * height;

    for (int a = 0; a < width; ++a) {
        QVector<Cell>& row = m_map[a];
        for (int b = 0; b < height; ++b) {
            Cell& cell = row[b];
            cell.load(ds);
            cell.setIndex(QPoint(a, b));
            if (cell.state() & Cell::Frontier) {
                m_frontierCache.insert(&cell);
            }
            if (cell.state() & Cell::Free) {
                if (cell.state() & Cell::Explored) {
                    ++m_exploredCellCount;
                }
            } else {
                --m_freeCellCount;
            }
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

Cell& GridMap::cell(const QPointF & index)
{
    // assert on index-out-of-range
    Q_ASSERT(index.x() >= 0 && index.y() >= 0 && index.x() < size().width() && index.y() < size().height());
    return m_map[index.x()][index.y()];
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

void GridMap::updateCellWeights()
{
    Q_ASSERT(m_map.size() > 0);
    
    computeDist();
    
    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[a].size(); ++b) {
            Cell& c = m_map[a][b];
            if (c.state() & Cell::Explored &&
                c.state() & Cell::Free)
            {
                float dist = c.frontierDist();
                c.setDensity(exp(-0.5/(3*3)*dist*dist));
            }
        }
    }
//  

    return;

    QVector<QBitArray> visitedMap(m_map.size(), QBitArray(m_map[0].size()));

    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[a].size(); ++b) {
            Cell& c = m_map[a][b];
            if (  !(c.state() & Cell::Explored)
                || (c.state() & Cell::Obstacle)
                || visitedMap[a][b]
            ) {
                continue;
            }

            QList<Path> allFrontierPaths = frontierPaths(QPoint(a, b));
            
            float pathLength = 1000000;
            int index = -1;
            for (int i = 0; i < allFrontierPaths.size(); ++i) {
                if (allFrontierPaths[i].m_length < pathLength) {
                    index = i;
                    pathLength = allFrontierPaths[i].m_length;
                }
            }
            
            if (index != -1) {
//                 qDebug() << 1.0/(sqrt(2 * M_PI) * (2*1)) * exp(-0.5/(1) * pathLength*pathLength);
//                 while (pathLength > 10) pathLength -= 10;
//                 c.setDensity(pathLength / 10.0);
//                 qDebug() << 1.0/(sqrt(2 * M_PI) * (1)) * exp(-0.5/(1) * pathLength*pathLength);
//                 c.setDensity(1.0/(sqrt(2 * M_PI) * (1)) * exp(-0.5/(1) * pathLength*pathLength));
                c.setDensity(exp(-0.5/(3*3)*pathLength*pathLength));
            }
        }
    }
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

    // create QPixmap spanning the entire space Q
    if (m_pixmapCache.width() != scale * sizex * w + 1 ||
        m_pixmapCache.height() != scale * sizey * w + 1)
    {
        m_pixmapCache = QPixmap(scale * sizex * w + 1, scale * sizey * w + 1);
    }
    m_pixmapCache.fill();

    // precalculate all weights in the cells
    updateCellWeights();

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

void GridMap::setState(Cell& cell, Cell::State state)
{
    const Cell::State oldState = cell.state();
    cell.setState(state);
    const Cell::State newState = cell.state();

    const bool wasFrontier = oldState & Cell::Frontier;
    const bool isFrontier  = newState & Cell::Frontier;
    const bool wasFree = oldState & Cell::Free;
    const bool isFree  = newState & Cell::Free;
    const bool wasExplored = oldState & Cell::Explored;
    const bool isExplored  = newState & Cell::Explored;
    const bool wasObstacle = oldState & Cell::Obstacle;
    const bool isObstacle  = newState & Cell::Obstacle;

    if (wasFrontier && !isFrontier) {
        m_frontierCache.remove(&cell);
    } else if (!wasFrontier && isFrontier) {
        m_frontierCache.insert(&cell);
    }

    if (wasFree && !isFree) {
        --m_freeCellCount;
    } else if (!wasFree && isFree) {
        ++m_freeCellCount;
    }
    
    if (wasFree && isFree) {
        if (wasExplored && !isExplored) {
            --m_exploredCellCount;
        } else if (!wasExplored && isExplored) {
            ++m_exploredCellCount;
        }
    } else if (!wasFree && isFree) {
        if (isExplored) {
            ++m_exploredCellCount;
        }
    } else if (wasFree && !isFree) {
        if (wasExplored) {
            --m_exploredCellCount;
        }
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

    const int minCellX = 0;
    const int minCellY = 0;
    const int maxCellX = m_map.size() - 1;
    const int maxCellY = maxCellX > 0 ? m_map[0].size() - 1 : 0;

    for (int a = xStart; a <= xEnd; ++a) {
        for (int b = yStart; b <= yEnd; ++b) {
            Cell& c = m_map[a][b];
            if (c.state() & (Cell::Frontier | destState | Cell::Obstacle)) continue;

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

            if (count > 0) {
                bool freeNeighbor = false;
                if ((a > minCellX && b > minCellY && m_map[a-1][b-1].state() & destState && !(m_map[a-1][b-1].state() & Cell::Obstacle)) ||
                    (                b > minCellY && m_map[a  ][b-1].state() & destState && !(m_map[a  ][b-1].state() & Cell::Obstacle)) ||
                    (a < maxCellX && b > minCellY && m_map[a+1][b-1].state() & destState && !(m_map[a+1][b-1].state() & Cell::Obstacle)) ||
                    (a > minCellX &&                 m_map[a-1][b  ].state() & destState && !(m_map[a-1][b  ].state() & Cell::Obstacle)) ||
                    (a < maxCellX &&                 m_map[a+1][b  ].state() & destState && !(m_map[a+1][b  ].state() & Cell::Obstacle)) ||
                    (a > minCellX && b < maxCellY && m_map[a-1][b+1].state() & destState && !(m_map[a-1][b+1].state() & Cell::Obstacle)) ||
                    (                b < maxCellY && m_map[a  ][b+1].state() & destState && !(m_map[a  ][b+1].state() & Cell::Obstacle)) ||
                    (a < maxCellX && b < maxCellY && m_map[a+1][b+1].state() & destState && !(m_map[a+1][b+1].state() & Cell::Obstacle))
                   ) freeNeighbor = true;

                if (freeNeighbor) {
                    setState(c, Cell::Frontier);
                    updateCell(a, b);
                } 
            }
        }
    }
}

double GridMap::explorationProgress() const
{
    qDebug() << m_freeCellCount;
    if (m_freeCellCount == 0) return 0;
    return static_cast<double>(m_exploredCellCount) / m_freeCellCount;
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
    if (m_frontierCache.isEmpty()) {
        return QList<Path>();
    }

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

    int frontierCount = 0;

    while ((!queue.empty()) && frontierCount != m_frontierCache.size())
    {
        // Knoten mit den niedrigsten Kosten aus der Liste holen
        PathField f = *queue.begin();
        queue.erase(queue.begin());

        f.cell->setPathState(Cell::PathClose);  // Jetzt geschlossen
        int x = f.x, y = f.y;

        // speed up: if amount of frontier cells is already reached, terminate
        if (f.cell->state() & Cell::Frontier) {
            ++frontierCount;
        }

        // Alle angrenzenden Felder bearbeiten
        for (int i = 0; i < 8; ++i) {
            // Nachbarzelle
            int ax = x + directionMap[i][0];
            int ay = y + directionMap[i][1];

            // Testen ob neue x/y-Position gueltig ist ( Rand ist ausgenommen )
            if (!isValidField(ax, ay) || m_map[ax][ay].state() & Cell::Unknown)
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
        path.m_length *= resolution();
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
            if (m_map[x][y].isObstacle())
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
            if (m_map[x][y].isObstacle())
                return false;
//             result.append( QPoint( x, y ) );
        }
    }
    return true;
}


static inline int ipart(double x)
{
    return static_cast<int>(x);
}

static inline double fpart(double x)
{
    return x - static_cast<int>(x);
}

static inline double rfpart(double x)
{
    return 1.0 - fpart(x);
}

bool GridMap::aaPathVisible(const QPoint& from, const QPoint& to)
{
    // taken from http://www.codeproject.com/kb/gdi/antialias.aspx
    short X0 = from.x();;
    short Y0 = from.y();
    short X1 = to.x();
    short Y1 = to.y();
    
    short NumLevels = 256;
    unsigned short IntensityBits = 8;

    unsigned short IntensityShift, ErrorAdj, ErrorAcc;
    unsigned short ErrorAccTemp, Weighting, WeightingComplementMask;
    short DeltaX, DeltaY, Temp, XDir;

    /* Make sure the line runs top to bottom */
    if (Y0 > Y1) {
        Temp = Y0; Y0 = Y1; Y1 = Temp;
        Temp = X0; X0 = X1; X1 = Temp;
    }
    /* Draw the initial pixel, which is always exactly intersected by
    the line and so needs no weighting */
    if (m_map[X0][Y0].isObstacle()) return false; // DrawPixel(X0, Y0, BaseColor);

    if ((DeltaX = X1 - X0) >= 0) {
        XDir = 1;
    } else {
        XDir = -1;
        DeltaX = -DeltaX; /* make DeltaX positive */
    }
    /* Special-case horizontal, vertical, and diagonal lines, which
    require no weighting because they go right through the center of
    every pixel */
    if ((DeltaY = Y1 - Y0) == 0) {
        /* Horizontal line */
        while (DeltaX-- != 0) {
            X0 += XDir;
            if (m_map[X0][Y0].isObstacle()) return false; // DrawPixel(X0, Y0, BaseColor);
        }
        return true;
    }
    if (DeltaX == 0) {
        /* Vertical line */
        do {
            Y0++;
            if (m_map[X0][Y0].isObstacle()) return false; // DrawPixel(X0, Y0, BaseColor);
        } while (--DeltaY != 0);
        return true;
    }
    if (DeltaX == DeltaY) {
        /* Diagonal line */
        do {
            X0 += XDir;
            Y0++;
            if (m_map[X0][Y0].isObstacle()) return false; // DrawPixel(X0, Y0, BaseColor);
        } while (--DeltaY != 0);
        return true;
    }
    /* Line is not horizontal, diagonal, or vertical */
    ErrorAcc = 0;  /* initialize the line error accumulator to 0 */
    /* # of bits by which to shift ErrorAcc to get intensity level */
    IntensityShift = 16 - IntensityBits;
    /* Mask used to flip all bits in an intensity weighting, producing the
    result (1 - intensity weighting) */
    WeightingComplementMask = NumLevels - 1;
    /* Is this an X-major or Y-major line? */
    if (DeltaY > DeltaX) {
        /* Y-major line; calculate 16-bit fixed-point fractional part of a
        pixel that X advances each time Y advances 1 pixel, truncating the
        result so that we won't overrun the endpoint along the X axis */
        ErrorAdj = ((unsigned long) DeltaX << 16) / (unsigned long) DeltaY;
        /* Draw all pixels other than the first and last */
        while (--DeltaY) {
            ErrorAccTemp = ErrorAcc;   /* remember currrent accumulated error */
            ErrorAcc += ErrorAdj;      /* calculate error for next pixel */
            if (ErrorAcc <= ErrorAccTemp) {
                /* The error accumulator turned over, so advance the X coord */
                X0 += XDir;
            }
            Y0++; /* Y-major, so always advance Y */
            /* The IntensityBits most significant bits of ErrorAcc give us the
            intensity weighting for this pixel, and the complement of the
            weighting for the paired pixel */
            Weighting = ErrorAcc >> IntensityShift;
            if (m_map[X0][Y0].isObstacle()) return false; // DrawPixel(X0, Y0, BaseColor + Weighting);
            if (m_map[X0 + XDir][Y0].isObstacle()) return false; // DrawPixel(X0 + XDir, Y0, BaseColor + (Weighting ^ WeightingComplementMask));
        }
        /* Draw the final pixel, which is 
        always exactly intersected by the line
        and so needs no weighting */
        if (m_map[X1][Y1].isObstacle()) return false; // DrawPixel(X1, Y1, BaseColor);
        return true;
    }
    /* It's an X-major line; calculate 16-bit fixed-point fractional part of a
    pixel that Y advances each time X advances 1 pixel, truncating the
    result to avoid overrunning the endpoint along the X axis */
    ErrorAdj = ((unsigned long) DeltaY << 16) / (unsigned long) DeltaX;
    /* Draw all pixels other than the first and last */
    while (--DeltaX) {
        ErrorAccTemp = ErrorAcc;   /* remember currrent accumulated error */
        ErrorAcc += ErrorAdj;      /* calculate error for next pixel */
        if (ErrorAcc <= ErrorAccTemp) {
            /* The error accumulator turned over, so advance the Y coord */
            Y0++;
        }
        X0 += XDir; /* X-major, so always advance X */
        /* The IntensityBits most significant bits of ErrorAcc give us the
        intensity weighting for this pixel, and the complement of the
        weighting for the paired pixel */
        Weighting = ErrorAcc >> IntensityShift;
        if (m_map[X0][Y0].isObstacle()) return false; // DrawPixel(X0, Y0, BaseColor + Weighting);
        if (m_map[X0][Y0 + 1].isObstacle()) return false; // DrawPixel(X0, Y0 + 1, BaseColor + (Weighting ^ WeightingComplementMask));
    }
    /* Draw the final pixel, which is always exactly intersected by the line
    and so needs no weighting */
    if (m_map[X1][Y1].isObstacle()) return false; // DrawPixel(X1, Y1, BaseColor);
    
    return true;
}

void GridMap::computeDist()
{
    QList<Cell*> queue;

    // queue all free explored cells next to the frontier
    foreach (Cell* frontierCell, frontiers()) {

        frontierCell->setFrontierDist(0);

        for (int i = 0; i < 8; ++i) {
            // 8-neighborhood
            const int x = frontierCell->index().x() + directionMap[i][0];
            const int y = frontierCell->index().y() + directionMap[i][1];

            // check validity
            if (!isValidField(x, y))
                continue;

            Cell* cell = &m_map[x][y];

            // if free, set costs accordingly and queue
            if (cell->state() & Cell::Free && 
                cell->state() & Cell::Explored)
            {
                cell->setPathState(Cell::PathClose);
                const float dist = m_resolution * (i < 4 ? 1.0 : 1.4142136);
                if (!queue.contains(cell)) {
                    cell->setFrontierDist(dist);
                    queue.append(cell);
                } else if (cell->frontierDist() > dist) {
                    cell->setFrontierDist(dist);
                }
            }
        }
    }

    QList<Cell*> dirtyCells;
    // now we have all free explored cells next to the frontier in the queue
    // next, as long as the queue is not empty, flood by iterating the neighbors
    while (queue.size()) {
        Cell* baseCell = queue.takeFirst();
        dirtyCells.append(baseCell);
        baseCell->setPathState(Cell::PathClose);

        // 8-neighborhood
        for (int i = 0; i < 16; ++i) {
            const int x = baseCell->index().x() + directionMap[i][0];
            const int y = baseCell->index().y() + directionMap[i][1];

            // check validity
            if (!isValidField(x, y))
                continue;

            Cell* cell = &m_map[x][y];
//             if (cell->pathState() == Cell::PathClose)
//                 continue;

            // obstacle or not explored
            if (!(cell->state() & Cell::Free && 
                  cell->state() & Cell::Explored))
                continue;

            const float dist = baseCell->frontierDist() + m_resolution * (i < 4 ? 1.0 : 
                                                                (i < 8 ? 1.4142136 : 2.236068));

            // Ignorieren wenn Knoten geschlossen ist und bessere Kosten hat
            if (cell->pathState() == Cell::PathClose && cell->frontierDist() < dist)
                continue;

            // Cell ist bereits in der Queue, nur ersetzen wenn Kosten besser
            if (cell->pathState() == Cell::PathOpen)
            {
                if (cell->frontierDist() <= dist)
                    continue;

                // Alten Eintrag aus der Queue entfernen
                queue.removeAll(cell);
            }

            cell->setFrontierDist(dist);

            // flag open and queue
            cell->setPathState(Cell::PathOpen);
            queue.append(cell);
            
        }
    }

    foreach (Cell* cell, dirtyCells) {
        cell->setPathState(Cell::PathNone);
    }
}

// kate: replace-tabs on; indent-width 4;
