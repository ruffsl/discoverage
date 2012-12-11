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

#ifndef MINDIST_HANDLER_H
#define MINDIST_HANDLER_H

#include <QtCore/QPoint>
#include <QtCore/QObject>
#include <QtGui/QFrame>
#include "cell.h"
#include "gridmap.h"
#include "toolhandler.h"

class QMouseEvent;
class QPainter;
class Scene;

class MinDistHandler : public QObject, public ToolHandler
{
    Q_OBJECT

    public:
        MinDistHandler(Scene* scene);
        virtual ~MinDistHandler();

    public:
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void toolHandlerActive(bool activated);
        virtual void reset();
        virtual void tick();

        // compute vector field
        void updateVectorField();

        // serialization
        virtual void load(QSettings& config);
        virtual void save(QSettings& config);

    private:
        void updateMinDist(const QPointF& robotPosition);

    private:
        double m_orientation;
        QPointF m_robotPosition;
        QVector<QPointF> m_trajectory;
};

#endif // MINDIST_HANDLER_H

// kate: replace-tabs on; indent-width 4;
