#ifndef RANDOMHANDLER_H
#define RANDOMHANDLER_H

#include "toolhandler.h"

#include <QtCore/QObject>


class RandomHandler : public QObject, public ToolHandler
{
    Q_OBJECT	

public:
    RandomHandler(Scene* scene);

    virtual void tick();
    virtual void postProcess();
    virtual void draw(QPainter& p);    
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual QPointF gradient(Robot* robot, bool interpolate);

private:
	QPointF grad;
	
	virtual double explorationPotential(GridMap& m, QPoint position, int radius);
};

#endif // RANDOMHANDLER_H
