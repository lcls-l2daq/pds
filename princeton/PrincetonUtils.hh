#include "pvcam/include/master.h"
#include "pvcam/include/pvcam.h"
  
namespace PICAM
{  
  void printPvError(const char *sPrefixMsg);
  void displayParamIdInfo(int16 hCam, uns32 uParamId, const char sDescription[]);
                            
  int getAnyParam(int16 hCam, uns32 uParamId, void *pParamValue, int16 iMode = ATTR_CURRENT, int iErrorReport = 1);
  //int getAnyParam(int16 hCam, uns32 uParamId, void *pParamValue, EnumGetParam eumGetParam = GET_PARAM_CURRENT);
  int setAnyParam(int16 hCam, uns32 uParamId, const void *pParamValue);
  
  void printROI(int iNumRoi, rgn_type* roi);
}


// To avoid pvcam.so dependency on firewire raw1394 shared library
extern "C" 
{
void raw1394_arm_register();
void raw1394_new_handle_on_port();
void raw1394_new_handle();
void raw1394_get_nodecount();
void raw1394_arm_get_buf();
void raw1394_arm_unregister();
void raw1394_destroy_handle();
void raw1394_read();
void raw1394_write();
void raw1394_get_port_info();
void raw1394_get_fd();
void raw1394_loop_iterate();
}
