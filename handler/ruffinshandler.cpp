#include "ruffinshandler.h"

#include "robot.h"
#include "scene.h"

#include <iostream>
#include <cmath>

#include <QtCore/QList>

RuffinsHandler::RuffinsHandler(Scene* scene): QObject(), ToolHandler(scene), grad(1, 0)
{
	srand(42);
}

void RuffinsHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);
}

void RuffinsHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);
}

void RuffinsHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void RuffinsHandler::tick()
{
    ToolHandler::tick();
}

void RuffinsHandler::postProcess()
{
    ToolHandler::postProcess();
	
    // update the frontier cache
    scene()->map().updateRobotFrontierCache();
}

QPointF RuffinsHandler::gradient(Robot* robot, bool interpolate)
{
    GridMap& m = *robot->map();
	
	if (rand() % 10) {
		QPoint pos = m.worldToIndex(robot->position()) + grad.toPoint();
		if (! m.cell(pos).isObstacle()) {
			return grad;
		}
	}
	
    while (1) {

		double x = rand() - RAND_MAX / 2;
		double y = rand() - RAND_MAX / 2;
		std::cout << x << " : " << y << std::endl;
		double scale = sqrt(x * x + y * y);
		QPointF newGrad(x / scale , y / scale);
		robot->position();

        //m = *robot->map();
		QPoint pos = m.worldToIndex(robot->position()) + newGrad.toPoint();

		if (! m.cell(pos).isObstacle()) {
			grad = newGrad;
			return newGrad;
		}
	}
    return QPointF();
}

double RuffinsHandler::explorationPotential(GridMap& m, QPoint position, int radius)
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
}
