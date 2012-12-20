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

#include "robotmanager.h"
#include "robot.h"
#include "scene.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtCore/QSettings>

RobotManager* RobotManager::s_self = 0;

RobotManager* RobotManager::self()
{
    return s_self;
}

RobotManager::RobotManager(QObject* parent)
    : QObject(parent)
    , m_activeRobot(0)
{
    s_self = this;
}

RobotManager::~RobotManager()
{
    s_self = 0;
}

void RobotManager::addRobot()
{
    Robot* robot = new Robot(Scene::self());
    m_robots.append(robot);

    if (m_robots.size() == 1)
        setActiveRobot(robot);

    emit robotCountChanged();
}

void RobotManager::removeRobot()
{
    if (m_robots.size() > 0)
        removeRobot(m_robots.last());
}

bool RobotManager::removeRobot(Robot* robot)
{
    const int index = m_robots.indexOf(robot);
    if (index == -1)
        return false;

    m_robots.remove(index);
    delete robot;

    // take care of active robot
    if (robot == m_activeRobot) {
        setActiveRobot(m_robots.size() ? m_robots[0] : 0);
    }

    emit robotCountChanged();

    return index != -1;
}

Robot* RobotManager::activeRobot()
{
    return m_activeRobot;
}

void RobotManager::setActiveRobot(Robot* robot)
{
    if (m_activeRobot == robot)
        return;

    m_activeRobot = robot;

    emit activeRobotChanged(m_activeRobot);
}

int RobotManager::count() const
{
    return m_robots.size();
}

int RobotManager::indexOf(Robot* robot) const
{
    return m_robots.indexOf(robot);
}

Robot* RobotManager::robot(int index) const
{
    return m_robots[index];
}

void RobotManager::setSensingRange(qreal sensingRange)
{
    foreach (Robot* robot, m_robots) {
        robot->setSensingRange(sensingRange);
    }
}

void RobotManager::clearTrajectory()
{
    foreach (Robot* robot, m_robots) {
        robot->clearTrajectory();
    }
}

void RobotManager::draw(QPainter& p)
{
    foreach (Robot* robot, m_robots) {
        robot->draw(p);
    }
}

void RobotManager::exportToTikz(QTextStream& ts)
{
    foreach (Robot* robot, m_robots) {
        robot->exportToTikz(ts);
    }
}

void RobotManager::load(QSettings& config)
{
    config.beginGroup("robots");
    const int robotCount = config.value("count", 0).toInt();
    config.endGroup();

    m_robots.clear();

    for (int i = 0; i < robotCount; ++i) {
        config.beginGroup(QString("robot-%1").arg(i));

        Robot* robot = new Robot(Scene::self());
        robot->load(config);
        m_robots.append(robot);

        config.endGroup();
    }
}

void RobotManager::save(QSettings& config)
{
    config.beginGroup("robots");
    config.setValue("count", m_robots.size());
    config.endGroup();

    for (int i = 0; i < m_robots.size(); ++i) {
        config.beginGroup(QString("robot-%1").arg(i));
        m_robots[i]->save(config);
        config.endGroup();
    }
}

// kate: replace-tabs on; indent-width 4;
