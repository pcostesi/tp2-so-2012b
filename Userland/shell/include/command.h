#ifndef __COMMANDS_H_
#define __COMMANDS_H_ 1
#include <shell.h>

#define INVALID_DATE "Invalid date inserted, please respect dd/mm/yy format & insert a valid date.\n"
#define INVALID_TIME "No arguments were sent to command settime format musut be ss:mm:hh \n"
#define KILL_ERROR "The id of the process to be killed must be inserted. Format: \"kill id\".\n"

int echo(char**, int);
int clear(char**, int);
int date(char**, int);
int time(char**, int);
int set_date(char**, int);
int set_time(char**, int);
int parse_date(char*, int*, int*, int*);
int parse_time(char*, int*, int*, int*);
int is_num(char);
int valid_time(int, int, int);
int valid_date(int, int, int);
int is_leap_year(int);
int getchar_cmd(char**, int);
int printf_cmd(char**, int);
int scanf_cmd(char**, int);
int reset_vect(char vec[]);
int help_error_print();
int help(char**, int); 
int halt_system(char** args, int argc);
int commands(char** args, int argc);
int print_ascii_table(char** args, int argc);
int setcolor(char** args, int argc);
int kill_cmd(char** args, int argc);
int ps_cmd(char** args, int argc);
int s_to_i(char *string);
#endif