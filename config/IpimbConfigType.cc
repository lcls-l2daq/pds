#include "pds/config/IpimbConfigType.hh"

static const char* _cap_range[] = { "1 pF", "4.7 pF", "24pF", "120pF", "620 pF", "3.3nF","10 nF", "expert", NULL };
const char** Pds::IpimbConfig::cap_range() { return _cap_range; }
