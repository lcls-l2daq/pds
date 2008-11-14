#ifndef Pds_Time_included
#define Pds_Time_included

//--------------------------------------------------------------------------
//
// Time
// 
// Environment:
//      This software was developed for the BaBar collaboration.  If you
//      use all or part of it, please give an appropriate acknowledgement.
//
// Copyright Information:
//      Copyright (C) 1997,1999      Stanford University, Caltech
//
//------------------------------------------------------------------------


#include <time.h>

#include <stdint.h>


#define SUBFIDPERIOD 873 // Number of sysclks in a SubFiducial period

// The following excludes leap seconds.  Is this a problem?
// (97*365+24)*24*60*60 = 3,061,065,600; 24 leap years.  1900 wasn't a leap year
#define SECONDS_1900_TO_1997 3061065600UL

// (27*365+7)*24*60*60 = 852,076,800;     7 leap years in the period
#define SECONDS_1970_TO_1997 852076800UL

namespace Pds {

class UnixTime
{
public:
  uint32_t second;
  uint32_t usecond;
  UnixTime& operator=(const UnixTime& time);
  UnixTime(const UnixTime& time);
  UnixTime() {}
  UnixTime(uint32_t sec, uint32_t usec) : second(sec), usecond(usec) {}
};

class BinTime   // The order here is important for the fcPM class
{
public:
  uint32_t lower;
  uint32_t upper;
  BinTime& operator=(const BinTime& time);
  BinTime(const BinTime& time);
  BinTime() {}
  BinTime(uint32_t up, uint32_t low) : lower(low), upper(up) {}
};

class Time{

public:
  Time();

  Time(BinTime time);

  Time(UnixTime time);

  Time(const struct timespec &time);

  Time(uint32_t upper, uint32_t lower);

  Time(const Time& time);

  ~Time();

  BinTime binary() const;

  UnixTime Unix() const;

  int timeSpec(struct timespec *ts) const;

  Time prevSubFidTime() const;

  uint16_t modulus() const;

  // Now define all the operators...

  Time& operator=(const Time& time);

  int operator==(const Time& time) const;

  int operator!=(const Time& time) const;

  int operator==(BinTime time) const;

  int operator==(UnixTime time) const;

  int operator>(const Time& time) const;

  int operator>(BinTime time) const;

  int operator>(UnixTime time) const;

  int operator<(const Time& time) const;

  int operator<(BinTime time) const;

  int operator<(UnixTime time) const;

private:

  BinTime btime;

};
}

#endif  // Time_included
