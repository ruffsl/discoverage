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

#include "unicycle.h"
#include "unicycleconfigwidget.h"
#include "robotmanager.h"
#include "tikzexport.h"
#include "scene.h"
#include "gridmap.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtCore/QSettings>

#include <math.h>

Unicycle::Unicycle(Scene* scene)
    : Robot(scene)
    , m_sensingRange(3.0)
    , m_fillSensingRange(false)
{
}

Unicycle::~Unicycle()
{
    delete m_configWidget;
}

Robot::Dynamics Unicycle::type()
{
    return Robot::Unicycle;
}

RobotConfigWidget* Unicycle::configWidget()
{
    if (!m_configWidget) {
        m_configWidget = new UnicycleConfigWidget(this);
    }

    return m_configWidget;
}

void Unicycle::setOrientation(double radian)
{
    if (radian > M_PI) radian -= 2 * M_PI;
    if (radian < M_PI) radian += 2 * M_PI;

    m_orientation = radian;
}

bool Unicycle::hasOrientation() const
{
    return true;
}

qreal Unicycle::orientation() const
{
    return m_orientation;
}

QPointF Unicycle::orientationVector() const
{
    return QPointF(cos(m_orientation), sin(m_orientation));
}

void Unicycle::setSensingRange(qreal sensingRange)
{
    m_sensingRange = sensingRange;
}

qreal Unicycle::sensingRange() const
{
    return m_sensingRange;
}

void Unicycle::clearTrajectory()
{
    m_trajectory.clear();
}

void Unicycle::setFillSensingRange(bool fill)
{
    m_fillSensingRange = fill;
}

bool Unicycle::fillSensingRange() const
{
    return m_fillSensingRange;
}

void Unicycle::drawSensedArea(QPainter& p)
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

void Unicycle::draw(QPainter& p)
{
    p.setRenderHints(QPainter::Antialiasing, true);

    p.setPen(QPen(color(), map()->resolution() * 0.3, Qt::SolidLine));

    // draw trajectory
    if (m_trajectory.size()) p.drawPolyline(&m_trajectory[0], m_trajectory.size());

    drawRobot(p);
    drawSensedArea(p);

    p.setRenderHints(QPainter::Antialiasing, false);
}

void Unicycle::drawRobot(QPainter& p)
{
    static QPen blackPen(Qt::black);
    static QBrush blackBrush(Qt::black);
    blackPen.setWidthF(map()->resolution() * 0.1);

    p.save();
    p.translate(position());
    p.rotate(m_orientation * 180 / M_PI);

    p.setOpacity(1.0);
    p.setPen(blackPen);
    p.setBrush(color());

    // draw wheels
    QRectF wheel(-0.1, -0.15, 0.2, 0.1);
    p.drawRect(wheel);
    wheel.moveTo(-0.1,  0.05);
    p.drawRect(wheel);

    p.setPen(blackPen);
    p.setBrush(color());

    // draw triangle
    QPainterPath triangle;
    triangle.moveTo(0.15, 0);
    triangle.lineTo(-0, -0.15);
    triangle.lineTo(-0, 0.15);
    triangle.closeSubpath();
    p.drawPath(triangle);

    p.restore();
}

void Unicycle::exportToTikz(QTikzPicture& tp)
{
    const QString c = tp.registerColor(color());
    tp.comment("robot trajectory");
    tp.line(m_trajectory, "thick, draw=" + c);

    // construct path of visibility region
    tp.comment("robot sensed area");
    QPainterPath visiblePath = visibleArea(m_sensingRange);
    tp.path(visiblePath, "thick, draw=" + c + ", fill=black, fill opacity=0.2");

    tp.circle(position(), 0.05, "draw=black, fill=" + c);
}

void Unicycle::load(QSettings& config)
{
    setPosition(config.value("position", QPointF(0.0, 0.0)).toPointF());
    setOrientation(config.value("orientation", 0.0).toDouble());
    setSensingRange(config.value("sensing-range", 3.0).toDouble());
    setFillSensingRange(config.value("fill-sensing-range", false).toBool());
}

void Unicycle::save(QSettings& config)
{
    config.setValue("position", position());
    config.setValue("orientation", orientation());
    config.setValue("sensing-range", sensingRange());
    config.setValue("fill-sensing-range", fillSensingRange());
}

void Unicycle::tick()
{
    QPointF pos = position();

    if (m_trajectory.size() == 0) {
        m_trajectory.append(pos);
    }

    QPointF grad = scene()->toolHandler()->gradient(this, true);
    if (!grad.isNull()) {

        double delta = - m_orientation + atan2(grad.y(), grad.x());

        if (delta > M_PI) delta -= 2 * M_PI;
        else if (delta < -M_PI) delta += 2 * M_PI;

        m_orientation += delta / 4.0;

        pos += QPointF(cos(m_orientation), sin(m_orientation)) * scene()->map().resolution();
        setPosition(pos);
        m_trajectory.append(pos);
    }

    bool changed = scene()->map().exploreInRadius(pos, m_sensingRange, Cell::Explored);
}

void Unicycle::reset()
{
    clearTrajectory();
}

// kate: replace-tabs on; indent-width 4;