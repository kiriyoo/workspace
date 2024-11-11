#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "anytimer.h"

void func1(void *p)
{
    printf("func1():%s\n",(char *)p);
    return;
}
void func2(void *p)
{
    printf("func2():%s\n",(char *)p);
    return;
}

int main()
{
    printf("begin!\n");
    at_addjob(7,func1,"aaa",0);
    at_addjob(2,func2,"bbb",1);
    at_addjob(5,func1,"ccc",0);
    printf("end!\n");

    while (1)
    {
        write(1,".",1);
        pause();
    }

    exit(0);
}