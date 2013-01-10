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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

class QPoint;
class QLabel;
class Scene;
class Statistics;

namespace Ui {
    class ToolWidget;
}

class MainWindow : public QMainWindow, protected Ui::MainWindow
{
    Q_OBJECT
    public:
        MainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
        virtual ~MainWindow();

        void setStatusPosition(const QPoint& pos);
        void setStatusResolution(qreal resolution);
        void updateExplorationProgress();

        Scene* scene() const;

    public slots:
        void loadScene(const QString& filename);
        void newScene();
        void openScene();
        void reloadScene();
        void saveScene();
        void saveSceneAs();

        void tick();

        void exportToTikz();

    protected:
        virtual void keyPressEvent(QKeyEvent* event);

        // after loading, set states of toggle QActions correctly
        void updateActionState();

    private:
        Ui::ToolWidget* m_toolsUi;

        QLabel* m_statusProgress;
        QLabel* m_statusPosition;
        QLabel* m_statusResolution;

        Scene* m_scene;
        QString m_sceneFile;
        Statistics* m_stats;
};

#endif // MAINWINDOW_H

// kate: replace-tabs on; indent-width 4;
