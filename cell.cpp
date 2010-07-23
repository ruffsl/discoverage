#include "scene.h"

#include <QtGui/QPainter>
#include <QtGui/QBrush>
#include <QtCore/QDataStream>

Cell::Cell()
    : m_pathState(PathNone)
    , m_pathParent(-1)
    , m_rect()
    , m_state(static_cast<State>(Free | Unknown))
    , m_costF(0.0)
    , m_costG(0.0)
{
}

Cell::Cell(const QRectF& rect)
    : m_pathState(PathNone)
    , m_pathParent(-1)
    , m_rect(rect)
    , m_state(static_cast<State>(Free | Unknown))
    , m_costF(0.0)
    , m_costG(0.0)
{
}

void Cell::setRect(const QRectF& rect)
{
    m_rect = rect;
}

void Cell::setIndex(const QPoint& index)
{
    m_index = index;
}

const QPoint& Cell::index() const
{
    return m_index;
}

void Cell::draw(QPainter& p)
{
    QBrush sBrushFrontier = QBrush(QColor(255, 127, 0, 255));
    QBrush sBrushExplored = QBrush(QColor(255, 255, 255, 0));

    QBrush sBrushObstacle = QBrush(QColor(127, 127, 127));
    QBrush sBrushFree = QBrush(QColor(255, 255, 255));

    p.setPen(Qt::NoPen);
    if (m_state & Free) {
        p.fillRect(m_rect, sBrushFree);
    } else {
        p.fillRect(m_rect, sBrushObstacle);
    }

    if (m_state & Unknown) {
        p.setPen(Qt::gray);
        p.drawLine(m_rect.topLeft(), m_rect.bottomRight());
        p.drawLine(m_rect.bottomLeft(), m_rect.topRight());
    } else if (m_state & Frontier) {
        p.fillRect(m_rect, sBrushFrontier);
    } else { // m_state & Explored
        p.fillRect(m_rect, sBrushExplored);
    }
}

const QRectF& Cell::rect() const
{
    return m_rect;
}

void Cell::setState(State newState)
{
    if (newState & 0x3) {
        m_state = (m_state & 0x1C) | newState; // 1C = 28 = 111|00
        if ((newState & Obstacle) && (m_state & Frontier)) {
            m_state = (m_state & 0x3) | Unknown;
        }
    }

    if (newState & 0x1C) {
        if ((newState & Frontier) && (m_state & Obstacle)) {
            m_state = (m_state & 0x3) | Explored;
        } else {
            m_state = (m_state & 0x3) | newState;
        }
    }
}

QDataStream& Cell::load(QDataStream& ds)
{
    qint32 s;
    QRectF r;
    
    ds >> s;
    ds >> r;
    
    setState((Cell::State)s);
    setRect(r);
    
    return ds;
}

QDataStream& Cell::save(QDataStream& ds)
{
    ds << qint32(state());
    ds << rect();
    
    return ds;
}

// kate: replace-tabs on; indent-width 4;
