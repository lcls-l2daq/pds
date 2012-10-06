#ifndef ANDOR_ERROR_CODES_HH
#define ANDOR_ERROR_CODES_HH

class AndorErrorCodes 
{
public:
  AndorErrorCodes()   {}
  ~AndorErrorCodes()  {}

  static const char *name(unsigned id);
};

#endif
