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

#include "discoveragehandler.h"
#include "scene.h"
#include "mainwindow.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>

#include <math.h>

//BEGIN DisCoverageHandler
DisCoverageHandler::DisCoverageHandler(Scene* scene)
    : ToolHandler(scene)
{
}

DisCoverageHandler::~DisCoverageHandler()
{
}

void DisCoverageHandler::draw(QPainter& p)
{
    ToolHandler::draw(p);

    highlightCurrentCell(p);

    GridMap &m = scene()->map();
    p.scale(m.scaleFactor(), m.scaleFactor());

    QPainter::RenderHints rh = p.renderHints();
    p.setRenderHints(QPainter::Antialiasing, true);
    foreach (const Path& path, m_allPaths) {

        for (int i = 0; i < path.m_path.size() - 1; ++i) {
            const QPoint& a = path.m_path[i];
            const QPoint& b = path.m_path[i+1];
            p.drawLine(m.cell(a.x(), a.y()).rect().center(), m.cell(b.x(), b.y()).rect().center());
        }
    }

    if (m_allPaths.size()) {
        const QPointF& pathPt = m_allPaths.first().m_path[0];
        const QPointF pt = m.cell(pathPt.x(), pathPt.y()).rect().center();
        p.setPen(QPen(Qt::red, 0.2));
        p.drawLine(pt, pt + QPointF(cos(m_delta), sin(m_delta)));
    }
    
    p.setRenderHints(rh, true);
}

void DisCoverageHandler::mouseMoveEvent(QMouseEvent* event)
{
    ToolHandler::mouseMoveEvent(event);

    if (event->buttons() & Qt::LeftButton) {
        updateDisCoverage();
    }
}

void DisCoverageHandler::mousePressEvent(QMouseEvent* event)
{
    mouseMoveEvent(event);
}

void DisCoverageHandler::updateDisCoverage()
{
    const QSet<Cell*>& frontiers = scene()->map().frontiers();
    m_allPaths.clear();
    GridMap&m = scene()->map();
    QPoint pt = currentCell();
    m_allPaths = m.frontierPaths(pt);
    for (int i = 0; i < m_allPaths.size(); ++i) {
        m_allPaths[i].beautify(m);
    }

    float delta = -M_PI;
    QPointF pi = m.cell(pt.x(), pt.y()).rect().center();
    float sMax = 0.0f;
    float deltaMax = 0.0f;
    while (delta < M_PI) {
        float s = 0;
        int i = 0;
        foreach (Cell* q, frontiers) {
            s += disCoverage(pi, delta, q->rect().center(), m_allPaths[i]);
            ++i;
        }

        if (s > sMax) {
            sMax = s;
            deltaMax = delta;
        }
//         qDebug() << s;
        delta += 0.1f;
    }

    qDebug() << deltaMax << sMax;
    m_delta = deltaMax;
}

float DisCoverageHandler::disCoverage(const QPointF& pos, float delta, const QPointF& q, const Path& path)
{
    if (path.m_path.size() < 2) {
        return 0.0f;
    }

    const float theta = 0.5;
    const float sigma = 2.0;

    const QPoint& p1 = path.m_path[0];
    const QPoint& p2 = path.m_path[1];

    const float dx = p2.x() - p1.x();
    const float dy = p2.y() - p1.y();

    float alpha = - delta + atan2(dy, dx);

    if (alpha > M_PI) alpha -= 2 * M_PI;
    else if (alpha < -M_PI) alpha += 2 * M_PI;

    float len = path.m_length*0.2;

    return exp(- alpha*alpha/(2.0*theta*theta)
               - len*len/(2.0*sigma*sigma));
}

#if 0
float DisCoverageHandler::disCoverage(const QPointF& pos, float delta, const QPointF& q, const Path& path)
{
    const float theta = 0.5;
    const float sigma = 2.0;

    const float dx = q.x() - pos.x();
    const float dy = q.y() - pos.y();

    float alpha = - delta + atan2(dy, dx);

    if (alpha > M_PI) alpha -= 2 * M_PI;
    else if (alpha < -M_PI) alpha += 2 * M_PI;

    float len = path.m_length*0.2;

    return exp(- alpha*alpha/(2.0*theta*theta)
               - len*len/(2.0*sigma*sigma));
}
#endif
//END DisCoverageHandler

// kate: replace-tabs on; indent-width 4;
