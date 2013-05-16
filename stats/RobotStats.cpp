#include "RobotStats.h"

#include "robotmanager.h"

Stats::Stats()
    : QObject()
{
    connect(RobotManager::self(), SIGNAL(robotCountChanged()), this, SLOT(updateRobotCount()));
}

Stats::~Stats()
{
}

void Stats::tick()
{
}

void Stats::updateRobotCount()
{
    for (int i = 0; i < RobotManager::self()->count(); ++i) {
        for (int i =
        m_robotStats
    }
}



class RobotStats
{
    public:
        RobotStats(Robot* robot);

        Robot* robot();

        void reset();

        void tick();

    private:
        Robot* robot;

        int m_itUnemployed; // number of iterations with no frontiers in the cell
};

#endif // ROBOT_STATS_H

// kate: replace-tabs on; indent-width 4;
