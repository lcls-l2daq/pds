#include "pds/config/pnCCDConfigType.hh"

void Pds::pnCCDConfig::setNumLinks( pnCCDConfigType& c,
                                    const std::string& fname )
{
  FILE* fp = ::fopen(fname.c_str(), "r");
  size_t ret;
  if (fp) {
    printf("Reading pnCCD config file\n");
    ret = fread(&c, sizeof(pnCCDConfigType), 1, fp);
    if (ret != 1) {
      printf("Error reading pnCCD config file\n");
      throw std::string("Error reading pnCCD config file") ;
    }
  } else {
    perror("Could not open pnCCD file\n");
    throw std::string("Could not open pnCCD file\n");
  }
}
