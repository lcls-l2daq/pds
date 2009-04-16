#include "pds/vmon/VmonRecorder.hh"

#include "pds/vmon/VmonRecord.hh"
#include "pds/mon/MonClient.hh"

#include <stdio.h>

using namespace Pds;


VmonRecorder::VmonRecorder() :
  _state  (Disabled),
  _buff   (new char[0x10000]),
  _record (0),
  _size   (0),
  _output (0)
{
  sprintf(_path,"None");
}

VmonRecorder::~VmonRecorder()
{
  delete[] _buff;
}

void VmonRecorder::enable () 
{
  _state = Enabled; 
}

void VmonRecorder::disable() 
{
  _state = Disabled; 
}

void VmonRecorder::description(MonClient& client)
{
  switch(_state) {
  case Recording : 
    _close();
  case Enabled   : 
    _open(); 
    _state = Describing; 
    _clients.clear();
    _len = 0;
  case Describing: 
    _clients.insert(std::pair<MonClient*,int>(&client,_len));
    _len += _record->append(client);
  default:
    break;
  }
}

void VmonRecorder::payload(MonClient& client)
{
  switch(_state) {
  case Describing:
    _state = Recording;
    _flush();
  case Recording:
    { std::map<MonClient*,int>::iterator it = _clients.find(&client);
      if (it != _clients.end())
	_record->append(client,it->second);
    }
  default:
    break;
  }
}

void VmonRecorder::flush()
{
  if (_state==Recording && _record->len()!=sizeof(*_record)) 
    _flush();
}

void VmonRecorder::_flush()
{
  ::fwrite(_record,_record->len(),1,_output);
  _size += _record->len();
  //  delete _record;
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ClockTime ctime(ts.tv_sec,ts.tv_nsec);
  _record = new (_buff)VmonRecord(VmonRecord::Payload,ctime,_len);
}

void VmonRecorder::_open()
{
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ClockTime ctime(ts.tv_sec,ts.tv_nsec);
  time_t tm_t(::time(NULL));
  struct tm tm_s;
  localtime_r(&tm_t,&tm_s);
  strftime(_path,128,"vmon_%F_%T.dat",&tm_s);
  printf("Opening %s\n",_path);
  _size   = 0;
  _output = ::fopen(_path,"w");
  _record = new (_buff) VmonRecord(VmonRecord::Description,ctime);
}

void VmonRecorder::_close()
{
  flush();
  //  delete _record;
  ::fclose(_output);
}
