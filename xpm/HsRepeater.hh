#ifndef Xpm_HsRepeater_hh
#define Xpm_HsRepeater_hh

#include "pds/cphw/Reg.hh"
#include "pds/cphw/Reg64.hh"

namespace Pds {
  namespace Xpm {

    class HsChannel {
    private:
      uint32_t    _reserved;            // Too bad this can't be at the end
    public:
      //  0x00 - RW: Signal detector force to off or on
      //  [1]    on   SD Preset
      //  [2]    off  SD Reset
      Cphw::Reg   _sigDetForce;
      //  0x04 - RW: Receive detect
      //  [3:2]  RXDET
      //  [4]    IDLE_SEL
      //  [5]    IDLE_AUTO
      Cphw::Reg   _rxDetect;
      //  0x08 - RW: Equalization control
      //  [7:0]  EQ Control
      Cphw::Reg   _eqControl;
      //  0x0C - RW: VOD control
      //  [2:0]  VOD control
      //  [6]    MODE_SEL
      //  [7]    Short Circuit Protection
      Cphw::Reg   _vodControl;
      //  0x10 - RW: DEM control
      //  [2:0]  DEM Control
      //  [6:5]  MODE_DET STATUS
      //  [7]    RXDET STATUS
      Cphw::Reg   _demControl;
      //  0x14 - RW: Idle threshold
      //  [1:0]  IDLE thd  Deassert threshold
      //  [3:2]  IDLE tha  Assert threshold
      Cphw::Reg   _idleThreshold;
    };

    class HsRepeater {
      enum { NChannels=4 };
    public:
      HsRepeater();
    public:
      //  0x0000 - RO: Device address readback
      //  [2]      EEPROM Read Done
      //  [6:3]    Address bit AD[3:0]
      Cphw::Reg   _deviceAddr;
      //  0x0004 - RW: Power down per channel
      //  [7:0]    PWDN CHx
      Cphw::Reg   _pwdnChannels;
      //  0x0008 - RO: Override power down
      //  [0]      Override PWDN pin
      //  [5:4]    LPBK Control
      Cphw::Reg   _overridePwdn;
    private:
      uint32_t    _reserved_12[3];
    public:
      //  0x0018 - RW: Enable slave register write
      //  [3]      Register Enable
      Cphw::Reg   _slaveRegCtrl;
      //  0x001c - RW: Digital Reset and Control
      //  [5]      Reset SMBus Master
      //  [6]      Reset Registers
      Cphw::Reg   _reserved_28; // Not impled? _digitalRstCtrl;
      //  0x0020 - RW: Override Pin Control
      //  [2]      Override MODE
      //  [3]      Override RXDET
      //  [4]      Override IDLE
      //  [6]      Override SD_TH
      Cphw::Reg   _reserved_32; // Not impled? overridePinCtrl;
    private:
      uint32_t    _reserved_36;
    public:
      //  0x0028 - RO: Signal detect monitor
      //  [7:0]    SD_TH Status
      Cphw::Reg   _sigDetMonitor;
    private:
      uint32_t    _reserved_44;
    public:
      //  -4 here because there's a reserved word the front of the struct:
      //  0x0034-4, 0x0050-4, 0x006c-4, 0x0088-4: CH0 - CHB0 to CH3 - CHB3
      HsChannel   _chB[NChannels];
      //  0x00A0 - RW: Signal detect control
      //  [1:0]    Reduced SD Gain
      //  [3:2]    Fast IDLE
      //  [5:4]    High IDLE
      Cphw::Reg   _sigDetControl;
      //  -4 here because there's a reserved word the front of the struct:
      //  0x00A8-4, 0x00C4-4, 0x00E0-4, 0x00FC-4: CH0 - CHA0 to CH3 - CHA3
      HsChannel   _chA[NChannels];
    private:
      uint32_t    _reserved_276[12];
    public:
      // 0x0144 - RO: Device ID
      Cphw::Reg   _deviceID;
    private:
      uint32_t    _reserved_284[16100]; // To keep size = 0x10000 bytes
    };
  };
};

#endif
