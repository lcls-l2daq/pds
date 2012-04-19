#include "pvcam/include/master.h"
#include "pvcam/include/pvcam.h"
  
namespace PICAM
{  
  void printPvError(const char *sPrefixMsg);
  void displayParamIdInfo(int16 hCam, uns32 uParamId,
                          const char sDescription[]);
                          
  enum EnumGetParam
  {
    GET_PARAM_CURRENT = 0,
    GET_PARAM_MIN     = 1,
    GET_PARAM_MAX     = 2,
    GET_PARAM_DEFAULT = 3,
    GET_PARAM_INC     = 4,
  };
  
  int getAnyParam(int16 hCam, uns32 uParamId, void *pParamValue, EnumGetParam eumGetParam = GET_PARAM_CURRENT);
  int setAnyParam(int16 hCam, uns32 uParamId, const void *pParamValue);
  
  void printROI(int iNumRoi, rgn_type* roi);
}
