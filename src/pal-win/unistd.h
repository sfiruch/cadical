#ifndef _unistd_h_INCLUDED
#define _unistd_h_INCLUDED

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <process.h>
#include <signal.h>

#define __PRETTY_FUNCTION__ __FUNCSIG__
#define __builtin_prefetch(x, rw, level) PreFetchCacheLine(level, x)
#define _CRT_NONSTDC_NO_WARNINGS

#define popen _popen
#define pclose _pclose
#define R_OK 04
#define W_OK 02


#define S_ISDIR(x) (((x)&_S_IFDIR)!=0)

typedef struct timeval {
	long tv_sec;
	long tv_usec;
} TIMEVAL, * PTIMEVAL, * LPTIMEVAL;

int gettimeofday(struct timeval* tp, struct timezone* tzp);

struct rusage {
	struct timeval ru_utime; /* user CPU time used */
	struct timeval ru_stime; /* system CPU time used */
	long   ru_maxrss;        /* maximum resident set size */
	long   ru_ixrss;         /* integral shared memory size */
	long   ru_idrss;         /* integral unshared data size */
	long   ru_isrss;         /* integral unshared stack size */
	long   ru_minflt;        /* page reclaims (soft page faults) */
	long   ru_majflt;        /* page faults (hard page faults) */
	long   ru_nswap;         /* swaps */
	long   ru_inblock;       /* block input operations */
	long   ru_oublock;       /* block output operations */
	long   ru_msgsnd;        /* IPC messages sent */
	long   ru_msgrcv;        /* IPC messages received */
	long   ru_nsignals;      /* signals received */
	long   ru_nvcsw;         /* voluntary context switches */
	long   ru_nivcsw;        /* involuntary context switches */
};

#define RUSAGE_SELF 1337

int getrusage(int who, struct rusage* usage);


#define _SC_PAGESIZE 1338
long sysconf(int name);

#undef min
#undef IGNORE

void pal_init();

#define SIGALRM 1339
#define SIGBUS 1340
_crt_signal_t pal_signal(int sig, _crt_signal_t);
#define signal(a,b) pal_signal(a,b)
unsigned int alarm(unsigned int seconds);

#define DllExport   __declspec( dllexport )

#endif
