#ifndef RUFFINSHANDLER_H
#define RUFFINSHANDLER_H

#include "toolhandler.h"

#include <QtCore/QObject>


class RuffinsHandler : public QObject, public ToolHandler
{
    Q_OBJECT	

public:
    RuffinsHandler(Scene* scene);

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

#endif // RUFFINSHANDLER_H
