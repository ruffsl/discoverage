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

#ifndef DISCOVERAGE_ROBOT_LIST_VIEW_H
#define DISCOVERAGE_ROBOT_LIST_VIEW_H

#include <QtGui/QScrollArea>
#include <QtCore/QVector>

class RobotWidget;
class QBoxLayout;

class RobotListView : public QScrollArea
{
    Q_OBJECT

    public:
        RobotListView(QWidget* parent = 0);
        virtual ~RobotListView();

    public slots:
        void updateList();

    private:
        QBoxLayout* m_robotLayout;
        QVector<RobotWidget*> m_robotItems;
};

#endif // DISCOVERAGE_ROBOT_LIST_VIEW_H

// kate: replace-tabs on; indent-width 4;
