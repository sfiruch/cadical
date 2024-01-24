#ifndef _unistd_h_INCLUDED
#define _unistd_h_INCLUDED

#define WIN32_LEAN_AND_MEAN
#define __WIN32

#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <process.h>
#include <signal.h>
#include <immintrin.h>

#define __PRETTY_FUNCTION__ __FUNCSIG__
#define __builtin_prefetch(x, rw, level) PreFetchCacheLine(level, x)
#define _CRT_NONSTDC_NO_WARNINGS

#ifdef __AVX2__

#define __builtin_clz _lzcnt_u32
#define __builtin_clzl _lzcnt_u32
#define __builtin_clzll _lzcnt_u64

#else

static inline int __builtin_clz (unsigned x) {
  unsigned long index;
  return (int)(_BitScanReverse (&index, (unsigned long) x)?31 - index : 32);
}

static inline int __builtin_clzl (unsigned x) {
  unsigned long index;
  return (int) (_BitScanReverse (&index, (unsigned long) x) ? 31 - index : 32);
}

static inline int __builtin_clzll (unsigned long long x) {
  unsigned long index;
  return (int) (_BitScanReverse64 (&index, (unsigned long long) x) ? 63 - index : 64);
}

#endif

#define popen _popen
#define pclose _pclose
#define R_OK 04
#define W_OK 02


#define S_ISDIR(x) (((x)&_S_IFDIR)!=0)
#define S_ISFIFO(x) (((x) &S_IFMT)==0x1000)

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
#define _SC_NPROCESSORS_ONLN 1339
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
