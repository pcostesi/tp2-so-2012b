#ifndef _SIGNAL_H
#define _SIGNAL_H

/* Standard signal numbers. */
#define SIGHUP             1    /* hangup */
#define SIGINT             2    /* interrupt (DEL) */
#define SIGQUIT            3    /* quit (ASCII FS) */
#define SIGILL             4    /* illegal instruction */
#define SIGTRAP            5    /* trace trap (not reset when caught) */
#define SIGABRT            6    /* IOT instruction */
#define SIGBUS             7    /* bus error */
#define SIGFPE             8    /* floating point exception */
#define SIGKILL            9    /* kill (cannot be caught or ignored) */
#define SIGUSR1           10    /* user defined signal # 1 */
#define SIGSEGV           11    /* segmentation violation */
#define SIGUSR2           12    /* user defined signal # 2 */
#define SIGPIPE           13    /* write on a pipe with no one to read it */
#define SIGALRM           14    /* alarm clock */
#define SIGTERM           15    /* software termination signal from kill */
#define SIGEMT            16    /* EMT instruction */
#define SIGCHLD           17    /* child process terminated or stopped */
#define SIGWINCH          21    /* window size has changed */
#define SIGCONT           18    /* continue if stopped */
#define SIGSTOP           19    /* stop signal */
#define SIGTSTP           20    /* interactive stop signal */
#define SIGTTIN           22    /* background process wants to read */
#define SIGTTOU           23    /* background process wants to write */
#define SIGVTALRM         24    /* virtual alarm */
#define SIGPROF           25    /* profiler alarm */
#endif