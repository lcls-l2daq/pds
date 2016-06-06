#ifndef Pds_Cphw_AxiVersion_hh
#define Pds_Cphw_AxiVersion_hh

namespace Pds {
  namespace Cphw {
    template <class T>
    class AxiVersion {
    public:
      std::string buildStamp() const;
    public:
      T FpgaVersion; 
      T ScratchPad; 
      T DeviceDnaHigh; 
      T DeviceDnaLow; 
      T FdSerialHigh; 
      T FdSerialLow; 
      T MasterReset; 
      T FpgaReload; 
      T FpgaReloadAddress; 
      T Counter; 
      T FpgaReloadHalt; 
      T reserved_11[0x100-11];
      T UserConstants[64];
      T reserved_0x140[0x200-0x140];
      T BuildStamp[64];
      T reserved_0x240[0x4000-0x240];
    };
  }
}

#endif
