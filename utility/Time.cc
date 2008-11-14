//
// $Id$
//

#include <time.h>

#include "Time.hh"
#include "QuadWord.hh"

using namespace Pds;

Time::Time()
{
  struct timespec ts;

  clock_gettime (CLOCK_REALTIME, &ts);

  Time t(ts);

  *this = t;
}

Time::Time(BinTime time)
{
  btime = time;
}

Time::Time(UnixTime time)
{
  QuadWord longsecond(0,time.second - SECONDS_1970_TO_1997);
  QuadWord longusecond(0,time.usecond);
  QuadWord upperusecond = longsecond * 1000000UL;
  QuadWord usecond      = upperusecond + longusecond;
  QuadWord binary       = (usecond * 59UL) + (usecond / 2) ;  // Convert to 59.5 MHz ticks

  btime.upper =  binary.upperword();
  btime.lower =  binary.lowerword();
}

Time::Time(const struct timespec &time)
{
  UnixTime uTime(time.tv_sec, time.tv_nsec / 1000);
  Time     oTime(uTime);

  *this = oTime;
}

Time::Time(uint32_t upper, uint32_t lower)
{
  btime.upper   = upper;
  btime.lower   = lower;
}

Time::Time(const Time& time)
{
  btime  = time.binary();
}

Time::~Time(){}

BinTime Time::binary() const
{
  return btime;
}

UnixTime Time::Unix() const
{
  uint32_t second, usecond;

  // convert from sysclks to microseconds by dividing by 59.5.
  QuadWord temp(btime.upper,btime.lower);
  int overflow = btime.upper & 0x80000000; // if highest bit set there will
                                           // be an overflow
  // remove highest bit if overflow will occur
  if (overflow) {
    QuadWord mask(0x80000000, 0x0);
    temp = temp - mask;
  }
  uint16_t remainder;
  temp = (temp + temp) / 119;           // temp / 59.5

  // now add in correction for overflow if necessary
  if (overflow) {
    QuadWord correction(0xffffffff, 0xffffffff);
    correction = correction / 119;
    temp = temp + correction;
  }

  // get microseconds and seconds separately by dividing by 1,000,000
  // and looking at remainder and quotient.
  // do two divisions of 1000, since we only support USHORT division now.
  remainder = temp % 1000;
  temp = temp / 1000;
  usecond = remainder;
  remainder = temp % 1000;
  temp = temp / 1000;
  usecond = usecond+1000*remainder;
  second  = temp.lowerword() + SECONDS_1970_TO_1997;

  return UnixTime(second, usecond);

}

int Time::timeSpec(struct timespec *ts) const
{
  if ( ts == 0 )  return -1;
  UnixTime u = Unix();
  ts->tv_sec  = u.second;
  ts->tv_nsec = u.usecond * 1000;
  return (0);
}

Time Time::prevSubFidTime() const
{
  QuadWord temp(btime.upper,btime.lower);

  temp = (temp / SUBFIDPERIOD) * SUBFIDPERIOD;


  BinTime retval(temp.upperword(), temp.lowerword());

  return Time(retval);
}

uint16_t Time::modulus() const
{
  uint16_t remainder;

  QuadWord temp(btime.upper,btime.lower);

  remainder = temp % SUBFIDPERIOD;

  return remainder;
}

// Now define all the operators...

Time& Time::operator=(const Time& time)
{
  btime   = time.binary();
  return *this;
}

int Time::operator==(const Time& time) const
{
  return (btime.lower == time.btime.lower) &&
         (btime.upper == time.btime.upper);         
}

int Time::operator!=(const Time& time) const
{
  return (btime.lower != time.btime.lower) ||
         (btime.upper != time.btime.upper);
}

int Time::operator==(BinTime time) const
{
  return (btime.lower == time.lower) && (btime.upper == time.upper);
}

int Time::operator==(UnixTime time) const
{
//   if (!utime.second)
//     utime = Unix(); // Ensures local unix value
//   return (utime.second == time.second) && (utime.usecond == time.usecond);
  //*** Need to implement this!
  return 0;
}

int Time::operator>(const Time& time) const
{
   return (btime.upper > time.binary().upper) ||
    ((btime.upper == time.binary().upper) && (btime.lower > time.binary().lower));
}

int Time::operator>(BinTime time) const
{
  return (btime.upper > time.upper) ||
    ((btime.upper == time.upper) && (btime.lower > time.lower));
}

int Time::operator>(UnixTime time) const
{
//   return (utime.second > time.second) ||
//     ((utime.second == time.second) && (utime.usecond > time.usecond));
  //*** Need to implement this!
  return 0;
}

int Time::operator<(const Time& time) const
{
  return (btime.upper < time.binary().upper) ||
    ((btime.upper == time.binary().upper)
     && (btime.lower < time.binary().lower));
}

int Time::operator<(BinTime time) const
{
  return (btime.upper < time.upper) ||
    ((btime.upper == time.upper) && (btime.lower < time.lower));
}

int Time::operator<(UnixTime time) const
{
//   return (utime.second < time.second) ||
//     ((utime.second == time.second) && (utime.usecond < time.usecond));
  //*** Need to implement this!
  return 0;
}

UnixTime& UnixTime::operator=(const UnixTime& time)
{
  second  = time.second;
  usecond = time.usecond;
  return *this;
}

BinTime& BinTime::operator=(const BinTime& time)
{
  upper   = time.upper;
  lower   = time.lower;
  return *this;
}

UnixTime::UnixTime(const UnixTime& time)
{
  second  = time.second;
  usecond = time.usecond;
}

BinTime::BinTime(const BinTime& time)
{
  upper   = time.upper;
  lower   = time.lower;
}
