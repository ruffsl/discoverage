#ifndef ROBOT_STATS_H
#define ROBOT_STATS_H

#include <QObject>

class RobotStats;

class Stats : public QObject
{
    Q_OBJECT

    public:
        Stats();
        ~Stats();

        void tick();

    public Q_SLOTS:
        void updateRobotCount();

    private:
        QVector<RobotStats*> m_robotStats;
};

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
