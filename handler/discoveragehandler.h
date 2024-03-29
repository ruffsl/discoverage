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

#ifndef DISCOVERAGE_HANDLER_H
#define DISCOVERAGE_HANDLER_H

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
class OrientationPlotter;
class DisCoverageBulloHandler;

namespace Ui { class DisCoverageWidget; }

class DisCoverageHandler : public QObject, public ToolHandler
{
    Q_OBJECT

    public:
        DisCoverageHandler(Scene* scene, DisCoverageBulloHandler* centroidalSearch);
        virtual ~DisCoverageHandler();

        double disCoverage(const QPointF& pos, double delta, const QPointF& q, const Path& path);

        void setOpeningAngleStdDeviation(double theta);
        double openingAngleStdDeviation() const;

        void setAutoAdaptDistanceStdDeviation(bool autoAdapt);
        bool autoAdaptDistanceStdDeviation() const;

        void setFollowLocalOptimum(bool localOptimum);
        bool followLocalOptimum() const;

        void setDistanceStdDeviation(double sigma);
        double distanceStdDeviation() const;

    public:
        virtual void draw(QPainter& p);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void toolHandlerActive(bool activated);
        virtual void reset();
        virtual void tick();
        virtual void postProcess();

        virtual void load(QSettings& config);
        virtual void save(QSettings& config);

        virtual QPointF gradient(Robot* robot, bool interpolate);
        virtual QString name() const;

    private Q_SLOTS:
        void updateVectorField();
        void updateVectorField(Robot* robot);
        void updateParameters();

    private:
        QDockWidget* dockWidget();
        QPointF interpolatedGradient(Robot* robot);
        QPointF gradient(Robot* robot, const QPointF& robotPos, const double* startOrientation = 0, bool adjustDistanceComponent = false);

    private:
        QDockWidget* m_dock;
        Ui::DisCoverageWidget* m_ui;
        OrientationPlotter* m_plotter;

        DisCoverageBulloHandler* m_centroidalSearch;
};

class OrientationPlotter : public QFrame
{
    Q_OBJECT

    public:
        OrientationPlotter(DisCoverageHandler* handler, QWidget* parent = 0);
        virtual ~OrientationPlotter();

        void setCurrentOrientation(const QPointF& currentOrientation);

    public slots:
        void updatePlot(Robot* robot);

    protected:
        virtual void paintEvent(QPaintEvent* event);
        void contextMenuEvent(QContextMenuEvent * event);

    private:
        DisCoverageHandler* m_handler;
        QVector<QPointF> m_data;
        QPointF m_currentOrientation;
};

#endif // DISCOVERAGE_HANDLER_H

// kate: replace-tabs on; indent-width 4;
