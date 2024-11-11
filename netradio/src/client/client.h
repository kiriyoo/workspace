#ifndef CLIENT_H__
#define CLIENT_H__

#define DEFAULT_PLAYERCMD   "/usr/local/bin/mpg123 > /device/null"
struct client_conf_st
{
    char *rcvport;
    char *mgroup;
    char *player_cmd;
};

// 如果变量可能在其他.c中被使用的话，则需要在.h中声明，并用extern修饰
extern struct client_conf_st client_conf;
#endif