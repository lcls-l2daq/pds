#ifndef PDS_IPIMBOARD
#define PDS_IPIMBOARD

#include <arpa/inet.h>

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <time.h>

#include "pdsdata/ipimb/ConfigV2.hh"

#define BAUDRATE B115200
#define DataPackets 16
#define CRPackets 4

static const int NChannels = 4;
static const int BufferLength = 100;

namespace Pds {
  class IpimBoardData;
  class IpimBoardPacketParser;
  class IpimBoardBaselineHistory;

  enum {
    crcFailed = 0x00,
    packetIncomplete = 0x01,
    baselineShift = 0x02
  };

  class IpimBoard {
  public:
    IpimBoard(char* serialDevice);
    ~IpimBoard();// {printf("IPBM::dtor, this = %p\n", this);}
    
    enum { // was IpimBoardRegisters
      timestamp0 = 0x00,
      timestamp1 = 0x01,
      serid0 = 0x02,
      serid1 = 0x03,
      adc0 = 0x04,
      adc1 = 0x05,
      rg_config = 0x06,
      cal_rg_config = 0x07,
      reset = 0x08,
      bias_data = 0x09,
      cal_data = 0x0a,
      biasdac_data_config = 0x0b,
      status = 0x0c,
      errors = 0x0d,
      cal_strobe = 0x0e,
      trig_delay = 0x0f,
      trig_ps_delay = 0x10,
      adc_delay = 0x11,
      vhdl_version = 0x17
    };

    void SetTriggerCounter(unsigned start0, unsigned start1);
    unsigned GetTriggerCounter1();
    void SetCalibrationMode(bool*);
    void SetCalibrationDivider(unsigned*);
    void SetCalibrationPolarity(bool*);
    void SetChargeAmplifierRef(float refVoltage);
    void SetCalibrationVoltage(float calibrationVoltage);
    void SetChargeAmplifierMultiplier(unsigned*);
    void oldSetChargeAmplifierMultiplier(unsigned*);
    void SetInputBias(float biasVoltage);
    void SetChannelAcquisitionWindow(uint32_t acqLength, uint16_t acqDelay);
    void SetTriggerDelay(uint32_t triggerDelay);
    void SetTriggerPreSampleDelay(uint32_t triggerPreSampleDelay);
    void CalibrationStart(unsigned calStrobeLength);
    unsigned ReadRegister(unsigned regAddr);
    void WriteRegister(unsigned regAddr, unsigned regValue);
    IpimBoardData WaitData();
    //  unsigned* read(bool command, int nBytes);
    void WriteCommand(unsigned*);
    int inWaiting(IpimBoardPacketParser&);
    
    bool configure(Ipimb::ConfigV2& config);
    bool unconfigure();
    bool setReadable(bool);
    void setBaselineSubtraction(const int, const int);
    void setOldVersion();
    int get_fd();
    void flush();
    int dataDamage();
    bool commandResponseDamaged();
    uint64_t GetSerialID();
    uint16_t GetStatus();
    uint16_t GetErrors();
    
  private:
    unsigned _commandList[CRPackets];
    unsigned _dataList[DataPackets];
    int _commandIndex;
    int _dataIndex;
    int _fd;
    char* _serialDevice;
    int _baselineSubtraction, _polarity;
    bool _c01;
    IpimBoardBaselineHistory* _history;
    int _dataDamage, _commandResponseDamage;

    void _SetAdcDelay(uint32_t adcDelay);
  };
  
  
  class IpimBoardCommand {
  public:
    IpimBoardCommand(bool write, unsigned address, unsigned data);
    ~IpimBoardCommand();// {printf("IPBMC::dtor, this = %p\n", this);}
    
    unsigned* getAll();
    
  private:
    unsigned _commandList[CRPackets];
  };


  class IpimBoardResponse {
  public:
    IpimBoardResponse(unsigned* packet);
    ~IpimBoardResponse();// {printf("IPBMR::dtor, this = %p\n", this);}
    
    bool CheckCRC();
    void setAll(int, int, unsigned*);
    unsigned* getAll(int, int);
    unsigned getData();
    
  private:
    unsigned _addr;
    unsigned _data;
    unsigned _checksum;
    unsigned _respList[CRPackets];
  };
  
  class IpimBoardBaselineHistory {
  public:
    IpimBoardBaselineHistory();
    ~IpimBoardBaselineHistory(){}
    
    double baselineBuffer[NChannels][BufferLength];
    double baselineBufferedAverage[NChannels];
    int nUpdates;
    int bufferLength;
    double baselineRunningAverage[NChannels];
  };

  class IpimBoardPsData {
  public:
    IpimBoardPsData(unsigned* packet, const int baselineSubtraction, const int polarity, IpimBoardBaselineHistory* history, int& dataDamage);
    IpimBoardPsData();
    ~IpimBoardPsData() {};//printf("IPBMD::dtor, this = %p\n", this);}
    
    bool CheckCRC();
    uint64_t GetTriggerCounter();
    unsigned GetTriggerDelay_ns();
    unsigned GetTriggerPreSampleDelay_ns();
    uint16_t GetCh(int channel);
    unsigned GetConfig0();
    unsigned GetConfig1();
    unsigned GetConfig2();
    unsigned GetChecksum();
    void setAll(int, int, unsigned*);
    void setList(int, int, unsigned*);
    void updateBaselineHistory();
    
    void dumpRaw();

  private:
    uint64_t _triggerCounter;
    uint16_t _config0;
    uint16_t _config1;
    uint16_t _config2;
    uint16_t _ch0;
    uint16_t _ch1;
    uint16_t _ch2;
    uint16_t _ch3;
    uint16_t _ch0_ps;
    uint16_t _ch1_ps;
    uint16_t _ch2_ps;
    uint16_t _ch3_ps;
    uint16_t _checksum;
    
    double _presampleChannel[NChannels], _sampleChannel[NChannels];
    int _baselineSubtraction, _polarity;
    int _dataDamage;
    IpimBoardBaselineHistory* _history;
  };
  
  class IpimBoardData {
  public:
    IpimBoardData(IpimBoardPsData data);
    //    IpimBoardData();
    ~IpimBoardData() {};//printf("IPBMD::dtor, this = %p\n", this);}
    
    uint64_t GetTriggerCounter();
    unsigned GetTriggerDelay_ns();
    unsigned GetTriggerPreSampleDelay_ns();
    /*
    float GetCh0_V();
    float GetCh1_V();
    float GetCh2_V();
    float GetCh3_V();
    float GetCh0_ps_V();
    float GetCh1_ps_V();
    float GetCh2_ps_V();
    float GetCh3_ps_V();
    */
    unsigned GetConfig0();
    unsigned GetConfig1();
    unsigned GetConfig2();
    void dumpRaw();

  private:
    uint64_t _triggerCounter;
    uint16_t _config0;
    uint16_t _config1;
    uint16_t _config2;
    uint16_t _ch0;
    uint16_t _ch1;
    uint16_t _ch2;
    uint16_t _ch3;
    uint16_t _ch0_ps;
    uint16_t _ch1_ps;
    uint16_t _ch2_ps;
    uint16_t _ch3_ps;
    uint16_t _checksum;
  };
  
  class IpimBoardPacketParser {
  public:
    IpimBoardPacketParser(bool command, int* damage, unsigned* lst);
    ~IpimBoardPacketParser() {};
    
    void update(char*);
    bool packetIncomplete();
    void readTimedOut(int, int, char*);
    int packetsRead();

  private:
    bool _command;
    int* _damage;
    bool _lastDamaged;
    unsigned* _lst;
    int _nPackets;
    int _allowedPackets, _leadHeaderNibble, _bodyHeaderNibble, _tailHeaderNibble;
  };

  unsigned CRC(unsigned*, int);
  timespec diff(timespec start, timespec end);
}
#endif
