#include "toolhandler.h"
#include "scene.h"
#include "mainwindow.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>

#include <math.h>

//BEGIN ToolHandler
QPoint ToolHandler::s_currentCell = QPoint(0, 0);
QPoint ToolHandler::s_mousePosition = QPoint(0, 0);
qreal ToolHandler::s_operationRadius = 1.0;

ToolHandler::ToolHandler(Scene* scene)
    : m_scene(scene)
{
}

ToolHandler::~ToolHandler()
{
}

Scene* ToolHandler::scene() const
{
    return m_scene;
}

QPoint ToolHandler::cellForMousePosition(const QPoint& mousePosition) const
{
    return QPoint(mapToCell(mousePosition.x()), mapToCell(mousePosition.y()));
}

void ToolHandler::setCurrentCell(const QPoint& cell)
{
    s_currentCell = cell;
    scene()->mainWindow()->setStatusPosition(cell + QPoint(1, 1));
}

QPoint ToolHandler::currentCell() const
{
    return s_currentCell;
}

QPoint ToolHandler::mousePosition() const
{
    return s_mousePosition;
}

void ToolHandler::setOperationRadius(double radius)
{
    s_operationRadius = radius;
}

qreal ToolHandler::operationRadius() const
{
    return s_operationRadius;
}

qreal ToolHandler::mapToMap(qreal screenPos) const
{
    return screenPos / scene()->map().scaleFactor();
}

int ToolHandler::mapToCell(qreal screenPos) const
{
    return screenPos / (scene()->map().scaleFactor() * scene()->map().resolution());
}

qreal ToolHandler::mapToScreen(qreal mapPos) const
{
    return mapPos * scene()->map().scaleFactor();
}

void ToolHandler::draw(QPainter& p)
{
}

void ToolHandler::mouseMoveEvent(QMouseEvent* event)
{
    setCurrentCell(cellForMousePosition(event->pos()));
    s_mousePosition = event->pos();
}

void ToolHandler::drawOperationRadius(QPainter& p)
{
    p.setOpacity(0.2);
    QPainter::RenderHints rh = p.renderHints();
    p.setRenderHints(QPainter::Antialiasing, true);
    p.setBrush(QBrush(Qt::blue));
    p.drawEllipse(QPointF(mousePosition()), mapToScreen(operationRadius()), mapToScreen(operationRadius()));
    p.setRenderHints(rh, true);
    p.setOpacity(1.0);
}

void ToolHandler::highlightCurrentCell(QPainter& p)
{
    if (s_currentCell.x() >= 0 && s_currentCell.x() < scene()->map().size().width() &&
        s_currentCell.y() >= 0 && s_currentCell.y() < scene()->map().size().height())
    {
        QRectF r = scene()->map().cell(s_currentCell.x(), s_currentCell.y()).rect();
        r.setWidth(r.width() * scene()->map().scaleFactor());
        r.setHeight(r.height() * scene()->map().scaleFactor());
        r.moveTo(r.left() * scene()->map().scaleFactor(), r.top() * scene()->map().scaleFactor());
        const qreal w = r.width() / 3.0;
        r.adjust(-w, -w, w, w); // draw rect a little bigger

        p.setOpacity(0.5);
        p.fillRect(r, Qt::green);
        p.drawRect(r);
        p.setOpacity(1.0);
    }   
}
//END ToolHandler        





//BEGIN RobotHandler
RobotHandler::RobotHandler(Scene* scene)
    : ToolHandler(scene)
{
}

RobotHandler::~RobotHandler()
{
}

void RobotHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);

    GridMap &m = scene()->map();
    p.scale(m.scaleFactor(), m.scaleFactor());

    foreach (const Path& path, m_allPaths) {

        for (int i = 0; i < path.m_path.size() - 1; ++i) {
            const QPoint& a = path.m_path[i];
            const QPoint& b = path.m_path[i+1];
            p.drawLine(m.cell(a.x(), a.y()).rect().center(), m.cell(b.x(), b.y()).rect().center());
/*            if (i == path.m_path.size() - 2) {
                p.scale(1/m.scaleFactor(), 1/m.scaleFactor());
                p.drawText(m.cell(b.x(), b.y()).rect().center()*m.scaleFactor(), QString("%1").arg(path.m_length, 0, 'g', 2));
                p.scale(m.scaleFactor(), m.scaleFactor());
            }*/
        }
    }

    if (m_allPaths.size()) {
        const QPointF& pathPt = m_allPaths.first().m_path[0];
        const QPointF pt = m.cell(pathPt.x(), pathPt.y()).rect().center();
        p.setPen(QPen(Qt::red, 0.2));
        p.drawLine(pt, pt + QPointF(cos(m_delta), sin(m_delta)));
    }
}

void RobotHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);

    if (event->buttons() & Qt::LeftButton) {
        updateDisCoverage();
    }
}

void RobotHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void RobotHandler::updateDisCoverage()
{
    const QSet<Cell*>& frontiers = scene()->map().frontiers();
    m_allPaths.clear();
    GridMap&m = scene()->map();
    QPoint pt = currentCell();
    foreach (Cell* q, frontiers) {
        m_allPaths.append(m.aStar(pt, q->index()));
    }

    float delta = -M_PI;
    QPointF pi = m.cell(pt.x(), pt.y()).rect().center();
    float sMax = 0.0f;
    float deltaMax = 0.0f;
    while (delta < M_PI) {
        float s = 0;
        int i = 0;
        foreach (Cell* q, frontiers) {
            s += disCoverage(pi, delta, q->rect().center(), m_allPaths[i]);
            ++i;
        }

        if (s > sMax) {
            sMax = s;
            deltaMax = delta;
        }
//         qDebug() << s;
        delta += 0.1f;
    }

    qDebug() << deltaMax << sMax;
    m_delta = deltaMax;
}

float RobotHandler::disCoverage(const QPointF& pos, float delta, const QPointF& q, const Path& path)
{
    const float theta = 0.5;
    const float sigma = 2.0;
    
    const QPoint& p1 = path.m_path[0];
    const QPoint& p2 = path.m_path[1];

    const float dx = p2.x() - p1.x();
    const float dy = p2.y() - p1.y();

    float alpha = - delta + atan2(dy, dx);

    if (alpha > M_PI) alpha -= 2 * M_PI;
    else if (alpha < -M_PI) alpha += 2 * M_PI;

    float len = path.m_length*0.2;

    return exp(- alpha*alpha/(2.0*theta*theta)
               - len*len/(2.0*sigma*sigma));
}

#if 0
float RobotHandler::disCoverage(const QPointF& pos, float delta, const QPointF& q, const Path& path)
{
    const float theta = 0.5;
    const float sigma = 2.0;

    const float dx = q.x() - pos.x();
    const float dy = q.y() - pos.y();

    float alpha = - delta + atan2(dy, dx);

    if (alpha > M_PI) alpha -= 2 * M_PI;
    else if (alpha < -M_PI) alpha += 2 * M_PI;

    float len = path.m_length*0.2;

    return exp(- alpha*alpha/(2.0*theta*theta)
               - len*len/(2.0*sigma*sigma));
}
#endif
//END RobotHandler





static bool inCircle(qreal x, qreal y, qreal radius, qreal px, qreal py)
{
    qreal dx = x - px;
    qreal dy = y - py;
    
    return (dx*dx + dy*dy) < radius*radius;
}





//BEGIN ObstacleHandler
ObstacleHandler::ObstacleHandler(Scene* scene)
    : ToolHandler(scene)
    , m_desiredState(Cell::Obstacle)
{
}

ObstacleHandler::~ObstacleHandler()
{
}

void ObstacleHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);
    highlightCurrentCell(p);
    drawOperationRadius(p);
}

void ObstacleHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);

    if (event->buttons() & Qt::LeftButton) {
        updateObstacles();
    }
}

void ObstacleHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void ObstacleHandler::setDesiredState(Cell::State desiredState)
{
    m_desiredState = desiredState;
}

void ObstacleHandler::updateObstacles()
{
    Cell::State destState = m_desiredState;
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        if (m_desiredState & Cell::Obstacle) {
            destState = Cell::Free;
        } else {
            destState = Cell::Obstacle;
        }
    }
    
    int cx = mapToCell(mousePosition().x());
    int cy = mapToCell(mousePosition().y());

    qreal x = mapToMap(mousePosition().x());
    qreal y = mapToMap(mousePosition().y());
    
    GridMap& m = scene()->map();

    int cr = operationRadius() / m.resolution();

    int xStart = qMax(0, cx - cr - 1);
    int xEnd = qMin(m.size().width() - 1, cx + cr + 1);
    
    int yStart = qMax(0, cy - cr - 1);
    int yEnd = qMin(m.size().height() - 1, cy + cr + 1);
    for (int a = xStart; a <= xEnd; ++a) {
        for (int b = yStart; b <= yEnd; ++b) {
            Cell& c = m.cell(a, b);
            if (!(c.state() & destState)) {
                const QRectF& r = c.rect();
                const qreal x1 = r.left();
                const qreal x2 = r.right();
                const qreal y1 = r.top();
                const qreal y2 = r.bottom();

                if (inCircle(x, y, operationRadius(), x1, y1) ||
                    inCircle(x, y, operationRadius(), x1, y2) ||
                    inCircle(x, y, operationRadius(), x2, y1) ||
                    inCircle(x, y, operationRadius(), x2, y2))
                {
                    m.setState(c, destState);
                    m.updateCell(a, b);
                }
            }
        }
    }
}
//END ObstacleHandler




//BEGIN ExplorationHandler
ExplorationHandler::ExplorationHandler(Scene* scene)
    : ToolHandler(scene)
    , m_desiredState(Cell::Explored)
{
}

ExplorationHandler::~ExplorationHandler()
{
}

void ExplorationHandler::setDesiredState(Cell::State desiredState)
{
    m_desiredState = desiredState;
}

void ExplorationHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);
    highlightCurrentCell(p);
    drawOperationRadius(p);
    
}

void ExplorationHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);

    if (event->buttons() & Qt::LeftButton) {
        updateExploredState();
    }
}

void ExplorationHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void ExplorationHandler::updateExploredState()
{
    Cell::State destState = m_desiredState;
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        if (m_desiredState & Cell::Unknown) {
            destState = Cell::Explored;
        } else {
            destState = Cell::Unknown;
        }
    }

    int cx = mapToCell(mousePosition().x());
    int cy = mapToCell(mousePosition().y());

    qreal x = mapToMap(mousePosition().x());
    qreal y = mapToMap(mousePosition().y());
    
    GridMap& m = scene()->map();

    int cr = operationRadius() / m.resolution();

    int xStart = qMax(0, cx - cr - 1);
    int xEnd = qMin(m.size().width() - 1, cx + cr + 1);
    
    int yStart = qMax(0, cy - cr - 1);
    int yEnd = qMin(m.size().height() - 1, cy + cr + 1);
    for (int a = xStart; a <= xEnd; ++a) {
        for (int b = yStart; b <= yEnd; ++b) {
            Cell& c = m.cell(a, b);
            if (!(c.state() & destState)) {
                const QRectF& r = c.rect();
                const qreal x1 = r.left();
                const qreal x2 = r.right();
                const qreal y1 = r.top();
                const qreal y2 = r.bottom();

                int count = 0;
                if (inCircle(x, y, operationRadius(), x1, y1)) ++count;
                if (inCircle(x, y, operationRadius(), x1, y2)) ++count;
                if (inCircle(x, y, operationRadius(), x2, y1)) ++count;
                if (inCircle(x, y, operationRadius(), x2, y2)) ++count;
                
                if (count == 4) {
                    m.setState(c, destState);
                    m.updateCell(a, b);
                } else if (count > 0) {
                    m.setState(c, Cell::Frontier);
                    m.updateCell(a, b);
                }
            }
        }
    }
}
//END ExplorationHandler

// kate: replace-tabs on; indent-width 4;
