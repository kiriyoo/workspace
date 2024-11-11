OBJS=main.o anytimer.o
CC=gcc
GFLAGS+=-c -Wall -g

anytimer:$(OBJS)
	$(CC) $^ -o $@

%.o:%.c
	$(CC) $^ $(GFLAGS) -o $@

clean:
	$(RM) $(OBJS) anytimer -r