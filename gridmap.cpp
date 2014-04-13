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
#include "scene.h"
#include "config.h"
#include "tikzexport.h"
#include "robotmanager.h"
#include "robot.h"

#include <QPainter>
#include <QPoint>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QTime>
#include <QtCore/QBitArray>

//#include <iostream>

#include <math.h>
#include <set>

static int directionMap[16][2] = {
    //
    //    x x
    //   x-o-x
    //    o o
    //   x-o-x
    //    x x
    //
    // 4 times o
    {  0, -1},  // top
    {  1,  0},  // right
    {  0,  1},  // bottom
    { -1,  0},  // left
    // 4 times -
    {  1, -1},  // top right
    {  1,  1},  // bottom right
    { -1,  1},  // bottom left
    { -1, -1},  // top left
    // 8 times x (horse jump from chess)
    {  2, -1},  // top right right
    {  2,  1},  // bottom right right
    { -2,  1},  // bottom left left
    { -2, -1},  // top left left
    {  1, -2},  // top top right
    {  1,  2},  // bottom bottom right
    { -1,  2},  // bottom bottom left
    { -1, -2}   // top top bottom left
};

void Path::beautify(GridMap& gridMap, bool computeExactLength)
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

    if (computeExactLength) {
        m_length = 0;
        for (int i = 0; i < m_path.size() - 1; ++i) {
            const QPoint d(m_path[i+1] - m_path[i]);
            m_length += sqrt((double)(d.x()*d.x() + d.y()*d.y())) * gridMap.resolution();
        }
    }
}

static const int border = 2;

GridMap::GridMap(Scene* scene, double width, double height, double resolution)
    : QObject(scene)
    , m_scene(scene)
    , m_resolution(resolution)
{
    const int xCellCount = ceil(width / m_resolution);
    const int yCellCount = ceil(height / m_resolution);

    m_map = QVector<QVector<Cell> >(xCellCount, QVector<Cell>(yCellCount));

    for (int a = 0; a < xCellCount; ++a) {
        QVector<Cell>& row = m_map[a];
        for (int b = 0; b < yCellCount; ++b) {
            row[b].setRect(QRectF(a * m_resolution, b * m_resolution, m_resolution, m_resolution));
            row[b].setIndex(QPoint(a, b));

            if (a == border || a == xCellCount - border - 1||
                b == border || b == yCellCount - border - 1)
                row[b].setState(Cell::Unknown | Cell::Obstacle);

            if (a < border || a > xCellCount - border - 1||
                b < border || b > yCellCount - border - 1)
                row[b].setState(Cell::Explored | Cell::Obstacle);
        }
    }

    m_frontierCache.clear();
    m_exploredCellCount = 0;
	m_oldexploredCellCount = 0;
	m_isunemployed = false;
    m_freeCellCount = (xCellCount - 2 * (border+1)) * (yCellCount - 2 * (border+1));

    updateCache();
}

GridMap::~GridMap()
{
}

void GridMap::load(QSettings& config)
{
    config.beginGroup("scene");
    m_resolution = config.value("resolution",  0.2).toDouble();
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
                m_frontierCache.append(&cell);
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

QSizeF GridMap::worldSize() const
{
    const int w = m_map.size();
    const int h = w > 0 ? m_map[0].size() : 0;

    return QSizeF(m_resolution * w,
                  m_resolution * h);
}

QPointF GridMap::center() const
{
    const int w = m_map.size();
    const int h = w > 0 ? m_map[0].size() : 0;

    return QPointF(m_resolution * w / 2,
                   m_resolution * h / 2);
}

double GridMap::convexDiameter() const
{
    const double w =  size().width();
    const double h =  size().height();
    return m_resolution * sqrt(w * w + h * h);
}

Cell& GridMap::cell(int xIndex, int yIndex)
{
    // assert on index-out-of-range
    Q_ASSERT(xIndex >= 0 && yIndex >= 0 && xIndex < size().width() && yIndex < size().height());
    return m_map[xIndex][yIndex];
}

Cell& GridMap::cell(const QPoint & cellIndex)
{
    return cell(cellIndex.x(), cellIndex.y());
}

qreal GridMap::scaleFactor() const
{
    return Config::self()->zoom() / m_resolution;
}

void GridMap::incScaleFactor()
{
    Config::self()->zoomIn();
}

void GridMap::decScaleFactor()
{
    Config::self()->zoomOut();
}

void GridMap::updateDensity()
{
    Q_ASSERT(m_map.size() > 0);
    
    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[a].size(); ++b) {
            Cell& c = m_map[a][b];
            if (c.state() & Cell::Explored &&
                c.state() & Cell::Free)
            {
                float dist = c.frontierDist();
                c.setDensity(exp(-0.5/(2*2)*dist*dist));
            } else {
                c.setDensity(1.0);
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

    // finally draw to pixmap cache
    QPainter p(&m_pixmapCache);
    p.scale(scaleFactor(), scaleFactor());
    for (int a = 0; a < m_map.size(); ++a) {
        QVector<Cell>& row = m_map[a];
        for (int b = 0; b < row.size(); ++b) {
            row[b].draw(p, Config::self()->showDensity(), Config::self()->showVectorField());
        }
    }

    m_partitionMap.clear();
    if (Config::self()->showPartition() && RobotManager::self()->count() > 1) {
        // create a QPainterPath for each robot
        for (int a = 0; a < m_map.size(); ++a) {
            QVector<Cell>& row = m_map[a];
            for (int b = 0; b < row.size(); ++b) {
                Robot* robot = row[b].robot();
                if (robot)
                    m_partitionMap[robot].addRect(row[b].rect());
            }
        }

        // simplify each painter path
        QMapIterator<Robot*, QPainterPath> it(m_partitionMap);
        while (it.hasNext()) {
            it.next();
            m_partitionMap[it.key()] = it.value().simplified();
        }
    }
}

void GridMap::draw(QPainter& p)
{
    p.drawPixmap(0, 0, m_pixmapCache);

    p.save();
    p.scale(scaleFactor(), scaleFactor());

    QMapIterator<Robot*, QPainterPath> it(m_partitionMap);
    while (it.hasNext()) {
        it.next();
        QColor col(it.key()->color());
        p.setPen(QPen(col, m_resolution * 0.3));
        col.setAlpha(96);
        p.setBrush(col);
        p.drawPath(it.value());
    }

//     foreach (Cell* c, m_frontierCache) {
//         p.fillRect(c->rect(), Qt::blue);
//     }
//     qDebug() << m_frontierCache.size();

    p.restore();
}

void GridMap::updateCell(int xIndex, int yIndex)
{
    updateCell(m_map[xIndex][yIndex]);
}

void GridMap::updateCell(Cell& cell)
{
    QPainter p(&m_pixmapCache);
    p.scale(scaleFactor(), scaleFactor());
    cell.draw(p, Config::self()->showDensity(), Config::self()->showVectorField());
}

bool GridMap::setState(Cell& cell, Cell::State state)
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
//     const bool wasObstacle = oldState & Cell::Obstacle;
//     const bool isObstacle  = newState & Cell::Obstacle;

    // update frontier cache
    if (wasFrontier && !isFrontier) {
        m_frontierCache.removeOne(&cell);
    } else if (!wasFrontier && isFrontier) {
        m_frontierCache.append(&cell);
    }

    // update free cell count
    if (wasFree && !isFree) {
        --m_freeCellCount;
    } else if (!wasFree && isFree) {
        ++m_freeCellCount;
    }

    // update explored cell count
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

    return oldState != newState;
}

inline static bool inCircle(qreal x, qreal y, qreal radius, qreal px, qreal py)
{
    qreal dx = x - px;
    qreal dy = y - py;

    return (dx*dx + dy*dy) <= radius*radius;
}

bool GridMap::exploreCell(const QPoint& center, const QPoint& target, qreal radius, Cell::State targetState)
{
    // check validity
    if (!isValidField(target.x(), target.y()))
        return false;

    Cell& c = m_map[target.x()][target.y()];

    // exit, if nothing to change
    if (c.state() & targetState)
        return false;

    // count the corners lying in the circle
    const QRectF& r = c.rect();
    const qreal x1 = r.left();
    const qreal x2 = r.right();
    const qreal y1 = r.top();
    const qreal y2 = r.bottom();

    Cell& centerCell = m_map[center.x()][center.y()];
    const qreal xCenter = centerCell.center().x();
    const qreal yCenter = centerCell.center().y();

    int count = 0;
    if (inCircle(xCenter, yCenter, radius, x1, y1)) ++count;
    if (inCircle(xCenter, yCenter, radius, x1, y2)) ++count;
    if (inCircle(xCenter, yCenter, radius, x2, y1)) ++count;
    if (inCircle(xCenter, yCenter, radius, x2, y2)) ++count;

    // exit, if outside radius
    if (count == 0)
        return false;

    // make sure the path is visible
    if (!pathVisible(center, target))
        return false;

    // if inside, mark as targetState, otherwise as Frontier
    bool changed = false;
    if (count == 4) {
        changed = setState(c, targetState);
    } else {
        changed = setState(c, c.isObstacle() ? targetState : Cell::Frontier);
    }

    // update pixmap cache
    if (changed)
        updateCell(target.x(), target.y());

    return changed;
}

bool GridMap::exploreInRadius(const QPointF& worldPos, double radius, bool markAsExplored)
{
    const Cell::State targetState = markAsExplored ? Cell::Explored : Cell::Unknown;

    const qreal xCenter = worldPos.x();
    const qreal yCenter = worldPos.y();

    const int cellRadius = ceil(radius / resolution());

    const int xCell = xCenter / resolution();
    const int yCell = yCenter / resolution();
    const QPoint robotIndex(xCell, yCell);

    bool changed = false;
    int count = 0;

    //
    // 1. explore from the center rectangular to the outside
    //
    while (count * m_resolution <= radius) {

        // explore square-like from left to right
        for (int dx = -count; dx <= count; ++dx) {
            const int x = xCell + dx;
            changed = exploreCell(robotIndex, QPoint(x, yCell + count), radius, targetState) || changed;
            changed = exploreCell(robotIndex, QPoint(x, yCell - count), radius, targetState) || changed;
        }

        // explore square-like from top to bottom
        for (int dy = -count; dy <= count; ++dy) {
            const int y = yCell + dy;
            changed = exploreCell(robotIndex, QPoint(xCell + count, y), radius, targetState) || changed;
            changed = exploreCell(robotIndex, QPoint(xCell - count, y), radius, targetState) || changed;
        }

        // add 4 missing corners
        changed = exploreCell(robotIndex, QPoint(xCell + count, yCell + count), radius, targetState) || changed;
        changed = exploreCell(robotIndex, QPoint(xCell + count, yCell - count), radius, targetState) || changed;
        changed = exploreCell(robotIndex, QPoint(xCell - count, yCell + count), radius, targetState) || changed;
        changed = exploreCell(robotIndex, QPoint(xCell - count, yCell - count), radius, targetState) || changed;

        ++count;
    }

    //
    // 2. neighbors to explored cells are either frontiers or explored
    //
    const int minCellX = 0;
    const int minCellY = 0;
    const int maxCellX = m_map.size() - 1;
    const int maxCellY = maxCellX > 0 ? m_map[0].size() - 1 : 0;

    const int xStart = qMax(0, xCell - cellRadius - 1);
    const int xEnd = qMin(size().width() - 1, xCell + cellRadius + 1);

    const int yStart = qMax(0, yCell - cellRadius - 1);
    const int yEnd = qMin(size().height() - 1, yCell + cellRadius + 1);

    for (int a = xStart; a <= xEnd; ++a) {
        for (int b = yStart; b <= yEnd; ++b) {
            Cell& c = m_map[a][b];
            if (c.state() & (Cell::Frontier | targetState /*| Cell::Obstacle*/)) continue;

            bool freeNeighbor = false;
            if ((a > minCellX && b > minCellY && m_map[a-1][b-1].state() & targetState && !(m_map[a-1][b-1].state() & Cell::Obstacle)) ||
                (                b > minCellY && m_map[a  ][b-1].state() & targetState && !(m_map[a  ][b-1].state() & Cell::Obstacle)) ||
                (a < maxCellX && b > minCellY && m_map[a+1][b-1].state() & targetState && !(m_map[a+1][b-1].state() & Cell::Obstacle)) ||
                (a > minCellX &&                 m_map[a-1][b  ].state() & targetState && !(m_map[a-1][b  ].state() & Cell::Obstacle)) ||
                (a < maxCellX &&                 m_map[a+1][b  ].state() & targetState && !(m_map[a+1][b  ].state() & Cell::Obstacle)) ||
                (a > minCellX && b < maxCellY && m_map[a-1][b+1].state() & targetState && !(m_map[a-1][b+1].state() & Cell::Obstacle)) ||
                (                b < maxCellY && m_map[a  ][b+1].state() & targetState && !(m_map[a  ][b+1].state() & Cell::Obstacle)) ||
                (a < maxCellX && b < maxCellY && m_map[a+1][b+1].state() & targetState && !(m_map[a+1][b+1].state() & Cell::Obstacle))
                ) freeNeighbor = true;

            if (freeNeighbor) {
                changed = setState(c, c.isObstacle() ? targetState : Cell::Frontier) || changed;
                updateCell(a, b);
            }
        }
    }

    return changed;
}

void GridMap::unexploreAll()
{
    const QSize s = size();
    for (int a = 0; a < s.width(); ++a) {
        QVector<Cell>& row = m_map[a];
        for (int b = 0; b < s.height(); ++b) {
            setState(row[b], Cell::Unknown);
        }
    }
}

QVector<Cell*> GridMap::visibleCells(const QPointF& worldPos, double radius)
{
    const qreal x = worldPos.x();
    const qreal y = worldPos.y();

    int cellX = x / resolution();
    int cellY = y / resolution();

    QVector<Cell*> cellVector;
    if (!isValidField(cellX, cellY)) {
        return cellVector;
    }

    const int cellRadius = ceil(radius / resolution());

    int xStart = qMax(0, cellX - cellRadius);
    int xEnd = qMin(size().width() - 1, cellX + cellRadius);

    int yStart = qMax(0, cellY - cellRadius - 1);
    int yEnd = qMin(size().height() - 1, cellY + cellRadius);

    for (int a = xStart; a <= xEnd; ++a) {
        for (int b = yStart; b <= yEnd; ++b) {
            Cell& c = m_map[a][b];
            if (!(c.state() == (Cell::Explored | Cell::Obstacle))
                && pathVisible(QPoint(cellX, cellY), QPoint(a, b)))
            {
                const QRectF& r = c.rect();
                const qreal x1 = r.left();
                const qreal x2 = r.right();
                const qreal y1 = r.top();
                const qreal y2 = r.bottom();

                bool visible = inCircle(x, y, radius, x1, y1)
                            || inCircle(x, y, radius, x1, y2)
                            || inCircle(x, y, radius, x2, y1)
                            || inCircle(x, y, radius, x2, y2);

                if (visible) {
                    cellVector.append(&c);
                }
            }
        }
    }
    return cellVector;
}

int GridMap::numVisibleCellsUnrestricted(const QPointF& worldPos, double radius)
{
    const qreal x = worldPos.x();
    const qreal y = worldPos.y();

    int cellX = x / resolution();
    int cellY = y / resolution();

    int result = 0;
    if (!isValidField(cellX, cellY)) {
        return 0;
    }

    const int cellRadius = ceil(radius / resolution());

    // TODO: warum bei yStart -1 und bei x nicht?
    int xStart = cellX - cellRadius;
    int xEnd = cellX + cellRadius;

    int yStart = cellY - cellRadius - 1;
    int yEnd = cellY + cellRadius;

    for (int a = xStart; a <= xEnd; ++a) {
        for (int b = yStart; b <= yEnd; ++b) {
            if (isValidField(a, b))
            {
                Cell& c = m_map[a][b];
                if (!(c.state() == (Cell::Explored | Cell::Obstacle))
                    && pathVisibleUnrestricted(QPoint(cellX, cellY), QPoint(a, b)))
                {
                    const QRectF& r = c.rect();
                    const qreal x1 = r.left();
                    const qreal x2 = r.right();
                    const qreal y1 = r.top();
                    const qreal y2 = r.bottom();

                    bool visible = inCircle(x, y, radius, x1, y1)
                                || inCircle(x, y, radius, x1, y2)
                                || inCircle(x, y, radius, x2, y1)
                                || inCircle(x, y, radius, x2, y2);

                    if (visible) {
                        ++result;
                    }
                }
            }
            else if (pathVisibleUnrestricted(QPoint(cellX, cellY), QPoint(a, b)))
            {
                // create virtual cell
                QRectF r(a * m_resolution, b * m_resolution, m_resolution, m_resolution);
                
                const qreal x1 = r.left();
                const qreal x2 = r.right();
                const qreal y1 = r.top();
                const qreal y2 = r.bottom();

                bool visible = inCircle(x, y, radius, x1, y1)
                            || inCircle(x, y, radius, x1, y2)
                            || inCircle(x, y, radius, x2, y1)
                            || inCircle(x, y, radius, x2, y2);

                if (visible) {
                    ++result;
                }
            }
        }
    }
    return result;
}

QVector<Cell*> GridMap::visibleCells(Robot* robot, double radius)
{
    Q_ASSERT(robot);

    // if only one robot exists, just return all visible cells
    QVector<Cell*> visibleList = visibleCells(robot->position(), radius);
    if (RobotManager::self()->count() == 1) {
        return visibleList;
    }

    // make sure the cell is assigned to this robot
    QVector<Cell*> cellVector;
    for (QVector<Cell*>::const_iterator it = visibleList.begin(); it != visibleList.end(); ++it)
    {
        if ((*it)->robot() == robot) {
            cellVector.append(*it);
        }
    }

    return cellVector;
}

void GridMap::filterCells(QVector<Cell*> & cells, Robot* robot)
{
    for (int i = 0; i < cells.size(); ) {
        Cell* c = cells[i];
        if (c->robot() != robot) {
            // swap with last element, then delete last
            qSwap(cells[i], cells.last());
            cells.pop_back();
        } else {
            ++i;
        }
    }
}

void GridMap::updateRobotFrontierCache()
{
    // clear cache before filling it
    m_robotFrontierCache.clear();

    // shortcut for only one robot
    if (RobotManager::self()->count() == 1) {
        if (m_frontierCache.size() > 0)
            m_robotFrontierCache[RobotManager::self()->robot(0)] = m_frontierCache;
        return;
    }

    // now assign each frontier cell to the correct list
    foreach (Cell* c, m_frontierCache) {
        m_robotFrontierCache[c->robot()].append(c);
    }
}

QList<Cell*> GridMap::frontiers(Robot* robot) const
{
    if (m_robotFrontierCache.contains(robot)) {
        return m_robotFrontierCache[robot];
    }

    return QList<Cell*>();
}

bool GridMap::hasFrontiers(Robot* robot) const 
{
    return m_robotFrontierCache.contains(robot);
//         && m_robotFrontierCache[robot].size() > 0;
}

double GridMap::explorationProgress() const
{
//     qDebug() << m_exploredCellCount << "/" << m_freeCellCount;
    if (m_freeCellCount == 0) return 0;
    return static_cast<double>(m_exploredCellCount) / m_freeCellCount;
}

int GridMap::freeCellCount() const
{
    return m_freeCellCount;
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


QList<Path> GridMap::frontierPaths(const QPoint& start, const QList<Cell*>& frontiers)
{
    if (frontiers.isEmpty()) {
        return QList<Path>();
    }

//     QTime time;
//     time.start();

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
		// Get the node with the lowest cost from the list
        PathField f = *queue.begin();
        queue.erase(queue.begin());

        f.cell->setPathState(Cell::PathClose);  // Jetzt geschlossen
        int x = f.x, y = f.y;

        // Alle angrenzenden Felder bearbeiten
		// Process all adjacent fields
        for (int i = 0; i < 8; ++i) {
            // Nachbarzelle
            int ax = x + directionMap[i][0];
            int ay = y + directionMap[i][1];

            // Testen ob neue x/y-Position gueltig ist ( Rand ist ausgenommen )
			// Test whether new x/y-Pposition is valid (edge is excluded)
            if (!isValidField(ax, ay))
                continue;

            pCell = &m_map[ax][ay];

            // Kosten um zu diesem Feld zu gelangen:
			// Cost to get to this box:
            const float factor = i > 3 ? 1.41421356f : 1.0f;
            float G = f.cell->m_costG + factor * pCell->cellCost();   // Vorherige + aktuelle Kosten vom Start

            // Ignorieren wenn Knoten geschlossen ist und bessere Kosten hat
			// Ignore if node is closed and has better cost
            if (pCell->pathState() == Cell::PathClose && pCell->m_costG < G)
                continue;

            // Cell ist bereits in der Queue, nur ersetzen wenn Kosten besser
			// Cell is already in the queue, only replace if better cost
            if (pCell->pathState() == Cell::PathOpen)
            {
                if (pCell->m_costG < G)
                    continue;

                // Alten Eintrag aus der Queue entfernen
				// Remove the old entry from the queue
                itr = queue.find(PathField(QPoint( ax, ay), pCell));
                if (itr != queue.end())
                {
                    // Es k�nnen mehrere Eintr�ge mit den gleichen Kosten vorhanden sein
                    // wir m�ssen den richtigen suchen
					// There can be several records with all same cost exist
					// We need the right search
                    while ((*itr).cell != pCell)
                        itr++;

                    queue.erase(itr);
                }
            }

            // Knoten berechnen
			// Get node
            pCell->m_costG  = G;
            pCell->m_costF  = G + 0;
            pCell->m_pathParent = i;

            // Zu OPEN hinzufuegen
			// Add to OPEN
            pCell->setPathState(Cell::PathOpen);
            queue.insert(PathField( QPoint( ax, ay ), pCell));
        }
    }

    QList<Path> frontierPaths;
    foreach (Cell* frontier, frontiers) {
        // den Weg vom Ziel zum Start zurueckverfolgen und markieren
        int x = frontier->index().x();
        int y = frontier->index().y();

        Path path;
        while (true) {
            path.m_path.prepend(QPoint(x, y));
            pCell = &m_map[x][y];

            // Abbrechen wenn wir am Startknoten angekommen sind
            int nParent = pCell->m_pathParent;
            if( nParent == -1 )
                break;

            x -= directionMap[nParent][0];
            y -= directionMap[nParent][1];

            path.m_cost += pCell->cellCost();
            path.m_length += nParent < 4 ? 1.0f : 1.41421356f;
        }
        path.m_length *= resolution();
        frontierPaths.append(path);
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

    return frontierPaths;
}


Path GridMap::aStar(const QPoint& from, const QPoint& to)
{
    QList<Cell*> dirtyCells;

//     QTime time;
//     time.start();

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

            // Testen ob neue x/y-Position gueltig ist ( Rand ist ausgenommen )
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
            if (i == dx - 1) return true;
            if (m_map[x][y].state() & Cell::Obstacle)
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
            if (i == dy - 1) return true;
            if (m_map[x][y].state() & Cell::Obstacle)
                return false;
//             result.append( QPoint( x, y ) );
        }
    }
    return true;
}

bool GridMap::pathVisibleUnrestricted(const QPoint& from, const QPoint& to)
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
            if (!isValidField(x, y) || i == dx - 1) return true;
            if (m_map[x][y].state() & Cell::Obstacle && m_map[x][y].state() & Cell::Explored)
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
            if (!isValidField(x, y) || i == dy - 1) return true;
            if (m_map[x][y].state() & Cell::Obstacle && m_map[x][y].state() & Cell::Explored)
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

//     short NumLevels = 256;
//     unsigned short IntensityBits = 8;

    unsigned short /*IntensityShift,*/ ErrorAdj, ErrorAcc;
    unsigned short ErrorAccTemp/*, Weighting, WeightingComplementMask*/;
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
//     IntensityShift = 16 - IntensityBits;
    /* Mask used to flip all bits in an intensity weighting, producing the
    result (1 - intensity weighting) */
//     WeightingComplementMask = NumLevels - 1;
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
//             Weighting = ErrorAcc >> IntensityShift;
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
//         Weighting = ErrorAcc >> IntensityShift;
        if (m_map[X0][Y0].isObstacle()) return false; // DrawPixel(X0, Y0, BaseColor + Weighting);
        if (m_map[X0][Y0 + 1].isObstacle()) return false; // DrawPixel(X0, Y0 + 1, BaseColor + (Weighting ^ WeightingComplementMask));
    }
    /* Draw the final pixel, which is always exactly intersected by the line
    and so needs no weighting */
    if (m_map[X1][Y1].isObstacle()) return false; // DrawPixel(X1, Y1, BaseColor);
    
    return true;
}

static inline int sgn(int val) {
    return (0 < val) - (val < 0);
}

void GridMap::computeDistanceTransform(Robot* robot)
{
//     QTime time;
//     time.start();

    const QList<Cell*> f = frontiers(robot);

    // if no frontiers -> set dist to 0 everywhere
    if (f.isEmpty()) {
        for (int a = 0; a < m_map.size(); ++a) {
            for (int b = 0; b < m_map[a].size(); ++b) {
                Cell& c = m_map[a][b];
                if (c.robot() == robot && c.state() == (Cell::Free | Cell::Explored)) {
                    c.setFrontierDist(0);
                }
            }
        }
        return;
    }

    QList<Cell*> queue;

    // queue all frontier cells
    foreach (Cell* frontierCell, f) {
        frontierCell->setFrontierDist(0);
        queue.append(frontierCell);
    }

    QList<Cell*> dirtyCells;
    // now we have all free explored cells next to the frontier in the queue
    // next, as long as the queue is not empty, flood by iterating the neighbors
    while (queue.size()) {
        Cell* baseCell = queue.takeFirst();
        dirtyCells.append(baseCell);
        baseCell->setPathState(Cell::PathClose);

        const int xBase = baseCell->index().x();
        const int yBase = baseCell->index().y();

        // 16-neighborhood
        for (int i = 0; i < 16; ++i) {
            const int x = xBase + directionMap[i][0];
            const int y = yBase + directionMap[i][1];

            // check validity
            if (!isValidField(x, y))
                continue;

            Cell* cell = &m_map[x][y];

            // if not in correct voronoi cell, ignore
            if (robot != 0 && cell->robot() != robot)
                continue;

            // obstacle or not explored
            if (!(cell->state() == (Cell::Free | Cell::Explored)))
                continue;

            // chess horse jumps: make sure cells inbetween are free and explored
            if (i >= 8 && i < 12) {
                // {  2, -1},  // top right right
                // {  2,  1},  // bottom right right
                // { -2,  1},  // bottom left left
                // { -2, -1},  // top left left
                const int xIdx = xBase + sgn(directionMap[i][0]);
                if (!isValidField(xIdx, yBase) || m_map[xIdx][yBase].state() != (Cell::Free | Cell::Explored)
                 || !isValidField(xIdx, yBase + directionMap[i][1]) || m_map[xIdx][yBase + directionMap[i][1]].state() != (Cell::Free | Cell::Explored))
                    continue;
            } else if (i >= 12) {
                // {  1, -2},  // top top right
                // {  1,  2},  // bottom bottom right
                // { -1,  2},  // bottom bottom left
                // { -1, -2}   // top top bottom left
                const int yIdx = yBase + sgn(directionMap[i][1]);
                if (!isValidField(xBase, yIdx) || m_map[xBase][yIdx].state() != (Cell::Free | Cell::Explored)
                 || !isValidField(xBase + directionMap[i][0], yIdx) || m_map[xBase + directionMap[i][0]][yIdx].state() != (Cell::Free | Cell::Explored))
                    continue;
            }

            const float dist = baseCell->frontierDist()
                     + m_resolution * (i < 4 ? 1.0 : (i < 8 ? 1.4142136 : 2.236068));

            // Ignorieren wenn Knoten geschlossen ist und bessere Kosten hat
            if (cell->pathState() == Cell::PathClose && cell->frontierDist() < dist)
                continue;

            // cell already in queue, only replace, if shorter path
            if (cell->pathState() == Cell::PathOpen) {
                if (cell->frontierDist() <= dist)
                    continue;

                // remove all entry
                queue.removeOne(cell);
            }

            cell->setFrontierDist(dist);

            // flag open and queue
            cell->setPathState(Cell::PathOpen);
            queue.append(cell);
        }
    }

    // cleanup again
    foreach (Cell* cell, dirtyCells) {
        cell->setPathState(Cell::PathNone);
    }

//     qDebug() << "computeDistanceTransform took " << time.elapsed() << "milli seconds";
}


bool GridMap::cellInCentroid(const Cell& cell ,const QPointF& worldPos, double radius)
{
	const qreal x = worldPos.x();
	const qreal y = worldPos.y();

//	int cellX = x / resolution();
//	int cellY = y / resolution();

	const QRectF& r = cell.rect();
	const qreal x1 = r.left();
	const qreal x2 = r.right();
	const qreal y1 = r.top();
	const qreal y2 = r.bottom();

	bool visible = inCircle(x, y, radius, x1, y1)
				|| inCircle(x, y, radius, x1, y2)
				|| inCircle(x, y, radius, x2, y1)
				|| inCircle(x, y, radius, x2, y2);

	return visible;
}


bool GridMap::cellInNetwork(const Cell& cell, double radius)
{
	int count = 0;
	for (int i = 0; i < RobotManager::self()->count(); ++i) {
		Robot* robot = RobotManager::self()->robot(i);
		if (cellInCentroid(cell, robot->position(), radius))
			count++;
		}

	if (count < 2)
		return false;

	return true;
}

bool GridMap::robotsInNetwork(const Cell& cell , double radius)
{

	QList<Robot*> robots;
	for (int i = 0; i < RobotManager::self()->count(); ++i) {
		Robot* robot = RobotManager::self()->robot(i);
		robots.append(robot);
		}

	Robot* startRobot = cell.robot();

	robotInRange(startRobot, &robots, radius);

	if (robots.size())
		return false;
	return true;
}

void GridMap::robotInRange(Robot* startRobot, QList<Robot*>* robots, double radius){

	robots->removeOne(startRobot);
	foreach(Robot* robot, *robots) {
		if (startRobot){
			qreal x = startRobot->position().x();
			qreal y = startRobot->position().y();
			qreal x1 = robot->position().x();
			qreal y1 = robot->position().y();

			if(inCircle(x, y, radius+1, x1, y1))
				robotInRange(robot, robots, radius);
		}
	}
}


//Ruffin's Bookmark
void GridMap::computeVoronoiPartition()
{
//     QTime time;
//     time.start();


	float mindist = HUGE_VALF;
	Robot* minRobot;
	if (m_isunemployed){
		for (int a = 0; a < m_map.size(); ++a) {
			for (int b = 0; b < m_map[a].size(); ++b) {
				Cell& c = m_map[a][b];
				if((c.state()==(Cell::Free | Cell::Frontier))|(c.state()==(Cell::Unknown | Cell::Frontier))|(c.state()==(Cell::Obstacle | Cell::Frontier))){
					int dist = c.robotDist();
					if(dist <= mindist){
						mindist = c.robotDist();
						minRobot = c.robot();
					}
				}
			}
		}
	}

    // take shortcut: if only one robot, assign it to all cells
    if (RobotManager::self()->count() == 1) {
        Robot* robot = RobotManager::self()->robot(0);
        for (int a = 0; a < m_map.size(); ++a) {
            for (int b = 0; b < m_map[a].size(); ++b) {
                Cell& c = m_map[a][b];
                c.setRobot(robot);
            }
        }
//         qDebug() << "computeVoronoiPartition took " << time.elapsed() << "milli seconds";
        return;
    }

    // set robot of all cells to 0
    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[a].size(); ++b) {
            m_map[a][b].setRobot(0);
        }
    }

    // flooding algorithm with robots as seeds
    QList<Cell*> queue;

    // queue all robots
    for (int i = 0; i < RobotManager::self()->count(); ++i) {
        Robot* robot = RobotManager::self()->robot(i);
        QPoint cellIndex = worldToIndex(robot->position());
        if (isValidField(cellIndex)) {
            Cell* cell = &m_map[cellIndex.x()][cellIndex.y()];
            cell->setRobot(robot);
            cell->setRobotDist(0);
            queue.append(cell);
        }
    }

	// Ruffin's Code
	//###########################################################
	qreal radius = 5;
	QPointF centroid;
	// get centroid of all robots
	for (int i = 0; i < RobotManager::self()->count(); ++i) {
		Robot* robot = RobotManager::self()->robot(i);
		centroid += robot->position();
	}
	centroid /= RobotManager::self()->count();

	qDebug() << "centroid: x" << centroid.x() << " y"<< centroid.y();

	//###########################################################

    QList<Cell*> dirtyCells;
    // now we have all robots as seeds in the queue
    // next, as long as the queue is not empty, flood by iterating the neighbors
    while (queue.size()) {
        Cell* baseCell = queue.takeFirst();
        dirtyCells.append(baseCell);
        baseCell->setPathState(Cell::PathClose);

        const int xBase = baseCell->index().x();
        const int yBase = baseCell->index().y();

        // 16-neighborhood
        for (int i = 0; i < 16; ++i) {
            const int x = xBase + directionMap[i][0];
            const int y = yBase + directionMap[i][1];

			// check validity
			if (!isValidField(x, y))
				continue;


			Cell* cell = &m_map[x][y];

//			//####################################
//			// check if in range
//			if (!cellInCentroid(*cell, centroid, radius))
//				continue;
//			//####################################

//			//####################################
//			// check if in range of all robots
//			if (!cellInNetwork(*cell, radius))
//				continue;
//			//####################################

			//####################################
			// check if in range of all robots
			if(m_oldexploredCellCount != m_exploredCellCount){
				m_isunemployed = false;
			}

			qreal unemployed = 0.0;
			const int count = RobotManager::self()->count();
			for (int i = 0; i < count; ++i) {
				if (RobotManager::self()->robot(i)->stats().isUnemployed())
					unemployed += 1;
			}
			unemployed /= count;
			if (unemployed == 1.0){
				m_oldexploredCellCount = m_exploredCellCount;
				m_isunemployed = true;
				minRobot = 0;
			}

			if (!m_isunemployed){
				if (!cellInNetwork(*cell, radius))
					continue;
			}
			else{
				Robot* myrobot = baseCell->robot();
				if ((myrobot == minRobot)|(minRobot==0)){
					int lol;
					lol = 1;
				}
				else{
					if (!cellInNetwork(*cell, radius)){
						continue;
					}
				}
			}
			//####################################


            // obstacle or not explored
//             if (!(cell->state() == (Cell::Free | Cell::Explored)))
            if (cell->state() == (Cell::Obstacle | Cell::Explored)) {
                cell->setRobot(0);
                continue;
            }

            // chess horse jumps: make sure cells inbetween are free and explored
            if (i >= 8 && i < 12) {
                // {  2, -1},  // top right right
                // {  2,  1},  // bottom right right
                // { -2,  1},  // bottom left left
                // { -2, -1},  // top left left
                const int xIdx = xBase + sgn(directionMap[i][0]);
                if (!isValidField(xIdx, yBase) || m_map[xIdx][yBase].state() != (Cell::Free | Cell::Explored)
                    || !isValidField(xIdx, yBase + directionMap[i][1]) || m_map[xIdx][yBase + directionMap[i][1]].state() != (Cell::Free | Cell::Explored))
                    continue;
            } else if (i >= 12) {
                // {  1, -2},  // top top right
                // {  1,  2},  // bottom bottom right
                // { -1,  2},  // bottom bottom left
                // { -1, -2}   // top top bottom left
                const int yIdx = yBase + sgn(directionMap[i][1]);
                if (!isValidField(xBase, yIdx) || m_map[xBase][yIdx].state() != (Cell::Free | Cell::Explored)
                    || !isValidField(xBase + directionMap[i][0], yIdx) || m_map[xBase + directionMap[i][0]][yIdx].state() != (Cell::Free | Cell::Explored))
                    continue;
            }

            const float dist = baseCell->robotDist()
                + m_resolution * (i < 4 ? 1.0 : (i < 8 ? 1.4142136 : 2.236068));

            // Ignorieren wenn Knoten geschlossen ist und bessere Kosten hat
            if (cell->pathState() == Cell::PathClose && cell->robotDist() < dist)
                continue;

            // cell already in queue, only replace, if shorter path
            if (cell->pathState() == Cell::PathOpen) {
                if (cell->robotDist() <= dist)
                    continue;

                // remove all entry
                queue.removeOne(cell);
            }

            cell->setRobotDist(dist);
            cell->setRobot(baseCell->robot());

            // flag open and queue
            cell->setPathState(Cell::PathOpen);
            queue.append(cell);
        }
    }

    // cleanup again
	foreach (Cell* cell, dirtyCells) {
		if (!robotsInNetwork(*cell, radius))
				if (cell->state() == (Cell::Unknown))
					cell->setRobot(0);
        cell->setPathState(Cell::PathNone);
    }

//     qDebug() << "computeVoronoiPartition took " << time.elapsed() << "milli seconds";
}

void GridMap::exportToTikz(QTikzPicture& tp)
{
    const bool showVectorField = Config::self()->showVectorField();
    const bool showDensity = Config::self()->showDensity();

    QSizeF mapSize(size());
    mapSize *= m_resolution;
    QRectF clipRect(QPointF(border*m_resolution, border*m_resolution), mapSize - QSizeF(2*border*m_resolution, 2*border*m_resolution));
    tp.clip(clipRect);

    // 1st round: export all explored free cells
    tp.newline();
    tp.comment("explored cells");
    if (!showDensity && showVectorField) tp.beginScope("gray");
    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[a].size(); ++b) {
            Cell& c = m_map[a][b];
            if (c.state() == (Cell::Explored | Cell::Free))
                c.exportToTikz(tp, showDensity, showVectorField);
        }
    }
    if (!showDensity && showVectorField) tp.endScope();

#if 0
    // 2nd round: all unexplored cells (not frontiers)
    tp.newline();
    tp.comment("all unexplored cells and obstacles except frontiers");
    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[a].size(); ++b) {
            Cell& c = m_map[a][b];
            if (c.state() & Cell::Unknown)
                c.exportToTikz(tp, showDensity, showVectorField);
        }
    }

    // 3nd round: all frontiers
    tp.newline();
    tp.comment("all frontiers");
    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[a].size(); ++b) {
            Cell& c = m_map[a][b];
            if (c.state() & Cell::Frontier)
                c.exportToTikz(tp, showDensity, showVectorField);
        }
    }
#else
    exportToTikzOpt(tp);
#endif

    // export partition
    tp.comment("Voronoi partition");
    if (Config::self()->showPartition() && RobotManager::self()->count() > 1) {
        for (int i = 0; i < RobotManager::self()->count(); ++i) {
            Robot* robot = RobotManager::self()->robot(i);
            if (!m_partitionMap.contains(robot))
                continue;

            const QPainterPath& path = m_partitionMap[robot];
            tp.beginScope();
            tp.clip(path);
            QString col = tp.registerColor(robot->color());
            tp.path(path, "ultra thick, draw=" + col);
            tp.endScope();
        }
    }
}

void GridMap::exportToTikzOpt(QTikzPicture& tp)
{
    // collect connecting regions
    QPainterPath unexploredRegion;
    QPainterPath exploredObstacles;
    QPainterPath unexploredObstacles;
    QPainterPath frontiers;
    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[a].size(); ++b) {
            Cell& c = m_map[a][b];
            if (c.state() & (Cell::Frontier | Cell::Unknown))
                unexploredRegion.addRect(c.rect());

            if (c.state() == (Cell::Obstacle | Cell::Explored))
                exploredObstacles.addRect(c.rect());

            if (c.state() == (Cell::Obstacle | Cell::Unknown))
                unexploredObstacles.addRect(c.rect());

            if (c.state() & Cell::Frontier)
                frontiers.addRect(c.rect());
        }
    }

    // simplify paths to reduce tikz size
    unexploredRegion = unexploredRegion.simplified();
    exploredObstacles = exploredObstacles.simplified();
    unexploredObstacles = unexploredObstacles.simplified();
    frontiers = frontiers.simplified();

    const QRectF sceneRect(0, 0, m_resolution * size().width(), m_resolution * size().height());

    // draw frontiers
    if (!frontiers.isEmpty()) {
        tp.beginScope();
            tp.clip(frontiers);
            tp.path(sceneRect, "fill=colFrontier");
        tp.endScope();
    }

    // draw unexplored obstacles
    if (!unexploredObstacles.isEmpty()) {
        tp.beginScope();
            tp.clip(unexploredObstacles);
            tp.path(sceneRect, "fill=colUnexploredObstacle");
        tp.endScope();
    }

    // draw explored obstacles
    if (!exploredObstacles.isEmpty()) {
        tp.beginScope();
            tp.clip(exploredObstacles);
            tp.path(sceneRect, "fill=colExploredObstacle");
        tp.endScope();
    }

    // draw grid for unexplored cells
    if (!unexploredRegion.isEmpty()) {
        tp.beginScope();
            tp.path(unexploredRegion, "draw=colBorder");
            tp.clip(unexploredRegion);
            tp << "\\draw[draw=colBorder] (0, 0) grid [step=" << m_resolution << "] ("
            << m_resolution * size().width() << ", "
            << m_resolution * size().height() << ");\n";
        tp.endScope();
    }
}

void GridMap::exportLegend(QTikzPicture& tp)
{
    // find maximum distance
    double maxDist = 0;
    for (int a = 0; a < m_map.size(); ++a) {
        for (int b = 0; b < m_map[a].size(); ++b) {
            Cell& c = m_map[a][b];
            if (c.state() == (Cell::Free | Cell::Explored)) {
                if (maxDist < c.frontierDist())
                    maxDist = c.frontierDist();
            }
        }
    }

    const double h = m_resolution * (size().height() - 4);
    tp.begin("yscale=-1");
    const int imax = 100;
    for (int i = 0; i < imax; i++) {
        double dist = (i * maxDist) / imax;
        QString c = tp.registerColor(Cell::densityToColor(exp(-0.5/(2*2)*dist*dist)));
        double y = h * i / imax;
        QRectF rect(0.0, y, 0.8, h / imax);
        tp.path(rect, "fill=" + c + ", draw=" + c);
    }
    tp.newline();
    tp.path(QRectF(0.0, 0.0, 0.8, h), "draw=black");
    tp.newline();
    tp << "\\node[below] at (0.4, 0) {$\\phi_\\text{max}$};\n";
    tp << "\\node[above] at (0.4, " << h << ") {$\\phi_\\text{min}$};\n";
    tp.end();
}

// kate: replace-tabs on; indent-width 4;
