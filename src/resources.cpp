#include "internal.hpp"

/*------------------------------------------------------------------------*/

// This is operating system specific code.  We mostly develop on Linux and
// there it should be fine and mostly works out-of-the-box on MacOS too but
// Windows needs special treatment (as probably other operating systems
// too).

extern "C" {

#ifdef __WIN32

#ifndef __WIN32_WINNT
#define __WIN32_WINNT 0x0600
#endif

#include <psapi.h>
#include <windows.h>

#else

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#endif

#include <string.h>
}

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

#ifdef __WIN32

double absolute_real_time () {
  FILETIME f;
  GetSystemTimeAsFileTime (&f);
  ULARGE_INTEGER t;
  t.LowPart = f.dwLowDateTime;
  t.HighPart = f.dwHighDateTime;
  double res = (__int64) t.QuadPart;
  res *= 1e-7;
  return res;
}

double absolute_process_time () {
  double res = 0;
  FILETIME fc, fe, fu, fs;
  if (GetProcessTimes (GetCurrentProcess (), &fc, &fe, &fu, &fs)) {
    ULARGE_INTEGER u, s;
    u.LowPart = fu.dwLowDateTime;
    u.HighPart = fu.dwHighDateTime;
    s.LowPart = fs.dwLowDateTime;
    s.HighPart = fs.dwHighDateTime;
    res = (__int64) u.QuadPart + (__int64) s.QuadPart;
    res *= 1e-7;
  }
  return res;
}

#else

double absolute_real_time () {
  struct timeval tv;
  if (gettimeofday (&tv, 0))
    return 0;
  return 1e-6 * tv.tv_usec + tv.tv_sec;
}

// We use 'getrusage' for 'process_time' and 'maximum_resident_set_size'
// which is pretty standard on Unix but probably not available on Windows
// etc.  For different variants of Unix not all fields are meaningful.

double absolute_process_time () {
  double res;
  struct rusage u;
  if (getrusage (RUSAGE_SELF, &u))
    return 0;
  res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;  // user time
  res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec; // + system time
  return res;
}

#endif

double Internal::real_time () {
  return absolute_real_time () - stats.time.real;
}

double Internal::process_time () {
  return absolute_process_time () - stats.time.process;
}

/*------------------------------------------------------------------------*/

#ifdef __WIN32

uint64_t current_resident_set_size () {
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo (GetCurrentProcess (), &pmc, sizeof (pmc))) {
    return pmc.WorkingSetSize;
  } else
    return 0;
}

uint64_t maximum_resident_set_size () {
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo (GetCurrentProcess (), &pmc, sizeof (pmc))) {
    return pmc.PeakWorkingSetSize;
  } else
    return 0;
}

#else

// This seems to work on Linux (man page says since Linux 2.6.32).

uint64_t maximum_resident_set_size () {
  struct rusage u;
  if (getrusage (RUSAGE_SELF, &u))
    return 0;
  return ((uint64_t) u.ru_maxrss) << 10;
}

// Unfortunately 'getrusage' on Linux does not support current resident set
// size (the field 'ru_ixrss' is there but according to the man page
// 'unused'). Thus we fall back to use the '/proc' file system instead.  So
// this is not portable at all and needs to be replaced on other systems
// The code would still compile though (assuming 'sysconf' and
// '_SC_PAGESIZE' are available).

uint64_t current_resident_set_size () {
  char path[64];
  sprintf (path, "/proc/%" PRId64 "/statm", (int64_t) getpid ());
  FILE * file = fopen (path, "r");
  if (!file)
  {
      // Fall back to rusage, if the '/proc' file system is not found
      struct rusage u;
      if (getrusage(RUSAGE_SELF, &u)) return 0;
      return ((size_t) u.ru_idrss + (size_t) u.ru_ixrss) << 10;
  }
  uint64_t dummy, rss;
  int scanned = fscanf (file, "%" PRIu64 " %" PRIu64 "", &dummy, &rss);
  fclose (file);
  return scanned == 2 ? rss * sysconf (_SC_PAGESIZE) : 0;
}

#endif

  bool amd, intel;
  int res;

  const int syscores = sysconf (_SC_NPROCESSORS_ONLN);
  if (syscores > 0) MSG ("'sysconf' reports %d processors", syscores);
  else MSG ("'sysconf' fails to determine number of online processors");

  FILE * p = popen (
               "grep '^core id' /proc/cpuinfo 2>/dev/null|sort|uniq|wc -l",
               "r");
  int coreids;
  if (p) {
    if (fscanf (p, "%d", &coreids) != 1) coreids = 0;
    if (coreids > 0) MSG ("found %d core ids in '/proc/cpuinfo'", coreids);
    else MSG ("failed to extract core ids from '/proc/cpuinfo'");
    pclose (p);
  } else coreids = 0;

  p = popen (
        "grep '^physical id' /proc/cpuinfo 2>/dev/null|sort|uniq|wc -l",
        "r");
  int physids;
  if (p) {
    if (fscanf (p, "%d", &physids) != 1) physids = 0;
    if (physids > 0)
         MSG ("found %d physical ids in '/proc/cpuinfo'", physids);
    else MSG ("failed to extract physical ids from '/proc/cpuinfo'");
    pclose (p);
  } else physids = 0;

  int procpuinfocores;
  if (coreids > 0 && physids > 0 &&
      (procpuinfocores = coreids * physids) > 0) {
    MSG ("%d cores = %d core times %d physical ids in '/proc/cpuinfo'",
         procpuinfocores, coreids, physids);
  } else procpuinfocores = 0;

  bool usesyscores = false, useprocpuinfo = false;

  if (procpuinfocores > 0 && procpuinfocores == syscores) {
    MSG ("'sysconf' and '/proc/cpuinfo' results match");
    usesyscores = 1;
  } else if (procpuinfocores > 0 && syscores <= 0) {
    MSG ("only '/proc/cpuinfo' result valid");
    useprocpuinfo = 1;
  } else if (procpuinfocores <= 0 && syscores > 0) {
    MSG ("only 'sysconf' result valid");
    usesyscores = 1;
  } else if (procpuinfocores > 0 && syscores > 0) {
    intel = !system ("grep vendor /proc/cpuinfo 2>/dev/null|grep -q Intel");
    if (intel) MSG ("found Intel as vendor in '/proc/cpuinfo'");
    amd = !system ("grep vendor /proc/cpuinfo 2>/dev/null|grep -q AMD");
    if (amd) MSG ("found AMD as vendor in '/proc/cpuinfo'");
    assert (syscores > 0);
    assert (procpuinfocores > 0);
    assert (syscores != procpuinfocores);
    if (amd) {
      MSG ("trusting 'sysconf' on AMD");
      usesyscores = true;
    } else if (intel) {
      MSG ("'sysconf' result off by a factor of %f on Intel",
           syscores / (double) procpuinfocores);
      MSG ("trusting '/proc/cpuinfo' on Intel");
      useprocpuinfo = true;
    }  else {
      MSG ("trusting 'sysconf' on unknown vendor machine");
      usesyscores = true;
    }
  }

  if (useprocpuinfo) {
    MSG (
      "assuming cores = core * physical ids in '/proc/cpuinfo' = %d",
      procpuinfocores);
    res = procpuinfocores;
  } else if (usesyscores) {
    MSG (
      "assuming cores = number of processors reported by 'sysconf' = %d",
      syscores);
    res = syscores;
  } else {
    MSG (
      "falling back to compiled in default value of %d number of cores",
      NUM_CORES);
    res = NUM_CORES;
  }

  return res;
}

/*------------------------------------------------------------------------*/

} // namespace CaDiCaL
