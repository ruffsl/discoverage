#include "robotstats.h"

#include "gridmap.h"
#include "scene.h"

RobotStats::RobotStats(Robot* robot)
    : m_robot(robot)
    , m_itUnemployed(0)
{
}

void RobotStats::reset()
{
    m_itUnemployed = 0;
}

void RobotStats::tick()
{
    if (! Scene::self()->map().hasFrontiers(m_robot)) {
        ++m_itUnemployed;
    }
}

bool RobotStats::isUnemployed() const
{
    return ! Scene::self()->map().hasFrontiers(m_robot);
}

// kate: replace-tabs on; indent-width 4;
