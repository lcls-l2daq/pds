#include "IpimBoard.hh"
#include "unistd.h" //for sleep
#include <iostream>
#include "assert.h"
#include "memory.h"
// for non-blocking attempt
#include <fcntl.h>
#include <cmath>

#include <stdlib.h>

using namespace std;
using namespace Pds;

const float CHARGEAMP_REF_MAX = 10.0;
const unsigned CHARGEAMP_REF_STEPS = 65536;

const float CALIBRATION_V_MAX = 10.0;
const unsigned CALIBRATION_V_STEPS = 65536;

const float INPUT_BIAS_MAX = 200;
const unsigned INPUT_BIAS_STEPS = 65536;

const int CLOCK_PERIOD = 8;
const float ADC_RANGE = 5.0;
const unsigned ADC_STEPS = 65536;

static const int HardCodedPresampleDelay = 150000;  // settling before presamples
static const int HardCodedAdcDelay = 4000; // time between presamples; this is the default on the board
static const int HardCodedTotalPresampleTime = (8+2)*HardCodedAdcDelay; // 8 presamples plus a bit extra
static const int OldHardCodedADCTime = 100000;
static const unsigned OldVhdlVersion = 0xDEADBEEF;
static const unsigned NewVhdlVersion = 0x00010000;


const bool DEBUG = false;

static int KvetchCounter = 0;
static int AllowedKvetches = 50;
static int BaselineKvetchCounter = 0;
static const bool DumpSampleEvents = false; // should be false in normal running

static float BaselineShiftTolerance = 0.005*(ADC_STEPS-1)/ADC_RANGE; // 5 mV

namespace Pds {

  unsigned CRC(unsigned* lst, int length) {
    unsigned crc = 0xffff;
    for (int l=0; l<length; l++) {
      unsigned word = *lst;
      if (DEBUG) {
	printf("In CRC, have word 0x%x\n", word);
      }
      lst++;
      // Calculate CRC 0x0421 (x16 + x12 + x5 + 1) for protocol calculation
      unsigned C[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      unsigned CI[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      unsigned D[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      for (int i=0; i<16; i++){
	C[i] = (crc & (1 << i)) >> i;
	D[i] = (word & (1 << i)) >> i;
      }
      CI[0] = D[12] ^ D[11] ^ D[8] ^ D[4] ^ D[0] ^ C[0] ^ C[4] ^ C[8] ^ C[11] ^ C[12];
      CI[1] = D[13] ^ D[12] ^ D[9] ^ D[5] ^ D[1] ^ C[1] ^ C[5] ^ C[9] ^ C[12] ^ C[13];
      CI[2] = D[14] ^ D[13] ^ D[10] ^ D[6] ^ D[2] ^ C[2] ^ C[6] ^ C[10] ^ C[13] ^ C[14];
      CI[3] = D[15] ^ D[14] ^ D[11] ^ D[7] ^ D[3] ^ C[3] ^ C[7] ^ C[11] ^ C[14] ^ C[15];
      CI[4] = D[15] ^ D[12] ^ D[8] ^ D[4] ^ C[4] ^ C[8] ^ C[12] ^ C[15];
      CI[5] = D[13] ^ D[12] ^ D[11] ^ D[9] ^ D[8] ^ D[5] ^ D[4] ^ D[0] ^ C[0] ^ C[4] ^  C[5] ^ C[8] ^ C[9] ^ C[11] ^ C[12] ^ C[13];
      CI[6] = D[14] ^ D[13] ^ D[12] ^ D[10] ^ D[9] ^ D[6] ^ D[5] ^ D[1] ^ C[1] ^ C[5] ^ C[6] ^ C[9] ^ C[10] ^ C[12] ^ C[13] ^ C[14];
      CI[7] = D[15] ^ D[14] ^ D[13] ^ D[11] ^ D[10] ^ D[7] ^ D[6] ^ D[2] ^ C[2] ^ C[6] ^ C[7] ^ C[10] ^ C[11] ^ C[13] ^ C[14] ^ C[15];
      CI[8] = D[15] ^ D[14] ^ D[12] ^ D[11] ^ D[8] ^ D[7] ^ D[3] ^ C[3] ^ C[7] ^ C[8] ^ C[11] ^ C[12] ^ C[14] ^ C[15];
      CI[9] = D[15] ^ D[13] ^ D[12] ^ D[9] ^ D[8] ^ D[4] ^ C[4] ^ C[8] ^ C[9] ^ C[12] ^ C[13] ^ C[15];
      CI[10] = D[14] ^ D[13] ^ D[10] ^ D[9] ^ D[5] ^ C[5] ^ C[9] ^ C[10] ^ C[13] ^ C[14];
      CI[11] = D[15] ^ D[14] ^ D[11] ^ D[10] ^ D[6] ^ C[6] ^ C[10] ^ C[11] ^ C[14] ^ C[15];
      CI[12] = D[15] ^ D[8] ^ D[7] ^ D[4] ^ D[0] ^ C[0] ^ C[4] ^ C[7] ^ C[8] ^ C[15];
      CI[13] = D[9] ^ D[8] ^ D[5] ^ D[1] ^ C[1] ^ C[5] ^ C[8] ^ C[9];
      CI[14] = D[10] ^ D[9] ^ D[6] ^ D[2] ^ C[2] ^ C[6] ^ C[9] ^ C[10];
      CI[15] = D[11] ^ D[10] ^ D[7] ^ D[3] ^ C[3] ^ C[7] ^ C[10] ^ C[11];
      crc = 0;
      for (int i=0; i<16; i++){
	crc = ((CI[i] <<i) + crc) & 0xffff;
      }
    }
    //  cout << "CRC calculates:" << crc << endl;
    return crc;
  }

  timespec diff(timespec start, timespec end)
  {
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
      temp.tv_sec = end.tv_sec-start.tv_sec-1;
      temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
      temp.tv_sec = end.tv_sec-start.tv_sec;
      temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
  }
}

IpimBoard::IpimBoard(char* serialDevice) {
  _serialDevice = serialDevice;
  struct termios newtio;
  memset(&newtio, 0, sizeof(newtio));
        
  //  /* open the device to be non-blocking (read will return immediately) */
  _fd = open(serialDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (_fd <0) {
    perror(serialDevice); exit(-1); 
  }

  //  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL;// | CREAD;
  //  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;//ICANON;
  newtio.c_cc[VMIN]=0;
  newtio.c_cc[VTIME]=0;
  newtio.c_iflag = 0;
  //  printf("cflag 0x%x, iflag %d, oflag %d\n", newtio.c_cflag, newtio.c_iflag, newtio.c_oflag);
  //  printf("iflag %d, oflag %d, cflag %d, lflag %d, ispeed %d, ospeed %d\n", newtio.c_iflag, newtio.c_oflag, newtio.c_cflag, newtio.c_lflag, newtio.c_ispeed, newtio.c_ospeed);
  flush();
  tcsetattr(_fd,TCSANOW,&newtio);
  
  memset(_dataList, 0, DataPackets*sizeof(unsigned));
  memset(_commandList, 0, CRPackets*sizeof(unsigned));
  _commandIndex = 0;
  _dataIndex = 0;

  _history = new IpimBoardBaselineHistory();
  _dataDamage = 0;
  _commandResponseDamage = false;

  _c01 = false; // hack to allow c02 code to read old boards during transition
}

IpimBoard::~IpimBoard() {
  //  _ser.close();
  // delete _lstData;
  // delete _lstCommands;
  delete _history;
}

void IpimBoard::SetTriggerCounter(unsigned start0, unsigned start1) {
  WriteRegister(timestamp0, start0);
  WriteRegister(timestamp1, start1);
  //  unsigned val0 = ReadRegister(timestamp0);
  //  unsigned val1 = ReadRegister(timestamp1);
  //  printf("have tried to set timestamp counter to 0x%x, 0x%x, reading 0x%x, 0x%x\n", start0, start1, val0, val1);
}

unsigned IpimBoard::GetTriggerCounter1() {
  return ReadRegister(timestamp1);
}

uint64_t IpimBoard::GetSerialID() {  
  uint32_t id0 = ReadRegister(serid0);
  uint32_t id1 = ReadRegister(serid1);
  uint64_t id = ((uint64_t)id0<<32) + id1;
  //  printf("id0: 0x%lx, id1: 0x%lx, id: 0x%llx\n", id0, id1, id);
  return id;
}

uint16_t IpimBoard::GetErrors() {
  return ReadRegister(errors);
}

uint16_t IpimBoard::GetStatus() {
  return ReadRegister(status)>>16;
}

void IpimBoard::SetCalibrationMode(bool* channels) {
  unsigned val = ReadRegister(cal_rg_config);
  //  printf("have retrieved config val 0x%x\n", val);
  val &= 0x7777;
  int shift = 3;
  for (int i=0; i<4; i++) {
    bool b = channels[i];
    if (b) 
      val |= 1<<shift;
    shift += 4;
  }
  if (!_c01) {
    WriteRegister(cal_rg_config, val);
  } else {
    WriteRegister(rg_config, val);
  }
}

void IpimBoard::SetCalibrationDivider(unsigned* channels) {
  unsigned val = ReadRegister(cal_rg_config);
  val &= 0xcccc;
  int shift = 0;
  for (int i=0; i<4; i++) {
    unsigned setting = channels[i];
    if (setting>2) {
      printf("IpimBoard error: Cal divider setting %d is illegal\n",setting);
      _commandResponseDamage = true;
    }
    else 
      val |= (setting+1)<<shift;
    shift+=4;
  }
  WriteRegister(cal_rg_config, val);
}

void IpimBoard::SetCalibrationPolarity(bool* channels) {
  unsigned val = ReadRegister(cal_rg_config);
  val &= 0xbbbb;
  int shift = 2;
  for (int i=0; i<4; i++) {
    bool b = channels[i];
    if (b)
      val |= 1<<shift;
    shift+=4;
  }
  WriteRegister(cal_rg_config, val);
}

void IpimBoard::SetChargeAmplifierRef(float refVoltage) {
  //Adjust the reference voltage for the charge amplifier
  unsigned i = unsigned((refVoltage/CHARGEAMP_REF_MAX)*(CHARGEAMP_REF_STEPS-1));
  if (i >= CHARGEAMP_REF_STEPS) {
    printf("IpimBoard error: calculated %d charge amp ref steps, allow %d; based on requested voltage %f\n", i, CHARGEAMP_REF_STEPS, refVoltage);
    //    _damage |= 1<<ampRef_damage; 
    _commandResponseDamage = true;
    return;
  }
  WriteRegister(bias_data, i);
}
//  raise RuntimeError, "Invalid charge amplifier reference of %fV, max is %fV" % (fRefVoltage, CHARGEAMP_REF_MAX);


void IpimBoard::SetCalibrationVoltage(float calibrationVoltage) {
  unsigned i = unsigned((calibrationVoltage/CALIBRATION_V_MAX)*(CALIBRATION_V_STEPS-1));
  if (i >= CALIBRATION_V_STEPS) {
    printf("IpimBoard error: invalid calibration bias of %fV, max is %fV", calibrationVoltage, CALIBRATION_V_MAX);
    _commandResponseDamage = true;
    return;
    //_damage |= calVoltage_damage;
  }
  //raise RuntimeError, 
  WriteRegister(cal_data, i);
  struct timespec req = {0, 10000000}; // 10 ms
  nanosleep(&req, NULL);
}

void IpimBoard::SetChargeAmplifierMultiplier(unsigned* channels) {
  unsigned val = 0;
  for (int i=0; i<4; i++) {
    val |= channels[i]<<i*4;
  }
  WriteRegister(rg_config, val);
}

void IpimBoard::oldSetChargeAmplifierMultiplier(unsigned* channels) {
  unsigned val = ReadRegister(rg_config);
  val &= 0x8888;
  int shift = 0;
  for (int i=0; i<4; i++) {
    unsigned setting = channels[i];
    if (setting<=1)
      val |= 4<<shift;
    else if (setting<=100)
      val |= 2<<shift;
    else if (setting<=10000)
      val |= 1<<shift;
    shift+=4;
  }
  WriteRegister(rg_config, val);
}

void IpimBoard::SetInputBias(float biasVoltage) {
  unsigned i = unsigned((biasVoltage/INPUT_BIAS_MAX)*(INPUT_BIAS_STEPS-1));
  if (i >= INPUT_BIAS_STEPS) {
    printf("IpimBoard error: calculated input bias setting register setting %d, allow %d; based on requested voltage %f\n", i, INPUT_BIAS_STEPS, biasVoltage);
    _commandResponseDamage = true;
    return;
    //    _damage |= 1<<diodeBias_damage;
  }
  unsigned originalSetting = ReadRegister(biasdac_data_config);
  WriteRegister(biasdac_data_config, i);
  if (i != originalSetting) {
    printf("Have changed input bias setting from 0x%x to 0x%x, pausing to allow diode bias to settle\n", (int)originalSetting, i);
    struct timespec req = {0, 999999999}; // 1 s
    for (int j=0; j<5; j++) {
      nanosleep(&req, NULL);
    }
  }
}

void IpimBoard::SetChannelAcquisitionWindow(uint32_t acqLength, uint16_t acqDelay) {
  uint32_t length = (acqLength+CLOCK_PERIOD-1)/CLOCK_PERIOD;
  if (length > 0xfffff) {
    printf("IpimBoard error: acquisition window cannot be more than %dns\n", (0xfffff*CLOCK_PERIOD));
    _commandResponseDamage = true;
    return;
    //    _damage |= 1 << acqWindow_damage;
  } 
  unsigned delay = (acqDelay+CLOCK_PERIOD-1)/CLOCK_PERIOD;
  if (delay > 0xfff) {
    printf("IpimBoard warning: acquisition window cannot be delayed more than %dns\n", 0xfff*CLOCK_PERIOD);
    _commandResponseDamage = true;
    return;
  }
  WriteRegister(reset, (length<<12) | (delay & 0xfff));
}

void IpimBoard::SetTriggerDelay(uint32_t triggerDelay) {
  unsigned delay = (triggerDelay+CLOCK_PERIOD-1)/CLOCK_PERIOD;
  if (delay > 0xffff) {
    printf("IpimBoard warning: trigger delay cannot be more than %dns\n", 0xffff*CLOCK_PERIOD);
    _commandResponseDamage = true;
    return;
  }
  WriteRegister(trig_delay, delay);
}

void IpimBoard::_SetAdcDelay(uint32_t adcDelay) {
  unsigned delay = (adcDelay+CLOCK_PERIOD-1)/CLOCK_PERIOD;
  if (delay > 0xffff) {
    printf("IpimBoard warning: adc delay cannot be more than %dns\n", 0xffff*CLOCK_PERIOD);
    _commandResponseDamage = true;
    return;
  }
  WriteRegister(adc_delay, delay);
}

void IpimBoard::SetTriggerPreSampleDelay(uint32_t triggerPsDelay) {
  unsigned delay = (triggerPsDelay+CLOCK_PERIOD-1)/CLOCK_PERIOD;
  if (delay > 0xffff) {
    printf("IpimBoard warning: trigger presample delay cannot be more than %dns\n", 0xffff*CLOCK_PERIOD);
    _commandResponseDamage = true;
    return;
  }
  WriteRegister(trig_ps_delay, delay);
}

void IpimBoard::CalibrationStart(unsigned calStrobeLength) { //=0xff):
  unsigned length = (calStrobeLength+CLOCK_PERIOD-1)/CLOCK_PERIOD;
  if (length > 0xffff) {
    printf("IpimBoard error: strobe cannot be more than %dns", 0xffff*CLOCK_PERIOD);
    _commandResponseDamage = true;
    return;
  }
  WriteRegister(cal_strobe, length);
}

unsigned IpimBoard::ReadRegister(unsigned regAddr) {
  IpimBoardCommand cmd = IpimBoardCommand(false, regAddr, 0);
  WriteCommand(cmd.getAll());
  _commandIndex = 0;
  IpimBoardPacketParser packetParser = IpimBoardPacketParser(true, &_commandResponseDamage, _commandList);
  struct timespec req = {0, 4000000}; // 4 ms - board takes a while to respond
  nanosleep(&req, NULL);
  int nParses = 0;
  while (inWaiting(packetParser)) {
    if (!packetParser.packetIncomplete()) {
      //      printf("while executes once regardless?\n");
      break;
    }

    nanosleep(&req, NULL);
    if (nParses++ == 10 and inWaiting(packetParser)) {
      _commandResponseDamage = true;
      return 0xdead;
    }
  }
  IpimBoardResponse resp = IpimBoardResponse(_commandList);
  if (!resp.CheckCRC()) {
    printf("IpimBoard warning: response CRC check failed in reading register 0x%x\n", regAddr);
    _commandResponseDamage = true;
  }
  return resp.getData();
}

void IpimBoard::WriteRegister(unsigned regAddr, unsigned regValue) {
  IpimBoardCommand cmd = IpimBoardCommand(true, regAddr, regValue);
  WriteCommand(cmd.getAll());
  unsigned result = ReadRegister(regAddr);
//printf(" *** regAddr = 0x%x\n", regAddr);
//printf(" *** status   = 0x%x\n", status);
//printf(" *** errors   = 0x%x\n", errors);
//printf(" *** result   = 0x%x\n", result);
  if (regAddr != status and regAddr != errors and regAddr != adc_delay) {
    if (result != regValue) {
      printf("IpimBoard error writing register 0x%x, wrote 0x%x, read 0x%x\n", regAddr, regValue, result);
      _commandResponseDamage = true;
    }
  }
}

IpimBoardData IpimBoard::WaitData() {
  IpimBoardPacketParser packetParser = IpimBoardPacketParser(false, &_dataDamage, _dataList);
  int nTries = 0;
  while (inWaiting(packetParser)) {
    nTries++;
    if (nTries==0) {
      unsigned fakeData = 0xdead;
      int packetsRead = packetParser.packetsRead();
      if (packetParser.packetIncomplete()) {
	_dataDamage = packetIncomplete;
	for (int i=packetsRead; i< DataPackets; i++) {
	  _dataList[i] = fakeData;
	}
	if (DEBUG or KvetchCounter++<AllowedKvetches) {
	  printf("IpimBoard warning: have failed to find expected event in %d * 6 ms, attempting to continue\n", nTries);
	}
	break;
      }
    }
  }

  IpimBoardPsData data = IpimBoardPsData(_dataList, _baselineSubtraction, _polarity, _history, _dataDamage);
  int c = data.CheckCRC();
  if (!c) {
    if (DEBUG or KvetchCounter++<AllowedKvetches) {
      printf("IpimBoard warning: data CRC check failed\n");
    }
    _dataDamage = crcFailed;
  }
  return IpimBoardData(data);
}
int IpimBoard::dataDamage() {
  if (_dataDamage) {
    int tmp = _dataDamage;
    _dataDamage = 0;
    return tmp;
  }
  return _dataDamage;
}

bool IpimBoard::commandResponseDamaged() {
  return _commandResponseDamage>0;
}

void IpimBoard::WriteCommand(unsigned* cList) {
  unsigned count = 0;
  for (int i=0; i<4; i++) {
    unsigned data = *cList++;
    unsigned w0 = 0x90 | (data & 0xf);
    unsigned w1 = (data>>4) & 0x3f;
    unsigned w2 = 0x40 | ((data>>10) & 0x3f);
    if (count == 0)
      w0 |= 0x40;
    if (count == 3) {//bytes.size()-1) {
      w0 |= 0x20;
    }
    count++;
    //    char* bS = (char)((int)w0) + (char)((int)w1) + (char)((int)w2);
    char bufferSend[3];
    if (DEBUG)
      printf("Ser W: 0x%x 0x%x 0x%x\n", w0, w1, w2);
    bufferSend[0] = (char)((int)w0);
    bufferSend[1] = (char)((int)w1);
    bufferSend[2] = (char)((int)w2);
    int res = write(_fd, bufferSend, 3);
    if (res != 3) {
      if (DEBUG or KvetchCounter++<AllowedKvetches) {
	printf( "write failed: %d bytes of %s written\n", res, bufferSend);
      }
      _commandResponseDamage = true;
      //    } else {
      //      printf( "write passed: %d bytes of buff written\n", res);
    }
  }
}

int IpimBoard::inWaiting(IpimBoardPacketParser& packetParser) {
  char bufferReceive[3];
  int nBytes = 0;
  int nTries = 0;
  while (nBytes<3) {
    int nRead = read(_fd, &(bufferReceive[nBytes]), 3-nBytes);
    if (nRead<0) {
      printf("IpimBoard error:failed read from fd %d, device %s\n", _fd, _serialDevice);
      perror("read error: ");
      //      assert(0);
    }
    nBytes += nRead;
    if (nBytes!=3) {
      if (nTries++ == 3) {
	packetParser.readTimedOut(nBytes, _fd, _serialDevice);
	return packetParser.packetIncomplete();
      }      
      if (DEBUG) printf("IpimBoard warning: have read %d (%d) (total) bytes of data from _fd %d, need 3, will retry\n", nRead, nBytes,_fd);
      struct timespec req = {0, 2000000}; // 2 ms
      nanosleep(&req, NULL);
    }
  }
  packetParser.update(bufferReceive);
  return packetParser.packetIncomplete();
}

int IpimBoard::get_fd() {
  return _fd;
}

void IpimBoard::flush() {
  tcflush(_fd, TCIOFLUSH);
}

bool IpimBoard::configure(Ipimb::ConfigV2& config) {
  flush();
  setReadable(true);
  printf("have set fd to readable in IpimBoard configure\n");

  _commandResponseDamage = false;
  WriteRegister(errors, 0xffff);

  unsigned vhdlVersion = ReadRegister(vhdl_version);

  if (_commandResponseDamage) {
    printf("read of board failed - check serial and power connections of fd %d, device %s\nGiving up on configuration\n", _fd, _serialDevice);
    return !_commandResponseDamage; // bail if can't read
  }

  // hope to never want to calibrate in the daq environment
  bool lstCalibrateChannels[4] = {false, false, false, false};
  SetCalibrationMode(lstCalibrateChannels);
  
  //  config.dump();
  unsigned start0 = (unsigned) (0xFFFFFFFF&config.triggerCounter());
  unsigned start1 = (unsigned) (0xFFFFFFFF00000000ULL&config.triggerCounter()>>32);
  SetTriggerCounter(start0, start1);

  SetChargeAmplifierRef(config.chargeAmpRefVoltage());
  // only allow one charge amp capacitance setting
  
  if (!_c01) {// c02 is default
    if (vhdlVersion != NewVhdlVersion) {
      printf("Expected vhdl version 0x%x, found 0x%x, old version is 0x%x\n", NewVhdlVersion, vhdlVersion, OldVhdlVersion);
      _commandResponseDamage = true;
    }
    unsigned multiplier = (unsigned) config.chargeAmpRange();
    unsigned mult_chan0 = ((multiplier&0xf)+1)%0xf;  // Board interprets 0 as no capacitor
    unsigned mult_chan1 = (((multiplier>>4)&0xf)+1)%0xf;
    unsigned mult_chan2 = (((multiplier>>8)&0xf)+1)%0xf;
    unsigned mult_chan3 = (((multiplier>>12)&0xf)+1)%0xf;
    printf("Configuring gain with %d, %d, %d, %d\n", mult_chan0, mult_chan1, mult_chan2, mult_chan3);
    if(mult_chan0*mult_chan1*mult_chan2*mult_chan3 == 0) {
      printf("Do not understand gain configuration 0x%x, byte values restricted to 0-14\n", multiplier);
      _commandResponseDamage = true;
    }
    unsigned inputAmplifier_pF[4] = {mult_chan0, mult_chan1, mult_chan2, mult_chan3};
    SetChargeAmplifierMultiplier(inputAmplifier_pF);
  } else {
    if (vhdlVersion != OldVhdlVersion) {
      printf("Expected vhdl version 0x%x, found 0x%x, new version is 0x%x\n", OldVhdlVersion, vhdlVersion, NewVhdlVersion);
      _commandResponseDamage = true;
    }
    unsigned multiplier = (unsigned) config.chargeAmpRange();
    unsigned mapping[16] = {1, 100, 10000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0};
    unsigned mult_chan0 = mapping[multiplier&0xf];
    unsigned mult_chan1 = mapping[(multiplier>>4)&0xf];
    unsigned mult_chan2 = mapping[(multiplier>>8)&0xf];
    unsigned mult_chan3 = mapping[(multiplier>>12)&0xf];
    printf("Configuring gain with %d, %d, %d, %d\n", mult_chan0, mult_chan1, mult_chan2, mult_chan3);
    if(mult_chan0*mult_chan1*mult_chan2*mult_chan3 == 0) {
      printf("Do not understand bit pattern of 0x%x gain configuration, only 0, 1, 2 allowed per byte\n", multiplier);
      _commandResponseDamage = true;
    }
    unsigned inputAmplifier_pF[4] = {mult_chan0, mult_chan1, mult_chan2, mult_chan3};
    oldSetChargeAmplifierMultiplier(inputAmplifier_pF);
  }
  //  SetCalibrationVoltage(float calibrationVoltage);
  SetInputBias(config.diodeBias());
  SetChannelAcquisitionWindow(config.resetLength(), config.resetDelay());
  SetTriggerDelay((unsigned) config.trigDelay());
  SetTriggerPreSampleDelay(HardCodedPresampleDelay);
  printf("Have hardcoded presample delay to %d us\n", HardCodedPresampleDelay/1000);
  if (!_c01) {
    if ((HardCodedPresampleDelay+HardCodedTotalPresampleTime)>config.trigDelay()) {
      _commandResponseDamage = true;
      printf("Sample delay %d is too short - must be at least %d earlier than %d\n", config.trigDelay(), HardCodedTotalPresampleTime, HardCodedPresampleDelay);
    }
    _SetAdcDelay(HardCodedAdcDelay);
    printf("Have hardcoded adc delay to %f us\n", float(HardCodedAdcDelay)/1000);
  } else {
    if ((HardCodedPresampleDelay+OldHardCodedADCTime)>config.trigDelay()) {
      _commandResponseDamage = true;
      printf("Sample delay %d is too short - must be at least %d earlier than %d\n", config.trigDelay(), OldHardCodedADCTime, HardCodedPresampleDelay);
    }
  }
  //  CalibrationStart((unsigned) config.calStrobeLength());

  config.setTriggerPreSampleDelay(HardCodedPresampleDelay);
  config.setAdcDelay(HardCodedAdcDelay);

  config.setSerialID(GetSerialID());
  config.setErrors(GetErrors());
  config.setStatus((ReadRegister(status) & 0xffff0000)>>16);
  
  config.dump();

  flush();
  printf("have flushed fd after IpimBoard configure, damage set to %s\n", (_commandResponseDamage) ? "true" : "false");
  //  setReadable(false); // test hack
  //  flush();

  return !_commandResponseDamage;
}

bool IpimBoard::unconfigure() {
  bool state = setReadable(false);
  flush();
  return state;
}

bool IpimBoard::setReadable(bool flag) {
  struct termios oldtio;
  tcgetattr(_fd,&oldtio);
  //  printf("found c_flag 0x%x, will set bit to %d\n", oldtio.c_cflag, int(flag));
  oldtio.c_cflag |= CREAD;
  if (!flag) {
    oldtio.c_cflag ^= CREAD;
    //    printf("set c_flag 0x%x\n", oldtio.c_cflag);
  }
  tcsetattr(_fd,TCSANOW,&oldtio);
  return true; // do something smarter here if possible
}

void IpimBoard::setBaselineSubtraction(const int baselineSubtraction, const int polarity) {
  _baselineSubtraction = baselineSubtraction;
  _polarity = polarity;
}

void IpimBoard::setOldVersion() {_c01 = true;}

IpimBoardCommand::IpimBoardCommand(bool write, unsigned address, unsigned data) {
  _commandList[0] = address & 0xFF;
  _commandList[1] = data & 0xFFFF;
  _commandList[2] = (data >> 16) & 0xFFFF;
  if (write) {
    _commandList[0] |= 1<<8;
  }
  _commandList[3] = CRC(_commandList, CRPackets-1);
  if (DEBUG)
    printf("data: 0x%x, address 0x%x, write %d, command list: 0x%x, 0x%x, 0x%x 0x%x\n", 
	   data, address, write, _commandList[0], _commandList[1], _commandList[2], _commandList[3]);
}

IpimBoardCommand::~IpimBoardCommand() {
  //  delete[] _lst;
}

unsigned* IpimBoardCommand::getAll() {
  return _commandList;
}


IpimBoardResponse::IpimBoardResponse(unsigned* packet) {
  _addr = 0;
  _data = 0;
  _checksum = 0;
  setAll(0, CRPackets, packet);
}

IpimBoardResponse::~IpimBoardResponse() {
  //   printf("IPBMR::dtor, this = %p\n", this);
}

void IpimBoardResponse::setAll(int i, int j, unsigned* s) { // slice
  for (int count=i; count<j; count++) {
    if (count==0) {
      _addr = s[count-i] & 0xFF;
    }
    else if (count==1) {
      _data = (_data & 0xFFFF0000L) | s[count-i];
    }
    else if (count==2) {
      _data = (_data & 0xFFFF) | ((unsigned)s[count-i]<<16);
    }
    else if (count==3) {
      _checksum = s[count-i];
    }
    else {
      if (DEBUG or KvetchCounter++<AllowedKvetches) {
	printf("IpimBoard error: resp list broken\n");
      }
      //      _commandResponseDamage = true;
    }
  }
}

unsigned* IpimBoardResponse::getAll(int i, int j) { // slice
  for (int count=i; count<j; count++) {
    if (count==0) {
      _respList[0] = _addr & 0xFF;
    }
    else if (count==1) {
      _respList[1] = _data & 0xFFFF;
    }
    else if (count==2) {
      _respList[2] = _data>>16;
    }
    else if (count==3) {
      _respList[3] = _checksum;
    }
  }
  return _respList;
}

unsigned IpimBoardResponse::getData() { // slice
  return _data;
}

bool IpimBoardResponse::CheckCRC() {
  if (CRC(getAll(0, CRPackets-1), CRPackets-1) == _checksum){
    return true;
  }
  return false;
}

IpimBoardBaselineHistory::IpimBoardBaselineHistory() {
  memset(baselineBuffer, 0, sizeof(double)*NChannels*BufferLength);
  nUpdates = 0;
  bufferLength = 0;
  memset(baselineRunningAverage, 0, sizeof(double)*NChannels);
}


IpimBoardPsData::IpimBoardPsData(unsigned* packet, const int baselineSubtraction, const int polarity, IpimBoardBaselineHistory* history, int &dataDamage) {
  _triggerCounter = 0;
  _config0 = 0;
  _config1 = 0;
  _config2 = 0;
  _ch0 = 0;
  _ch1 = 0;
  _ch2 = 0;
  _ch3 = 0;
  _ch0_ps = 0;
  _ch1_ps = 0;
  _ch2_ps = 0;
  _ch3_ps = 0;
  _checksum = 0;
  _dataDamage = dataDamage;
  _history = history;
  _baselineSubtraction = baselineSubtraction;
  _polarity = polarity; // input is 0 or 1, use -1 or 1
  if(polarity == 0) _polarity = -1;

  setAll(0, DataPackets, packet);  
  updateBaselineHistory();
}

IpimBoardPsData::IpimBoardPsData() { // for IpimbServer setup
  printf("actually use default constructor, don't kill\n");
  _triggerCounter = 0;//-1;
  _config0 = 0;
  _config1 = 0;
  _config2 = 0;
  _ch0 = 0;
  _ch1 = 0;
  _ch2 = 0;
  _ch3 = 0;
  _ch0_ps = 0;
  _ch1_ps = 0;
  _ch2_ps = 0;
  _ch3_ps = 0;
  _checksum = 0;//12345;
}


void IpimBoardPsData::dumpRaw() {
  printf("trigger counter:0x%8.8x\n", (unsigned)_triggerCounter);//%llu\n", _timestamp);
  printf("config0: 0x%2.2x\n", _config0);
  printf("config1: 0x%2.2x\n", _config1);
  printf("config2: 0x%2.2x\n", _config2);
  printf("channel0: 0x%02x\n", _ch0);
  printf("channel1: 0x%02x\n", _ch1);
  printf("channel2: 0x%02x\n", _ch2);
  printf("channel3: 0x%02x\n", _ch3);
  printf("channel0_ps: 0x%02x\n", _ch0_ps);
  printf("channel1_ps: 0x%02x\n", _ch1_ps);
  printf("channel2_ps: 0x%02x\n", _ch2_ps);
  printf("channel3_ps: 0x%02x\n", _ch3_ps);
  printf("checksum: 0x%02x\n", _checksum);
}

void IpimBoardPsData::setAll(int i, int j, unsigned* s) { // slice
  for (int count=i; count<j; count++) {
    if (count==0) {
      _triggerCounter = (_triggerCounter & 0x0000FFFFFFFFFFFFLLU) | ((long long)(s[count-i])<<48);
    }
    else if (count==1) {
      _triggerCounter = (_triggerCounter & 0xFFFF0000FFFFFFFFLLU) | ((long long)(s[count-i])<<32);
    }
    else if (count==2) {
      _triggerCounter = (_triggerCounter & 0xFFFFFFFF0000FFFFLLU) | (s[count-i]<<16);
    }
    else if (count==3) {
      _triggerCounter = (_triggerCounter & 0xFFFFFFFFFFFF0000LLU) | s[count-i];
    }    
    else if (count==4) {
      _config0 = s[count-i];
    }    
    else if (count==5) {
      _config1 = s[count-i];
    }
    else if (count==6) {
      _config2 = s[count-i];
    }
    else if (count==7) {
      _ch0 = s[count-i];
    }
    else if (count==8) {
      _ch1 = s[count-i];
    }
    else if (count==9) {
      _ch2 = s[count-i];
    }
    else if (count==10) {
      _ch3 = s[count-i];
    }
    else if (count==11) {
      _ch0_ps = s[count-i];
    }
    else if (count==12) {
      _ch1_ps = s[count-i];
    }
    else if (count==13) {
      _ch2_ps = s[count-i];
    }
    else if (count==14) {
      _ch3_ps = s[count-i];
    }
    else if (count==DataPackets-1) {
      _checksum = s[count-i];
    }
    else {
      if (DEBUG or KvetchCounter++<AllowedKvetches) {
	printf("IpimBoard error: data list parsing broken\n");
	//	_dataDamage = true; 	
      }
    }
  }
  if(DumpSampleEvents and _triggerCounter%1000==0) {
    dumpRaw();
  }
}

void IpimBoardPsData::setList(int i, int j, unsigned* lst) { // slice - already _dataList?
  for (int count=i; count<j; count++) {
    if (count==0) {
      lst[count] = _triggerCounter>>48;
    }
    else if (count==1) {
      lst[count] = (_triggerCounter>>32) & 0xFFFF;
    }
    else if (count==2) {
      lst[count] = (_triggerCounter>>16) & 0xFFFF;
    }
    else if (count==3) {
      lst[count] = _triggerCounter & 0xFFFF;
    }
    else if (count==4) {
      lst[count] = _config0;
    }
    else if (count==5) {
      lst[count] = _config1;
    }
    else if (count==6) {
      lst[count] = _config2;
    }
    else if (count==7) {
      lst[count] = _ch0;
    }
    else if (count==8) {
      lst[count] = _ch1;
    }
    else if (count==9) {
      lst[count] = _ch2;
    }
    else if (count==10) {
      lst[count] = _ch3;
    }
    else if (count==11) {
      lst[count] = _ch0_ps;
    }
    else if (count==12) {
      lst[count] = _ch1_ps;
    }
    else if (count==13) {
      lst[count] = _ch2_ps;
    }
    else if (count==14) {
      lst[count] = _ch3_ps;
    }
    else if (count==DataPackets-1) {
      lst[count] = _checksum;
    }
    else {
      if (DEBUG or KvetchCounter++<AllowedKvetches) {
	printf("IpimBoard error: data list retrieval broken\n");
      //      _dataDamage = true;
      }
    }
  }
}

bool IpimBoardPsData::CheckCRC() {
  unsigned lst[DataPackets];
  setList(0, DataPackets-1, lst);
  unsigned crc = CRC(lst, DataPackets-1);
  if (crc == _checksum){
    return true;
  }
  return false;
}

void IpimBoardPsData::updateBaselineHistory() {
  _presampleChannel[0] = _ch0_ps;
  _presampleChannel[1] = _ch1_ps;
  _presampleChannel[2] = _ch2_ps;
  _presampleChannel[3] = _ch3_ps;
  _sampleChannel[0] = _ch0;
  _sampleChannel[1] = _ch1;
  _sampleChannel[2] = _ch2;
  _sampleChannel[3] = _ch3;
  for (int i=0; i<NChannels; i++) {
    _history->baselineBuffer[i][_history->nUpdates%BufferLength] = _presampleChannel[i];
    double oldAverage = _history->baselineRunningAverage[i];
    double delta = std::abs(oldAverage -_presampleChannel[i]);
    if (oldAverage > 0 and delta>BaselineShiftTolerance) {
      _dataDamage = baselineShift;
      if (DEBUG or BaselineKvetchCounter++<AllowedKvetches) {
	printf("Running baseline average for channel %d is %f, new baseline value %f is larger than %f away\n", i, oldAverage, _presampleChannel[i], BaselineShiftTolerance);
      }
    }
    _history->baselineRunningAverage[i] = (_history->nUpdates*_history->baselineRunningAverage[i] + _presampleChannel[i])/float(_history->nUpdates+1);
  }
  if (_history->bufferLength < 100)
    _history->bufferLength++;
  
  for (int i=0; i<NChannels; i++) {
    double sum = 0;
    for (int j=0; j<_history->bufferLength; j++) {
      sum += _history->baselineBuffer[i][j];
    }
    _history->baselineBufferedAverage[i] = sum/_history->bufferLength;
  }
  _history->nUpdates++;
}

uint64_t IpimBoardPsData::GetTriggerCounter() {
  return _triggerCounter;
}
unsigned IpimBoardPsData::GetTriggerDelay_ns() {
  return _config2*CLOCK_PERIOD;
}
uint16_t IpimBoardPsData::GetCh(int i) {
  double presample = 0;
  if(_baselineSubtraction == 1) {
    presample = _presampleChannel[i];
  } else if (_baselineSubtraction == 2) {
    presample = _history->baselineBufferedAverage[i];
  } else if (_baselineSubtraction == 3) {
    presample = _history->baselineRunningAverage[i];
  }
  if (_baselineSubtraction != 0) {
    // polarity -1 for negative-going signal (default)
    // ADC maximum is ADC_STEPS - 1
    int diff = max(0, int(-1*_polarity*presample + _polarity*_sampleChannel[i])); // guard against overflow due to noise
    if (_polarity == 1) {
      return diff;
    } else {
      return ADC_STEPS - 1 - diff; // 0xffff if diff is zero (negative-going signal)
    }
  }
  return (uint16_t) _sampleChannel[i];
}

uint16_t IpimBoardPsData::GetCh_ps(int i) {
  return (uint16_t) _presampleChannel[i];
}

unsigned IpimBoardPsData::GetConfig0() {
  return _config0;
}
unsigned IpimBoardPsData::GetConfig1() {
  return _config1;
}
unsigned IpimBoardPsData::GetConfig2() {
  return _config2;
}
unsigned IpimBoardPsData::GetChecksum() {
    return 0; // checksum has no meaning for baseline-subtracted data or unsubtracted data without baseline data
}


IpimBoardData::IpimBoardData(IpimBoardPsData data) {
  _triggerCounter = data.GetTriggerCounter();
  _config0 = data.GetConfig0();
  _config1 = data.GetConfig1();
  _config2 = data.GetConfig2();
  _ch0 = data.GetCh(0);
  _ch1 = data.GetCh(1);
  _ch2 = data.GetCh(2);
  _ch3 = data.GetCh(3);
  _ch0_ps = data.GetCh_ps(0);
  _ch1_ps = data.GetCh_ps(1);
  _ch2_ps = data.GetCh_ps(2);
  _ch3_ps = data.GetCh_ps(3);
  _checksum = data.GetChecksum();
 }

uint64_t IpimBoardData::GetTriggerCounter() {
  return _triggerCounter;
}
unsigned IpimBoardData::GetTriggerDelay_ns() {
  return _config2*CLOCK_PERIOD;
}
/*
float IpimBoardData::GetCh0_V() {
  return (float(_ch0)*ADC_RANGE)/(ADC_STEPS-1);
}
float IpimBoardData::GetCh1_V() {
  return (float(_ch1)*ADC_RANGE)/(ADC_STEPS-1);
}
float IpimBoardData::GetCh2_V() {
  return (float(_ch2)*ADC_RANGE)/(ADC_STEPS-1);
}
float IpimBoardData::GetCh3_V() {
  return (float(_ch3)*ADC_RANGE)/(ADC_STEPS-1);
}
*/
unsigned IpimBoardData::GetConfig0() {
  return _config0;
}
unsigned IpimBoardData::GetConfig1() {
  return _config1;
}
unsigned IpimBoardData::GetConfig2() {
  return _config2;
}


IpimBoardPacketParser::IpimBoardPacketParser(bool command, int* damage, unsigned* lst):
  _command(command), _damage(damage), _lst(lst),
  _nPackets(0), _allowedPackets(0), _leadHeaderNibble(0), _bodyHeaderNibble(0), _tailHeaderNibble(0) {
  _lastDamaged = *_damage;
  //  *_damage = false;
  if (!command) {
    _allowedPackets = DataPackets;
    _leadHeaderNibble = 0xc;
    _bodyHeaderNibble = 0x8;
    _tailHeaderNibble = 0xa;
  } else {
    _allowedPackets = CRPackets;
    _leadHeaderNibble = 0xd;
    _bodyHeaderNibble = 0x9;
    _tailHeaderNibble = 0xb;
  }    
}

void IpimBoardPacketParser::update(char* buff) {
  int w0, w1, w2;
  w0 = buff[0] & 0xff;
  w1 = buff[1] & 0xff;
  w2 = buff[2] & 0xff;
  if (_nPackets == 0) {
    if ((w0>>4) != _leadHeaderNibble) {
      if (!_lastDamaged) {
	*_damage = true;
	if (DEBUG or KvetchCounter++<AllowedKvetches) {
	  printf("IpimBoard warning: read initial packet with bad header, expected 0x%xnnnnn, got 0x%x%x%x\n", _leadHeaderNibble, w0, w1, w2);
	}
      } else {
	if (DEBUG or KvetchCounter++<AllowedKvetches) {
	  printf("IpimBoard warning: previous message damaged, demand correct header; expected 0x%xnnnnn, got 0x%x%x%x; dropping packet to reframe\n", _leadHeaderNibble, w0, w1, w2);
	}
	return;
      }
    }
  } else if (_nPackets < _allowedPackets -1) {
    if ((w0>>4) != _bodyHeaderNibble) {
      *_damage = true;
      if (DEBUG or KvetchCounter++<AllowedKvetches) {
	printf("IpimBoard warning: read packet with bad header, expected 0x%xnnnnn, got 0x%x%x%x\n", _bodyHeaderNibble, w0, w1, w2);
      }
    }
  } else if (_nPackets == _allowedPackets -1) {
    if ((w0>>4) != _tailHeaderNibble) {
      *_damage = true;
      if (DEBUG or KvetchCounter++<AllowedKvetches) {
	printf("IpimBoard warning: read trailing packet with bad header, expected 0x%xnnnnn, got 0x%x%x%x\n", _tailHeaderNibble, w0, w1, w2);
      }
    }
  }
  if (_nPackets < _allowedPackets) {
    _lst[_nPackets++] = (w0 & 0xf) | ((w1 & 0x3f)<<4) | ((w2 & 0x3f)<<10); // strip header bits
  } else {
    *_damage = true;
    if (DEBUG or KvetchCounter++<AllowedKvetches) {
      printf("IpimBoard error parsing type %d packet, have found %d packets, %d allowed\n", _command, _nPackets, _allowedPackets);
    }
  }
  //  if (!_command && (rand()%1000)<1) {
  //    printf("setting fake damage now\n");
  //    *_damage = true;
  //  }
}

bool IpimBoardPacketParser::packetIncomplete() {
  return _nPackets != _allowedPackets;
}

void IpimBoardPacketParser::readTimedOut(int nBytes, int fd, char* serialDevice) {
  *_damage = true;
  if (DEBUG or KvetchCounter++<AllowedKvetches) {
    printf("IpimBoard error: read only %d bytes of required 3 from fd %d, device %s, timed out after %d packets of %d, should probably flush\n", nBytes, fd, serialDevice, _nPackets, _allowedPackets);
  }
}

int IpimBoardPacketParser::packetsRead() {
  return _nPackets;
}
