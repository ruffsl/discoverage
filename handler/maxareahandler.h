#ifndef MAXAREAHANDLER_H
#define MAXAREAHANDLER_H


#include "toolhandler.h"

#include <QtCore/QObject>



class MaxAreaHandler : public QObject, public ToolHandler
{
    Q_OBJECT	

public:
    MaxAreaHandler(Scene* scene);

    virtual void tick();
    virtual void postProcess();
    virtual void draw(QPainter& p);    
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual QPointF gradient(Robot* robot, bool interpolate);
	
	virtual QPointF gradient(Robot* robot, const QPointF& robotPos);
	
	virtual void updateVectorField();
	
	virtual double explorationPotential(GridMap& m, QPoint position, int radius);
	
    virtual QString name() const;
	
private:
    double computeDistance(QPointF arg1, QPointF arg2);
};

#endif // MAXAREAHANDLER_H
