#include "pvcam/include/master.h"
#include "pvcam/include/pvcam.h"
  
namespace PICAM
{  
  void printPvError(const char *sPrefixMsg);
  void displayParamIdInfo(int16 hCam, uns32 uParamId,
                          const char sDescription[]);
  int getAnyParam(int16 hCam, uns32 uParamId, void *pParamValue);
  int setAnyParam(int16 hCam, uns32 uParamId, const void *pParamValue);
  
  void printROI(int iNumRoi, rgn_type* roi);
}
