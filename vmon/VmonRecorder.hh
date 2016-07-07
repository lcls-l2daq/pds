#ifndef Pds_VmonRecorder_hh
#define Pds_VmonRecorder_hh

#include <map>
#include <string>

#include <stdio.h>

namespace Pds {

  class MonClient;
  class VmonRecord;

  class VmonRecorder {
  public:
    VmonRecorder(const char* root=".",
                 const char* base="vmon");
    ~VmonRecorder();
  public:
    void enable();
    void disable();
    void begin(int);
    void end();
  public:
    void description(MonClient&);
    void payload    (MonClient&);
    void flush      ();
  public:
    const char* filename() const { return _path; }
    unsigned    filesize() const { return _size; }
  private:
    void _open(int);
    void _close();
    void _flush(const VmonRecord*);
  private:
    enum State { Disabled, Enabled, Describing, Recording };
    State _state;

    char*       _dbuff;   // where the description update is stored
    char*       _pbuff;   // where the payload update is stored
    VmonRecord* _drecord; // the description record update under construction
    VmonRecord* _precord; // the payload record update under construction

    std::map<MonClient*,int> _clients;
    unsigned                 _len;

    std::string _root;
    std::string _base;
    enum { MAX_FNAME_SIZE=128 };
    char     _path[MAX_FNAME_SIZE];
    unsigned _size;
    FILE*    _output;
    bool     _persistent;
  };
};

#endif
