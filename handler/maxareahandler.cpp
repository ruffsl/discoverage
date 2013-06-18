#include "maxareahandler.h"
#include "robot.h"
#include "scene.h"
#include "config.h"
#include "robotmanager.h"

#include <iostream>
#include <QtCore/QList>
#include <cmath>

MaxAreaHandler::MaxAreaHandler(Scene* scene): QObject(), ToolHandler(scene)
{
}

void MaxAreaHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);
}

void MaxAreaHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);
}

void MaxAreaHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void MaxAreaHandler::tick()
{
    ToolHandler::tick();
}

QString MaxAreaHandler::name() const
{
    return QString("MaxArea");
}

void MaxAreaHandler::postProcess()
{
    ToolHandler::postProcess();
	
    // update the frontier cache
    scene()->map().updateRobotFrontierCache();
	
	if (Config::self()->showVectorField()) {
        // updateVectorField();
    }
    
    // redraw pixmap cache
    scene()->map().updateCache();
}

void MaxAreaHandler::updateVectorField() {
    const int dx = scene()->map().size().width();
    const int dy = scene()->map().size().height();
    const QList<Cell*> frontiers = scene()->map().frontiers(0);

    // iterate over all free explored cells
    for (int a = 0; a < dx; ++a) {
		GridMap &m = scene()->map();
        for (int b = 0; b < dy; ++b) {
            Cell& c = m.cell(a, b);

            if (c.state() != (Cell::Explored | Cell::Free))
                continue;

            c.setGradient(gradient(0, c.center()));
        }
    }
}

QPointF MaxAreaHandler::gradient(Robot* robot, bool interpolate)
{
	return gradient(robot, robot->position());
}

QPointF MaxAreaHandler::gradient(Robot* robot, const QPointF& robotPos) {
	if (!robot) {
		robot = RobotManager::self()->activeRobot();
	}
    GridMap& m = Scene::self()->map();
	double max = 0, x;
	QPoint cell;
	QList<Cell*> front = m.frontiers(robot);
	
	/* include explored area?
	for (int i = 0; i < m.size().width(); ++i) {
		for (int j = 0; j < m.size().height(); ++j) {
			if ((m.cell(i, j).state() & (Cell::Free | Cell::Explored)) == (Cell::Free | Cell::Explored)) {
				double dist = computeDistance(m.cell(i, j).center(), robotPos);
				if (dist > 1) {
					front.append(&m.cell(i, j));
				}
			}
		}
	}
	*/
	/*
	QVector<Cell*> visibleArea = m.visibleCells(robot, robot->sensingRange());
	for (int i = 0; i < visibleArea.size(); ) {
		if (visibleArea[i]->isObstacle()) {
			visibleArea.remove(i);
		}
		else {
			++i;
		}
	}
	front.append(QList<Cell*>::fromVector(visibleArea));
	*/
	if (front.empty())
	{
		std::cout << "empty" << std::endl;
		return QPointF();
	}
	QPoint pt = m.worldToIndex(robotPos);
    QList<Path> allPaths = m.frontierPaths(pt, front);
	
	Path* favPath = &allPaths[0];
// 	std::cout << "###########################################################" << std::endl;
    for (int i = 0; i < allPaths.size(); ++i) {
        allPaths[i].beautify(m);
		cell = allPaths[i].m_path.back();
		double length = allPaths[i].m_length;
		int size = m.numVisibleCellsUnrestricted(m.screenToWorld(cell), robot->sensingRange());
		x = size / std::pow(length, 1.5);
// 			std::cout << "cell: [" << cell.x() << ";" << cell.y() << "] size: " << size << " length: " << length << " x: " << x << std::endl;
		if(x > max) {
			max = x;
			favPath = &allPaths[i];
		}
    }
    
    // vector field creation for path
    {
		const int dx = scene()->map().size().width();
		const int dy = scene()->map().size().height();
		GridMap &m = scene()->map();
		for (int a = 0; a < dx; ++a) {
			for (int b = 0; b < dy; ++b) {
				m.cell(a, b).setGradient(QPointF());
			}
		}
		for (int i = 1; i < favPath->m_path.size(); ++i) {
			Cell& c = m.cell(favPath->m_path[i-1]);
			QPointF cellGrad = favPath->m_path[i] - favPath->m_path[i-1];
			double length = sqrt(cellGrad.x()*cellGrad.x() + cellGrad.y()*cellGrad.y());
			QPointF cellGradNorm = cellGrad / length;
			for (int j = 1; j < length; ++j) {
				Cell& c2 = m.cell((favPath->m_path[i-1] + (j * cellGradNorm)).toPoint());
				c2.setGradient(cellGradNorm);
			}
			c.setGradient(cellGradNorm);
		}
	}
	
	QPointF grad = favPath->m_path[1] - m.worldToIndex(robotPos);
	grad = grad / sqrt(grad.x()*grad.x() + grad.y()*grad.y());
	return grad;
}

double MaxAreaHandler::explorationPotential(GridMap& m, QPoint position, int radius)
{
	double result = 0.0;
	
	if (m.isValidField(position))
	{
		return 0.0;
	}
	
	int xStart = position.x() - radius;
	int xEnd = position.x() + radius;
	int yStart = position.y() - radius;
	int yEnd = position.y() + radius;
	
	for ( int a = xStart; a <= xEnd; ++a) {
		for (int b = yStart; b <= yEnd; ++b) {
			if (m.isValidField(a, b)) {
				
			}
			else {
				
			}
		}
	}
	return result;
}

double MaxAreaHandler::computeDistance(QPointF arg1, QPointF arg2)
{
	QPointF vect = arg2 - arg1;
	return sqrt (vect.x() * vect.x() + vect.y() * vect.y());
}

