#ifndef ANYTIMER_H__
#define ANYTIMER_H__

#define MAXJOBNUM   1024

// typedef void timer_st;
typedef void (*at_func_t)(void *);

int at_addjob(int sec, at_func_t funcp, void *arg, int isrepeat);

int at_canceljob(int id);

int at_waitjob(int id);

int at_pausejob();
int at_resumejob();

#endif