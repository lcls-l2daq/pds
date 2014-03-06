#include "picam/include/picam_advanced.h"
#include <string>

namespace PiUtils
{

const char* piErrorDesc(int iError);
const char* piCameraDesc(const PicamCameraID& piCameraId);
int         piGetEnum(PicamHandle hCam, PicamParameter parameter, std::string& sResult);
int         piReadTemperature(PicamHandle hCam, float& fTemperature, std::string& sTemperatureStatus);
void        piPrintAllParameters(PicamHandle camera);
void        piPrintParameter(PicamHandle camera, PicamParameter parameter);
int         piCommitParameters(PicamHandle camera);
int         piWaitAcquisitionUpdate(PicamHandle hCam, int iMaxReadoutTimeInMs, bool bCleanAcq, bool bStopAcqIfTimeout, unsigned char** ppData);

inline bool piIsFuncOk(int iError)
{
  return (iError == PicamError_None);
}

} // namespace PiUtils

