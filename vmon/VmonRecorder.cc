#include "pds/vmon/VmonRecorder.hh"

#include "pds/vmon/VmonRecord.hh"
#include "pds/mon/MonClient.hh"

#include <stdio.h>

using namespace Pds;

VmonRecorder::VmonRecorder(const char* base) :
  _state  (Disabled),
  _dbuff  (new char[VmonRecord::MaxLength]),
  _pbuff  (new char[VmonRecord::MaxLength]),
  _drecord(0),
  _precord(0),
  _base   (base),
  _size   (0),
  _output (0)
{
  sprintf(_path,"%s/",base);
}

VmonRecorder::~VmonRecorder()
{
  delete[] _dbuff;
  delete[] _pbuff;
}

void VmonRecorder::enable () 
{
  _state = Enabled; 
}

void VmonRecorder::disable() 
{
  if (_state==Recording) _close();
  _state = Disabled; 
}

void VmonRecorder::begin(int n)
{
  _open(n);
  _state = Recording;
}

void VmonRecorder::end()
{
  _close();
  _state = Enabled;
}

void VmonRecorder::description(MonClient& client)
{
  switch(_state) {
  case Recording : 
    printf("Received description while recording\n");
    break;
  case Enabled   : 
    printf("Resetting description\n");
    { _state = Describing; 
      _clients.clear();
      _len = 0;
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ClockTime ctime(ts.tv_sec,ts.tv_nsec);
      _drecord = new (_dbuff) VmonRecord(VmonRecord::Description,ctime);
    }
  case Describing: 
    _clients.insert(std::pair<MonClient*,int>(&client,_len));
    _len += _drecord->append(client);
    if (_len > VmonRecord::MaxLength) {
      printf("Description record len = %d, exceeds max RECORD_LEN(%d)\n",
	     _len,VmonRecord::MaxLength);
      abort();
    }
  default:
    break;
  }
}

void VmonRecorder::payload(MonClient& client)
{
  switch(_state) {
  case Enabled:
  case Recording:
    { std::map<MonClient*,int>::iterator it = _clients.find(&client);
      if (it != _clients.end())
	_precord->append(client,it->second);
    }
  default:
    break;
  }
}

void VmonRecorder::flush()
{
  if (_state==Recording && _precord->len()!=sizeof(*_precord))
    _flush(_precord);

  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ClockTime ctime(ts.tv_sec,ts.tv_nsec);
  _precord = new (_pbuff)VmonRecord(VmonRecord::Payload,ctime,_len);
}

void VmonRecorder::_flush(const VmonRecord* record)
{
  ::fwrite(record,record->len(),1,_output);
  _size += record->len();
}

void VmonRecorder::_open(int n)
{
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ClockTime ctime(ts.tv_sec,ts.tv_nsec);
  time_t tm_t(::time(NULL));
  struct tm tm_s;
  localtime_r(&tm_t,&tm_s);
  char path[256];
  if (n>=0)
    sprintf(_path,"vmon_run%04d",n);
  else
    sprintf(_path,"vmon");
  strftime(_path+strlen(_path),MAX_FNAME_SIZE,"_%F_%T.dat",&tm_s);
  sprintf(path,"%s/%s",_base,_path);
  printf("Opening %s\n",path);
  _size   = 0;
  _output = ::fopen(path,"w");


  _drecord->time(ctime);
  _flush(_drecord);
}

void VmonRecorder::_close()
{
  ::fclose(_output);
}
