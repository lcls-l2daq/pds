#ifndef PDS_SELECT_HH
#define PDS_SELECT_HH

namespace Pds {
class Select {
public:
  virtual int poll() = 0;
};
}
#endif
