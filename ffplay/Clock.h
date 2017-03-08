#ifndef CLOCK_H
#define CLOCK_H

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

/*时钟差超过此值将不会进行同步矫正*/
#define AV_NOSYNC_THRESHOLD 10.0

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
