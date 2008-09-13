#ifndef PDS_ERRLOG_HH
#define PDS_ERRLOG_HH

class ErrLog {
public:
  enum Severity {debugging=-1, trace=0, routine, warning, error, fatal};
  virtual void msg(Severity severity, 
		   const char* where, 
		   const char* message) = 0;
};

#endif
