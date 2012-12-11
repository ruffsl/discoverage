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

#ifndef DISCOVERAGE_BULLO_HANDLER_H
#define DISCOVERAGE_BULLO_HANDLER_H

#include <QtCore/QPoint>
#include <QtCore/QObject>
#include <QtGui/QFrame>
#include "cell.h"
#include "gridmap.h"
#include "toolhandler.h"

class QMouseEvent;
class QPainter;
class Scene;
class QDockWidget;

namespace Ui { class DisCoverageBulloWidget; }

class DisCoverageBulloHandler : public QObject, public ToolHandler
{
    Q_OBJECT

    public:
        DisCoverageBulloHandler(Scene* scene);
        virtual ~DisCoverageBulloHandler();

    public:
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void toolHandlerActive(bool activated);
        virtual void reset();
        virtual void tick();

        // update vector field for all explored cells
        void updateVectorField();

        // serialization
        virtual void load(QSettings& config);
        virtual void save(QSettings& config);

    private Q_SLOTS:
//         void showVectorField(bool show);
        void updateParameters();

    private:
        QPointF gradient(const QPointF& robotPos);
        qreal performance(const QPointF& p, const QPointF& q);
        qreal fitness(const QPointF& robotPos, const QVector<Cell*>& cells);

        void updateDisCoverage(const QPointF& robotPosition);
        double disCoverage(const QPointF& pos, double delta, const QPointF& q, const Path& path);

        QDockWidget* dockWidget();

    private:
        QDockWidget* m_dock;
        Ui::DisCoverageBulloWidget* m_ui;
 
        QVector<QVector<QLineF> > m_vectorField;

        QPointF m_robotPosition;
        double m_visionRadius;
        QPointF m_gradient;
        QVector<QPointF> m_trajectory;
};

#endif // DISCOVERAGE_BULLO_HANDLER_H

// kate: replace-tabs on; indent-width 4;
