#ifndef ROBOT_STATS_H
#define ROBOT_STATS_H

class Robot;

class RobotStats
{
    public:
        RobotStats(Robot* robot);

        Robot* robot();

        // user clicked reset or loaded new scene
        void reset();

        // user invoked one simulation iteration
        void tick();

        // returns true, if the robot has not frontieres in its cell
        bool isUnemployed() const;

    private:
        Robot* m_robot;

        int m_itUnemployed; // number of iterations with no frontiers in the cell
};

#endif // ROBOT_STATS_H

// kate: replace-tabs on; indent-width 4;
