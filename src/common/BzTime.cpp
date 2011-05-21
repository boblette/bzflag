/* bzflag
 * Copyright (c) 1993-2010 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// interface header
#include "BzTime.h"

// system headers
#include <time.h>
#include <string>
#include <string.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef __BEOS__
#  include <OS.h>
#endif
#if !defined(_WIN32)
#  include <sys/time.h>
#  include <sys/types.h>
static int64_t lastTime = 0;
#  ifdef HAVE_SCHED_H
#    include <sched.h>
#  endif
#else // !defined(_WIN32)
#  include <mmsystem.h>
static unsigned long int lastTime = 0;
static LARGE_INTEGER     qpcLastTime;
static LONGLONG          qpcFrequency = 0;
static LONGLONG          qpcLastCalibration;
static DWORD             timeLastCalibration;
#endif // !defined(_WIN32)

// system threading headers
#ifndef _WIN32
#  include <sys/types.h>
#  include <dirent.h>
#endif
#ifdef HAVE_PTHREADS
#  include <pthread.h>
#endif

// common headers
#include "TextUtils.h"
#include "bzfio.h"


//============================================================================//

#if defined(HAVE_PTHREADS)
static pthread_mutex_t    timer_mutex = PTHREAD_MUTEX_INITIALIZER;
# define LOCK_TIMER_MUTEX   pthread_mutex_lock(&timer_mutex);
# define UNLOCK_TIMER_MUTEX pthread_mutex_unlock(&timer_mutex);
#elif defined(_WIN32)
static CRITICAL_SECTION   timer_critical;
# define LOCK_TIMER_MUTEX   EnterCriticalSection(&timer_critical);
# define UNLOCK_TIMER_MUTEX LeaveCriticalSection(&timer_critical);
#else
# define LOCK_TIMER_MUTEX
# define UNLOCK_TIMER_MUTEX
#endif


//============================================================================//

static BzTime currentTime;
static BzTime tickTime;
static BzTime sunExplodeTime;
static BzTime sunGenesisTime;
static BzTime nullTime;
static BzTime startTime = BzTime::getCurrent();


//============================================================================//

#if !defined(_WIN32)
static inline int64_t getEpochMicroseconds() {
  struct timeval nowTime;
  gettimeofday(&nowTime, NULL);
  return (int64_t(nowTime.tv_sec) * int64_t(1000000))
         + int64_t(nowTime.tv_usec);
}
#endif


const BzTime& BzTime::getCurrent(void) {
  // a mutex lock is used because this routine
  // is called from the client's sound thread

#if !defined(_WIN32)

  LOCK_TIMER_MUTEX

  if (lastTime == 0) {
    lastTime = getEpochMicroseconds();
    currentTime += double(lastTime) * 1.0e-6; // sync with system clock
  }
  else {
    const int64_t nowTime = getEpochMicroseconds();

    const int64_t diff = (nowTime - lastTime);

    if (diff > 0) {
      // add to currentTime
      currentTime += double(diff) * 1.0e-6;
    }
    else if (diff < 0) {
      // eh, how'd we go back in time?
      logDebugMessage(5, "WARNING: went back in time %li microseconds\n",
                      (long int)diff);
    }

    lastTime = nowTime;
  }

  UNLOCK_TIMER_MUTEX

#else /* !defined(_WIN32) */

  static bool inited = false;
  if (!inited) {
    inited = true;
    InitializeCriticalSection(&timer_critical);
  }

  LOCK_TIMER_MUTEX

  if (qpcFrequency != 0) {
    // main timer is qpc
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    LONGLONG diff     = now.QuadPart - qpcLastTime.QuadPart;
    LONGLONG clkSpent = now.QuadPart - qpcLastCalibration;
    qpcLastTime = now;

    if (clkSpent > qpcFrequency) {
      // Recalibrate Frequency
      DWORD tgt    = timeGetTime();
      DWORD deltaTgt      = tgt - timeLastCalibration;
      timeLastCalibration = tgt;
      qpcLastCalibration  = now.QuadPart;
      if (deltaTgt > 0) {
        LONGLONG oldqpcfreq = qpcFrequency;
        qpcFrequency  = (clkSpent * 1000) / deltaTgt;
        if (qpcFrequency != oldqpcfreq)
          logDebugMessage(4, "Recalibrated QPC frequency.  Old: %f ; New: %f\n",
                          (double)oldqpcfreq, (double)qpcFrequency);
      }
    }

    currentTime += (double) diff / (double) qpcFrequency;
  }
  else if (lastTime != 0) {
    unsigned long int now = (unsigned long int)timeGetTime();
    unsigned long int diff;
    if (now < lastTime) {
      // eh, how'd we go back in time?
      diff = 0;
    }
    else {
      diff = now - lastTime;
    }
    currentTime += 1.0e-3 * (double)diff;
    lastTime = now;
  }
  else {
    static bool sane = true;

    // should only get into here once on app start
    if (!sane) {
      logDebugMessage(1, "Sanity check failure in BzTime::getCurrent()\n");
    }
    sane = false;

    // make sure we're at our best timer resolution possible
    timeBeginPeriod(1);

    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq)) {
      QueryPerformanceCounter(&qpcLastTime);
      qpcFrequency  = freq.QuadPart;
      logDebugMessage(4, "Actual reported QPC Frequency: %f\n", (double)qpcFrequency);
      qpcLastCalibration  = qpcLastTime.QuadPart;
      timeLastCalibration = timeGetTime();
      currentTime += 1.0e-3 * (double)timeLastCalibration; // sync with system clock
    }
    else {
      logDebugMessage(1, "QueryPerformanceFrequency failed with error %d\n", GetLastError());

      lastTime = (unsigned long int)timeGetTime();
      currentTime += 1.0e-3 * (double)lastTime; // sync with system clock
    }
  }

  UNLOCK_TIMER_MUTEX

#endif /* !defined(_WIN32) */

  return currentTime;
}


const BzTime& BzTime::getStartTime(void) { // const
  return startTime;
}


const BzTime& BzTime::getTick(void) { // const
  return tickTime;
}


void BzTime::setTick(void) {
  tickTime = getCurrent();
}


const BzTime& BzTime::getSunExplodeTime(void) {
  sunExplodeTime.seconds = 10000.0 * 365 * 24 * 60 * 60;
  return sunExplodeTime;
}


const BzTime& BzTime::getSunGenesisTime(void) {
  sunGenesisTime.seconds = -10000.0 * 365 * 24 * 60 * 60;
  return sunGenesisTime;
}


const BzTime& BzTime::getNullTime(void) {
  nullTime.seconds = 0;
  return nullTime;
}


const char* BzTime::timestamp(void) { // const
  static char buffer[256]; // static, so that it doesn't vanish
  time_t tnow = time(0);
  struct tm* now = localtime(&tnow);
  now->tm_year += 1900;
  ++now->tm_mon;

  strncpy(buffer, TextUtils::format("%04d-%02d-%02d %02d:%02d:%02d",
                                    now->tm_year, now->tm_mon, now->tm_mday,
                                    now->tm_hour, now->tm_min, now->tm_sec).c_str(), 256);
  buffer[255] = '\0'; // safety

  return buffer;
}


/** returns a short string of the local time */
//static
std::string BzTime::shortTimeStamp(void) {
  time_t tnow = time(0);
  struct tm* now = localtime(&tnow);

  std::string result(TextUtils::format("%02d:%02d", now->tm_hour, now->tm_min));
  return result;
}


void BzTime::localTime(int* year, int* month, int* day,
                       int* hour, int* min, int* sec, bool* dst) { // const
  time_t tnow = time(0);
  struct tm* now = localtime(&tnow);
  now->tm_year += 1900;
  ++now->tm_mon;

  if (year)  { *year  = now->tm_year; }
  if (month) { *month = now->tm_mon;  }
  if (day)   { *day   = now->tm_mday; }
  if (hour)  { *hour  = now->tm_hour; }
  if (min)   { *min   = now->tm_min;  }
  if (sec)   { *sec   = now->tm_sec;  }
  if (dst)   { *dst   = (now->tm_isdst != 0); }
}


void BzTime::localTimeDOW(int* year, int* month, int* day, int* wday,
                          int* hour, int* min, int* sec, bool* dst) { // const
  time_t tnow = time(0);
  struct tm* now = localtime(&tnow);
  now->tm_year += 1900;
  ++now->tm_mon;

  if (year)  { *year  = now->tm_year; }
  if (month) { *month = now->tm_mon;  }
  if (day)   { *day   = now->tm_mday; }
  if (wday)  { *wday  = now->tm_wday; }
  if (hour)  { *hour  = now->tm_hour; }
  if (min)   { *min   = now->tm_min;  }
  if (sec)   { *sec   = now->tm_sec;  }
  if (dst)   { *dst   = (now->tm_isdst != 0); }
}


void BzTime::UTCTime(int* year, int* month, int* day, int* wday,
                     int* hour, int* min, int* sec, bool* dst) { // const
  time_t tnow = time(0);
  struct tm* now = gmtime(&tnow);
  now->tm_year += 1900;
  ++now->tm_mon;

  if (year)  { *year  = now->tm_year; }
  if (month) { *month = now->tm_mon;  }
  if (day)   { *day   = now->tm_mday; }
  if (wday)  { *wday  = now->tm_wday; }
  if (hour)  { *hour  = now->tm_hour; }
  if (min)   { *min   = now->tm_min;  }
  if (sec)   { *sec   = now->tm_sec;  }
  if (dst)   { *dst   = (now->tm_isdst != 0); }
}


// function for converting a float time (e.g. difference of two BzTimes)
// into an array of ints
void BzTime::convertTime(double raw, long int convertedTimes[]) { // const
  long int day, hour, min, sec, remainder;
  static const int secondsInDay = 86400;

  sec = (long int)raw;
  day = sec / secondsInDay;
  remainder = sec - (day * secondsInDay);
  hour = remainder / 3600;
  remainder = sec - ((hour * 3600) + (day * secondsInDay));
  min = remainder / 60;
  remainder = sec - ((hour * 3600) + (day * secondsInDay) + (min * 60));
  sec = remainder;

  convertedTimes[0] = day;
  convertedTimes[1] = hour;
  convertedTimes[2] = min;
  convertedTimes[3] = sec;

  return;
}


// function for printing an array of ints representing a time
// as a human-readable string
const std::string BzTime::printTime(long int timeValue[]) {
  std::string valueNames;
  char temp[20];

  if (timeValue[0] > 0) {
    snprintf(temp, 20, "%ld day%s", timeValue[0], timeValue[0] == 1 ? "" : "s");
    valueNames.append(temp);
  }
  if (timeValue[1] > 0) {
    if (timeValue[0] > 0) {
      valueNames.append(", ");
    }
    snprintf(temp, 20, "%ld hour%s", timeValue[1], timeValue[1] == 1 ? "" : "s");
    valueNames.append(temp);
  }
  if (timeValue[2] > 0) {
    if ((timeValue[1] > 0) || (timeValue[0] > 0)) {
      valueNames.append(", ");
    }
    snprintf(temp, 20, "%ld min%s", timeValue[2], timeValue[2] == 1 ? "" : "s");
    valueNames.append(temp);
  }
  if (timeValue[3] > 0) {
    if ((timeValue[2] > 0) || (timeValue[1] > 0) || (timeValue[0] > 0)) {
      valueNames.append(", ");
    }
    snprintf(temp, 20, "%ld sec%s", timeValue[3], timeValue[3] == 1 ? "" : "s");
    valueNames.append(temp);
  }

  return valueNames;
}


// function for printing a float time difference as a human-readable string
const std::string BzTime::printTime(double diff) {
  long int temp[4];
  convertTime(diff, temp);
  return printTime(temp);
}


void BzTime::sleep(double seconds) {
  if (seconds <= 0.0) {
    return;
  }

#ifdef HAVE_USLEEP
  usleep((unsigned int)(1.0e6 * seconds));
  return;
#endif
#if defined(HAVE_SLEEP) && !defined(__APPLE__)
  // equivalent to _sleep() on win32 (not sleep(3))
  Sleep((DWORD)(seconds * 1000.0));
  return;
#endif
#ifdef HAVE_SNOOZE
  snooze((bigtime_t)(1.0e6 * seconds));
  return;
#endif
#ifdef HAVE_SELECT
  struct timeval tv;
  tv.tv_sec = (long)seconds;
  tv.tv_usec = (long)(1.0e6 * (seconds - tv.tv_sec));
  select(0, NULL, NULL, NULL, &tv);
  return;
#endif
#ifdef HAVE_WAITFORSINGLEOBJECT
  HANDLE dummyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  WaitForSingleObject(dummyEvent, (DWORD)(1000.0 * seconds));
  CloseHandle(dummyEvent);
  return;
#endif

  // fall-back case is fugly manual timekeeping
  BzTime now = BzTime::getCurrent();
  while ((BzTime::getCurrent() - now) < seconds) {
    continue;
  }
  return;
}

void BzTime::setProcessorAffinity(int processor) {
#ifdef HAVE_SCHED_SETAFFINITY
  /* linuxy fix for time travel */
  cpu_set_t mask;
  CPU_ZERO(&mask);
  (void) CPU_SET(processor, &mask);
  sched_setaffinity(0, sizeof(mask), &mask);
#elif defined(WIN32)
  /* windowsy fix for time travel */
  HANDLE hThread = GetCurrentThread();
  DWORD_PTR dwMask = 1 << processor;
  DWORD_PTR dwProcs = 0;
  GetProcessAffinityMask(NULL, NULL, &dwProcs);
  if (dwMask < dwProcs) {
    logDebugMessage(1, "Unable to set process affinity mask (specified processor does not exist).\n");
    return;
  }
  SetThreadAffinityMask(hThread, dwMask);
#else
  logDebugMessage(1, "Unable to set processor affinity to %d - function not implemented on this platform.\n", processor);
#endif
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=2 tabstop=8