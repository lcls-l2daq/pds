#ifndef Pds_VmonRecorder_hh
#define Pds_VmonRecorder_hh

#include <map>

#include <stdio.h>

namespace Pds {

  class MonClient;
  class VmonRecord;

  class VmonRecorder {
  public:
    VmonRecorder();
    ~VmonRecorder();
  public:
    void enable();
    void disable();
  public:
    void description(MonClient&);
    void payload    (MonClient&);
    void flush      ();
  public:
    const char* filename() const { return _path; }
    unsigned    filesize() const { return _size; }
  private:
    void _open();
    void _close();
    void _flush();
  private:
    enum State { Disabled, Enabled, Describing, Recording };
    State _state;

    char*       _buff;   // where the update is stored
    VmonRecord* _record; // the record update under construction

    std::map<MonClient*,int> _clients;
    unsigned                 _len;

    char     _path[128];
    unsigned _size;
    FILE*    _output;
  };
};

#endif
