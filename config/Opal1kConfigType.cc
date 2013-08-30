#include "pds/config/Opal1kConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

unsigned Pds::Opal1k::max_row_pixels   (const DetInfo& info)
{
  switch(info.device()) {
  case Pds::DetInfo::Opal1000: return 1024;
  case Pds::DetInfo::Opal1600: return 1200;
  case Pds::DetInfo::Opal2000: return 1080;
  case Pds::DetInfo::Opal4000: return 1752;
  case Pds::DetInfo::Opal8000: return 2472;
  default:       return 0;
  }
}

unsigned Pds::Opal1k::max_column_pixels(const DetInfo& info)
{
  switch(info.device()) {
  case Pds::DetInfo::Opal1000: return 1024;
  case Pds::DetInfo::Opal1600: return 1600;
  case Pds::DetInfo::Opal2000: return 1920;
  case Pds::DetInfo::Opal4000: return 2336;
  case Pds::DetInfo::Opal8000: return 3296;
  default:       return 0;
  }
}
