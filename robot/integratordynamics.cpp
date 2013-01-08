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

#include "integratordynamics.h"
#include "integratordynamicsconfigwidget.h"
#include "robotmanager.h"
#include "tikzexport.h"
#include "scene.h"
#include "gridmap.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtCore/QSettings>

#include <math.h>

IntegratorDynamics::IntegratorDynamics(Scene* scene)
    : Robot(scene)
    , m_sensingRange(3.0)
    , m_fillSensingRange(false)
{
}

IntegratorDynamics::~IntegratorDynamics()
{
    delete m_configWidget;
}

Robot::Dynamics IntegratorDynamics::type()
{
    return Robot::IntegratorDynamics;
}

RobotConfigWidget* IntegratorDynamics::configWidget()
{
    if (!m_configWidget) {
        m_configWidget = new IntegratorDynamicsConfigWidget(this);
    }

    return m_configWidget;
}

bool IntegratorDynamics::hasOrientation() const
{
    return (trajectory().size() > 1);
}

qreal IntegratorDynamics::orientation() const
{
    const int count = trajectory().size();
    if (count < 2) {
        return 0.0;
    } else {
        const QPointF lastMove = trajectory()[count - 1] - trajectory()[count - 2];
        qreal delta = atan2(lastMove.y(), lastMove.x());
        return delta;
    }
}

QPointF IntegratorDynamics::orientationVector() const
{
    const int count = trajectory().size();
    if (count < 2) {
        return QPointF(0, 0);
    } else {
        QPointF lastMove = trajectory()[count - 1] - trajectory()[count - 2];
        if (!lastMove.isNull()) {
            lastMove /= sqrt(lastMove.x()*lastMove.x() + lastMove.y()*lastMove.y());
        }
        return lastMove;
    }
}

void IntegratorDynamics::setSensingRange(qreal sensingRange)
{
    m_sensingRange = sensingRange;
}

qreal IntegratorDynamics::sensingRange() const
{
    return m_sensingRange;
}

void IntegratorDynamics::setFillSensingRange(bool fill)
{
    m_fillSensingRange = fill;
}

bool IntegratorDynamics::fillSensingRange() const
{
    return m_fillSensingRange;
}

void IntegratorDynamics::drawSensedArea(QPainter& p)
{
    QColor col(color());
    QPen pen(col, map()->resolution() * 0.3);
    p.setPen(pen);
    if (fillSensingRange()) {
        col.setAlpha(50);
        p.setBrush(col);
    } else {
        p.setBrush(Qt::NoBrush);
    }

    QPainterPath visiblePath = visibleArea(m_sensingRange);
    p.drawPath(visiblePath);
}

void IntegratorDynamics::draw(QPainter& p)
{
    p.setRenderHints(QPainter::Antialiasing, true);
    p.setPen(QPen(color(), map()->resolution() * 0.3, Qt::SolidLine));

    drawTrajectory(p);
    drawRobot(p);
    drawSensedArea(p);

    p.setRenderHints(QPainter::Antialiasing, false);
}

void IntegratorDynamics::drawRobot(QPainter& p)
{
    static QPen blackPen(Qt::black);
    blackPen.setWidthF(map()->resolution() * 0.1);
    p.setOpacity(1.0);
    p.setPen(blackPen);
    p.setBrush(color());
    p.drawEllipse(position(), 0.05, 0.05);
}

void IntegratorDynamics::exportToTikz(QTikzPicture& tp)
{
    const QString c = tp.registerColor(color());
    tp.comment("robot trajectory (integrator dynamics)");
    tp.line(trajectory(), "thick, draw=" + c);

    // construct path of visibility region
    tp.comment("robot sensed area");
    QPainterPath visiblePath = visibleArea(m_sensingRange);
    tp.path(visiblePath, "thick, draw=" + c + ", fill=black, fill opacity=0.2");

    tp.circle(position(), 0.05, "draw=black, fill=" + c);
}

void IntegratorDynamics::load(QSettings& config)
{
    Robot::load(config);

    setSensingRange(config.value("sensing-range", 3.0).toDouble());
    setFillSensingRange(config.value("fill-sensing-range", false).toBool());
}

void IntegratorDynamics::save(QSettings& config)
{
    Robot::save(config);

    config.setValue("sensing-range", sensingRange());
    config.setValue("fill-sensing-range", fillSensingRange());
}

void IntegratorDynamics::tick()
{
    QPointF pos = position();

    pos += scene()->toolHandler()->gradient(this, true) * scene()->map().resolution();
    setPosition(pos, true);

    bool changed = scene()->map().exploreInRadius(pos, m_sensingRange, Cell::Explored);
}

void IntegratorDynamics::reset()
{
    clearTrajectory();
}

// kate: replace-tabs on; indent-width 4;
