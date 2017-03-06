#ifndef CLOCK_H
#define CLOCK_H

class Clock
{
public:
    double pts;
    double pts_drift;
    double last_updated;
    double speed;
    int serial;
    int paused;
    int *queue_serial;

    double get_clock();
    void set_clock_at(double pts,int serial,double time);
    void set_clock(double pts,int serial);
    void set_clock_speed(double speed);
    void init_clock(int * queue_serial);
    void sync_clock_to_slave(Clock * slave);
};

#endif
