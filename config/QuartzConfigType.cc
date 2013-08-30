#include "pds/config/QuartzConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

unsigned Pds::Quartz::max_row_pixels   (const DetInfo& info)
{
  switch(info.device()) {
  case Pds::DetInfo::Quartz4A150: return 2048;
  default:       return 0;
  }
}

unsigned Pds::Quartz::max_column_pixels(const DetInfo& info)
{
  switch(info.device()) {
  case Pds::DetInfo::Quartz4A150: return 2048;
  default:       return 0;
  }
}
