#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "anytimer.h"


enum state_timer
{
    STATE_RUNNING = 1,
    STATE_CANCELED,
    STATE_OVER,
    STATE_PAUSE
};

struct timer_st
{
    int sec;
    int time_remain;
    int repeat;
    at_func_t funcp;
    void *arg;
    enum state_timer state;
};

static struct timer_st *timerjobs[MAXJOBNUM];
static int inited = 0;

static void timer_handler(int s,siginfo_t *infop,void *unused)
{
    int i;

    if (infop->si_code != SI_KERNEL)
    {
        return;
    }

    for (i = 0; i < MAXJOBNUM; i++)
    {
        if (timerjobs[i] != NULL && timerjobs[i]->state == STATE_RUNNING)
        {
            timerjobs[i]->time_remain --;
            if (timerjobs[i]->time_remain == 0)
            {
                if (timerjobs[i]->repeat == 0)
                {
                    timerjobs[i]->funcp(timerjobs[i]->arg);
                    timerjobs[i]->state = STATE_OVER;
                }
                else
                {
                    timerjobs[i]->funcp(timerjobs[i]->arg);
                    timerjobs[i]->time_remain = timerjobs[i]->sec;
                }
            }
        }
        
    }
}
static struct sigaction sa,osa;

static void timer_unload(void);

static void timer_laod(void)
{
    struct itimerval itv;

    sigset_t set;

    sigemptyset(&set);
    sa.sa_sigaction = timer_handler;
    sa.sa_mask = set;
    sa.sa_flags = SA_SIGINFO;
    if(sigaction(SIGALRM,&sa,&osa)<0)
    {
        perror("sigaction()");
        exit(1);
    }

    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;

    if(setitimer(ITIMER_REAL,&itv,NULL)<0)
    {
        perror("setitimer()");
        exit(1);
    }
    

    atexit(timer_unload);
}

static void timer_unload(void)
{
    printf("timer_unload.\n");
    int i;
    struct itimerval itv;

    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = 0; 

    if(sigaction(SIGALRM,&osa,NULL)<0)
    {
        perror("sigaction()");
        exit(1);
    }
    if(setitimer(ITIMER_REAL,&itv,NULL)<0)
    {
        perror("setitimer()");
        exit(1);
    }
    for (i = 0; i < MAXJOBNUM; i++)
    {
        at_canceljob(i);
    }
}

static int get_free_pos(void)
{
    int i;
    for (i = 0; i < MAXJOBNUM; i++)
    {
        
        if (timerjobs[i] == NULL)
        {
            return i;
        }
    }
    return -1;
}

int at_addjob(int sec,at_func_t funcp,void *arg,int isrepeat)
{
    if (sec < 0)
    {
        return -EINVAL;
    }
    
    int pos;
    struct timer_st *jobp;

    if (inited==0)
    {
        timer_laod();
        inited = 1;
    }

    pos = get_free_pos();
    if (pos < 0)
    {
        return -ENOSPC;
    }

    jobp = malloc(sizeof(*jobp));
    if (jobp == NULL)
    {
        return -ENOMEM;
    }
    
    jobp->funcp = funcp;
    jobp->arg = arg;
    jobp->sec = sec;
    jobp->time_remain = sec;
    jobp->repeat = isrepeat;
    jobp->state = STATE_RUNNING;
    timerjobs[pos] = jobp;
    return pos;
}

int at_canceljob(int id)
{
    if (id > MAXJOBNUM || id < 0 || timerjobs[id]==NULL)
    {
        return -EINVAL;
    }
    if (timerjobs[id]->state == STATE_OVER)
    {
        return -EBUSY;
    }
    if (timerjobs[id]->state == STATE_CANCELED)
    {
        return -ECANCELED;
    }
    if (timerjobs[id]->state == STATE_RUNNING)
    {
        timerjobs[id]->state = STATE_CANCELED;
    }
    return 0;
}

int at_waitjob(int id)
{
    if (id > MAXJOBNUM || id < 0 || timerjobs[id]==NULL)
    {
        return -EINVAL;
    }
    if (timerjobs[id]->repeat)
    {
        return -EBUSY;
    }
    
    while (timerjobs[id]->state == STATE_OVER)
    {

        pause();
    }
    if (timerjobs[id]->state == STATE_OVER || timerjobs[id]->state == STATE_CANCELED)
    {
        free(timerjobs[id]);
        timerjobs[id] = NULL;
    }
    return 0;
}