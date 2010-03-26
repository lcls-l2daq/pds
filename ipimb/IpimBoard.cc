#include "IpimBoard.hh"
#include "unistd.h" //for sleep
#include <iostream>
//#include "assert.h"
#include "memory.h"
// for non-blocking attempt
#include <fcntl.h>

// include home-brewed list if need be

//#include "pdsdata/ipimb/ConfigV1.hh"

using namespace std;
using namespace Pds;

const float CHARGEAMP_REF_MAX = 2.5;
const unsigned CHARGEAMP_REF_STEPS = 65536;

const float CALIBRATION_V_MAX = 5;
const unsigned CALIBRATION_V_STEPS = 65536;

const float INPUT_BIAS_MAX = 100;
const unsigned INPUT_BIAS_STEPS = 65536;

const int CLOCK_PERIOD = 8;
const float ADC_RANGE = 3.3;
const unsigned ADC_STEPS = 65536;

const int MAX_COMMANDS_AND_DATA = 100;
//const int MAX_DATA = 100;

const bool DEBUG = false;//true;

namespace Pds {

  void list2Array(UnsignedList lst, unsigned* s) {
    UnsignedList::iterator p = lst.begin();
    int i = 0;
    while (p != lst.end()) {
      //      printf("%d 0x%x ", i, *p);
      s[i++] = *p;
      //    cout << "in l2A: " << *p << endl;
      p++;
    }
    //    printf("\n");
  }

  unsigned CRC(UnsignedList lst) {
    unsigned crc = 0xffff;
    UnsignedList::iterator p = lst.begin();
    while (p != lst.end()) {
      unsigned word = *p++;
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

IpimBoard::IpimBoard(char* serialDevice){
  struct termios newtio;
  memset(&newtio, 0, sizeof(newtio));
        
  //  /* open the device to be non-blocking (read will return immediatly) */
  _fd = open(serialDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (_fd <0) {
    perror(serialDevice); exit(-1); 
  }
  
  //  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  //  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;//ICANON;
  newtio.c_cc[VMIN]=0;
  newtio.c_cc[VTIME]=0;
  newtio.c_iflag = 0;
  //  printf("cflag 0x%x, iflag %d, oflag %d\n", newtio.c_cflag, newtio.c_iflag, newtio.c_oflag);
  //  printf("iflag %d, oflag %d, cflag %d, lflag %d, ispeed %d, ospeed %d\n", newtio.c_iflag, newtio.c_oflag, newtio.c_cflag, newtio.c_lflag, newtio.c_ispeed, newtio.c_ospeed);
  tcflush(_fd, TCIFLUSH);
  tcsetattr(_fd,TCSANOW,&newtio);
}

IpimBoard::~IpimBoard() {
  //  printf("IPBM::dtor, this = %p\n", this);
  //  _ser.close();
  // delete _lstData;
  // delete _lstCommands;
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

void IpimBoard::SetCalibrationMode(bool* channels) {
  unsigned val = ReadRegister(rg_config);
  //  printf("have retrieved config val 0x%x\n", val);
  val &= 0x7777;
  int shift = 3;
  for (int i=0; i<4; i++) {
    bool b = channels[i];
    if (b) 
      val |= 1<<shift;
    shift += 4;
  }
  WriteRegister(rg_config, val);
}

void IpimBoard::SetCalibrationDivider(unsigned* channels) {
  unsigned val = ReadRegister(cal_rg_config);
  val &= 0x8888;
  int shift = 0;
  for (int i=0; i<4; i++) {
    unsigned setting = channels[i];
    if (setting<=1)
      val |= 1<<shift;
    else if (setting<=100)
      val |= 2<<shift;
    else if (setting<=10000)
      val |= 4<<shift;
    shift+=4;
  }
  WriteRegister(cal_rg_config, val);
}

void IpimBoard::SetCalibrationPolarity(bool* channels) {
  unsigned val = ReadRegister(cal_rg_config);
  val &= 0x7777;
  int shift = 3;
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

void IpimBoard::SetTriggerDelay(unsigned triggerDelay) {
  unsigned delay = (triggerDelay+CLOCK_PERIOD-1)/CLOCK_PERIOD;
  if (delay > 0xffff) {
    printf("IpimBoard warning: trigger delay cannot be more than %dns\n", 0xffff*CLOCK_PERIOD);
    _commandResponseDamage = true;
    return;
  }
  WriteRegister(trig_delay, delay);
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
  while (inWaiting(true) < 4) {
    ;//timeout here?
  }
  IpimBoardResponse resp = IpimBoardResponse(Read(true, 4));
  if (!resp.CheckCRC()) {
    printf("IpimBoard warning: data CRC check failed\n");
    _commandResponseDamage = true;
  }
  return resp.getData();
}

void IpimBoard::WriteRegister(unsigned regAddr, unsigned regValue) {
  IpimBoardCommand cmd = IpimBoardCommand(true, regAddr, regValue);
  WriteCommand(cmd.getAll());
}

IpimBoardData IpimBoard::WaitData(int timeout_us, bool& success) {
  _dataDamage = false;
  timespec t0, t1;
  clock_gettime(CLOCK_REALTIME, &t0);
  
  while (inWaiting(false) < 12) {
    clock_gettime(CLOCK_REALTIME, &t1);
    timespec deltaT = diff(t0, t1);
    if (deltaT.tv_sec > (float(timeout_us)/1000000)) {
      printf("IpimBoard warning: have timed out for now after %f s\n", double(deltaT.tv_sec));
      _dataDamage = true;
      return IpimBoardData();
      // or
      int n = inWaiting(false);
      unsigned fakeData = 0xdead;
      if (n < 12) {
	for (int i=n; i< 12; i++) {
	  _lstData.push_back(fakeData);
	}
      }
    }
  }

  IpimBoardData data = IpimBoardData(Read(false, 12));
  int c = data.CheckCRC();
  if (!c) {
    printf("IpimBoard warning: data CRC check failed\n");
    _dataDamage = true;
    //    _damage |= 1<<crc_damage;
  }
  else {
    success = true;
  }
  return data;
}

bool IpimBoard::dataDamaged() {
  return _dataDamage;
}

bool IpimBoard::commandResponseDamaged() {
  return _commandResponseDamage;
}

UnsignedList IpimBoard::Read(bool command, unsigned nBytes) {
  //unsigned lst[MAX_COMMANDS_AND_DATA];
  UnsignedList lst;
  if (command) {
    while (_lstCommands.size() < nBytes)
      inWaiting(command);
    for (unsigned i=0; i<nBytes; i++) {
      //      lst.append(self.lstCommands.pop(0))
      unsigned com = _lstCommands.front();
      _lstCommands.pop_front();
      lst.push_back(com);
    }
  }
  else {    
    while (_lstData.size() < nBytes)
      inWaiting(command);
    for (unsigned i=0; i<nBytes; i++) {
      //      lst.append(self.lstCommands.pop(0))
      unsigned data = _lstData.front();
      _lstData.pop_front();
      lst.push_back(data);
    }
  }
  return lst;//list2Array(lst);
}

void IpimBoard::WriteCommand(UnsignedList bytes) {
  unsigned count = 0;
  //  string str = "";
  UnsignedList::iterator p = bytes.begin();
  while (p != bytes.end()) {
    unsigned data = *p++;
    unsigned w0 = 0x90 | (data & 0xf);
    unsigned w1 = (data>>4) & 0x3f;
    unsigned w2 = 0x40 | ((data>>10) & 0x3f);
    if (count == 0)
      w0 |= 0x40;
    if (count == bytes.size()-1) {
      w0 |= 0x20;
    }
    count++;
    //    char* bS = (char)((int)w0) + (char)((int)w1) + (char)((int)w2);
    char bufferSend[3];
    //    cout << "words to send:" << (char)((int)w0) << " " << (char)((int)w1) << " "<< (char)((int)w2) << endl; 
    //    cout << "words to send:" << w0 << " " << w1 << " "<< w2 <<endl; 
    //    cout << "words to send:" << w0 << " " << w1 << " "<< w2 <<endl; 
    if (DEBUG)
      printf("Ser W: 0x%x 0x%x 0x%x\n", w0, w1, w2);
    bufferSend[0] = (char)((int)w0);
    bufferSend[1] = (char)((int)w1);
    bufferSend[2] = (char)((int)w2);
    int res = write(_fd, bufferSend, 3);
    if (res != 3) {
      printf( "write failed: %d bytes of %s written\n", res, bufferSend);
      //    } else {
      //      printf( "write passed: %d bytes of buff written\n", res);
    }
  }
}

int IpimBoard::inWaiting(bool command) {
  char bufferReceive[12];
  int res = read(_fd, bufferReceive, 3);
  bufferReceive[res]=0; // for printf
  int n = res;
  //  if (n>0) {
    //    if (DEBUG)
    //      printf("buffer:n is: %s:%d\n", bufferReceive, res);
    //    if (DEBUG)
    //      printf("Received Message: %s\n", bufferReceive);
  //  }
  //  printf("have n = %d in inWaiting, ought to be multiple of 3\n", n);
  int w0, w1, w2;
  int index = 0;
  //  cout << "in inWaiting, n=" << n << endl;
  while (n >= 3) {
    w0 = bufferReceive[index] & 0xff;
    //    printf("n, index, w0: %d, %d, %x\n", n, index, w0);
    index += 1;
    if ((w0 & 0x80) == 0) {// Out of sync: bit 8 must be set
      n--;
      printf("IpimBoard warning: Ser R: %x, out of sync\n", w0);
      continue;
    }
    w1 = bufferReceive[index] & 0xff;
    w2 = bufferReceive[index+1] & 0xff;
    index += 2;
    unsigned data = (w0 & 0xf) | ((w1 & 0x3f)<<4) | ((w2 & 0x3f)<<10);
    if ((w0 & 0x10) != 0) {
      _lstCommands.push_back(data);
    }
    else {
      _lstData.push_back(data);
      //      printf("Data read : %x %x %x\n", w0, w1, w2);
    }
    if (DEBUG) {
      printf("Ser R: %x %x %x\n", w0, w1, w2);
    }
    n = n-3;
  }
  if (n!=0) {
    printf("IpimBoard warning: have n = %d after inWaiting, ought to be 0\n", n);
    //    int wNext = bufferReceive[index] & 0xff;
    //    printf("next R: %x\n", wNext);
    //    printf("try to exit\n");
    //    exit(-1);
    if (command) {
      printf("IpimBoard warning: extra bytes found parsing port while looking for command\n");
      _commandResponseDamage = true;
    } else {
      printf("IpimBoard warning: extra bytes found parsing port while looking for data\n");
      _dataDamage = true;
    }      
    //    _damage |= 1<<n0_damage;
  }
  //  assert(n==0);  // or buffer anything extra for next pass
  if (command) {
    if (DEBUG) 
      printf("returning command size %d\n", _lstCommands.size());
    return _lstCommands.size();
  }
  else {
    if (DEBUG)
      printf("returning data size %d\n", _lstData.size());
    return _lstData.size();
  }
}

int IpimBoard::get_fd() {
  return _fd;
}

void IpimBoard::flush() {
  tcflush(_fd, TCIFLUSH);
}

int IpimBoard::configure(const Ipimb::ConfigV1& config) {
  int errors = 0;
  // hope to never want to calibrate in the daq environment
  bool lstCalibrateChannels[4] = {false, false, false, false};
  SetCalibrationMode(lstCalibrateChannels);
  
  //  config.dump();
  unsigned start0 = (unsigned) (0xFFFFFFFF&config.triggerCounter());
  unsigned start1 = (unsigned) (0xFFFFFFFF00000000ULL&config.triggerCounter()>>32);
  SetTriggerCounter(start0, start1);
  //errors += 
  SetChargeAmplifierRef(config.chargeAmpRefVoltage());
  // only allow one charge amp capacitance setting
  unsigned multiplier = (unsigned) config.chargeAmpRange();
  unsigned inputAmplifier_pF[4] = {multiplier, multiplier, multiplier, multiplier};
  SetChargeAmplifierMultiplier(inputAmplifier_pF);
  //  SetCalibrationVoltage(float calibrationVoltage);
  SetInputBias(config.diodeBias());
  SetChannelAcquisitionWindow(config.resetLength(), config.resetDelay());
  SetTriggerDelay((unsigned) config.trigDelay());
  //  CalibrationStart((unsigned) config.calStrobeLength());

  tcflush(_fd, TCIFLUSH);
  printf("have flushed fd after IpimBoard configure\n");

  return errors;
}

IpimBoardCommand::IpimBoardCommand(bool write, unsigned address, unsigned data) {
  _lst.push_back(address & 0xFF);
  _lst.push_back(data & 0xFFFF);
  _lst.push_back((data >> 16) & 0xFFFF);
  _lst.push_back(0);
  if (write) {
    unsigned tmp = _lst.front();
    _lst.pop_front();
    _lst.push_front(tmp | 1<<8);
  }
  _lst.pop_back();
  _lst.push_back(CRC(_lst));
  unsigned s[_lst.size()];
  list2Array(_lst, s);
  if (DEBUG)
    printf("command list: 0x%x, 0x%x, 0x%x 0x%x\n", s[0], s[1], s[2], s[3]);
}

IpimBoardCommand::~IpimBoardCommand() {
  //  delete[] _lst;
}

UnsignedList IpimBoardCommand::getAll() {
  return _lst;
}


//IpimBoardResponse::IpimBoardResponse(unsigned* packet) {
IpimBoardResponse::IpimBoardResponse(UnsignedList packet) {
  //                if len(lstPacket) != 4:
  //                      raise RuntimeError, "Invalid response packet size %d" % (len(lstPacket))
  // do above elsewehere
  _addr = 0;
  _data = 0;
  _checksum = 0;
  setAll(0, 4, packet);
}

IpimBoardResponse::~IpimBoardResponse() {
  //   printf("IPBMR::dtor, this = %p\n", this);
}

//void IpimBoardResponse::setAll(int i, int j, unsigned* s) { // slice
void IpimBoardResponse::setAll(int i, int j, UnsignedList packet) { // slice
  unsigned s[packet.size()];
  list2Array(packet, s);
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
      printf("IpimBoard error: resp list broken\n");
      //      _commandResponseDamage = true;
    }
  }
}

UnsignedList IpimBoardResponse::getAll(int i, int j) { // slice
  UnsignedList lst;
  for (int count=i; count<j; count++) {
    if (count==0) {
      lst.push_back(_addr & 0xFF);
    }
    else if (count==1) {
      lst.push_back(_data & 0xFFFF);
    }
    else if (count==2) {
      lst.push_back(_data>>16);
    }
    else if (count==3) {
      lst.push_back(_checksum);
    }
  }
  return lst;
}

unsigned IpimBoardResponse::getData() { // slice
  return _data;
}

bool IpimBoardResponse::CheckCRC() {
  if (CRC(getAll(0, 3)) == _checksum){
    return true;
  }
  return false;
}


//IpimBoardData::IpimBoardData(unsigned* packet) {
IpimBoardData::IpimBoardData(UnsignedList packet) {
  //                if len(lstPacket) != 12:
  //                      raise RuntimeError, "Invalid data packet size %d" % (len(lstPacket))
  // do above implicitly or explicitly elsewehere
  _triggerCounter = 0;
  _config0 = 0;
  _config1 = 0;
  _config2 = 0;
  _ch0 = 0;
  _ch1 = 0;
  _ch2 = 0;
  _ch3 = 0;
  _checksum = 0;
  setAll(0, 12, packet);
}

IpimBoardData::IpimBoardData() { // for IpimbServer setup
  printf("actually use default constructor, don't kill\n");
  _triggerCounter = 0;//-1;
  _config0 = 0;
  _config1 = 0;
  _config2 = 0;
  _ch0 = 0;
  _ch1 = 0;
  _ch2 = 0;
  _ch3 = 0;
  _checksum = 0;//12345;
}


void IpimBoardData::dumpRaw() {
  printf("trigger counter:0x%8.8x\n", (unsigned)_triggerCounter);//%llu\n", _timestamp);
  printf("config0: 0x%2.2x\n", _config0);
  printf("config1: 0x%2.2x\n", _config1);
  printf("config2: 0x%2.2x\n", _config2);
  printf("channel0: 0x%02x\n", _ch0);
  printf("channel1: 0x%02x\n", _ch1);
  printf("channel2: 0x%02x\n", _ch2);
  printf("channel3: 0x%02x\n", _ch3);
  printf("checksum: 0x%02x\n", _checksum);
}

//void IpimBoardData::setAll(int i, int j, unsigned* s) { // slice
void IpimBoardData::setAll(int i, int j, UnsignedList packet) { // slice
  unsigned s[packet.size()];
  list2Array(packet, s);
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
      _checksum = s[count-i];
    }
    else {
      printf("IpimBoard error: data list parsing broken\n");
      //      _dataDamage = true;
    }
  }
  //  dumpRaw();
}

UnsignedList IpimBoardData::getAll(int i, int j) { // slice
  UnsignedList lst;
  for (int count=i; count<j; count++) {
    if (count==0) {
      lst.push_back(_triggerCounter>>48);
    }
    else if (count==1) {
      lst.push_back((_triggerCounter>>32) & 0xFFFF);
    }
    else if (count==2) {
      lst.push_back((_triggerCounter>>16) & 0xFFFF);
    }
    else if (count==3) {
      lst.push_back(_triggerCounter & 0xFFFF);
    }
    else if (count==4) {
      lst.push_back(_config0);
    }
    else if (count==5) {
      lst.push_back(_config1);
    }
    else if (count==6) {
      lst.push_back(_config2);
    }
    else if (count==7) {
      lst.push_back(_ch0);
    }
    else if (count==8) {
      lst.push_back(_ch1);
    }
    else if (count==9) {
      lst.push_back(_ch2);
    }
    else if (count==10) {
      lst.push_back(_ch3);
    }
    else if (count==11) {
      lst.push_back(_checksum);
    }
    else {
      printf("IpimBoard error: data list retrieval broken\n");
      //      _dataDamage = true;
    }
  }
  return lst;
}

bool IpimBoardData::CheckCRC() {
  unsigned crc = CRC(getAll(0, 11));
  if (crc == _checksum){
    return true;
  }
  return false;
}

uint64_t IpimBoardData::GetTriggerCounter() {
  return _triggerCounter;
}
unsigned IpimBoardData::GetTriggerDelay_ns() {
  return _config2*CLOCK_PERIOD;
}
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
unsigned IpimBoardData::GetConfig0() {
  return _config0;
}
unsigned IpimBoardData::GetConfig1() {
  return _config1;
}
unsigned IpimBoardData::GetConfig2() {
  return _config2;
}


// int main() {
//   cout << "make ipmb, or try\n"; 
//   IpimBoard ipmb = IpimBoard(2105, "192.168.0.91");

//   cout << "made ipmb, try to get status \n"; 
//   unsigned status = ipmb.ReadRegister(ipmb.status);
//   cout << "status: "<< status <<endl;
//   bool calibrateChannel[4] = {true, false, false, false};
//   ipmb.SetCalibrationMode(calibrateChannel);
//   unsigned calRange[4] = {1, 1, 1, 1};
//   ipmb.SetCalibrationDivider(calRange);
//   bool calibratePolarity[4] = {false, false, false, false};
//   ipmb.SetCalibrationPolarity(calibratePolarity);
//   float calV = 1.5;
//   ipmb.SetCalibrationVoltage(calV);
//   float chargeAmpRef = 2.0;
//   ipmb.SetChargeAmplifierRef(chargeAmpRef);
//   unsigned inputAmplifier[4] = {100, 100, 100, 100};
//   ipmb.SetChargeAmplifierMultiplier(inputAmplifier);
//   float biasData = 10.;
//   ipmb.SetInputBias(biasData);
//   for (int i=0; i<1000; i++) {
//     ipmb.SetCalibrationVoltage(calV);
//     ipmb.SetInputBias(biasData);
//     unsigned acqLength = 1000000;
//     unsigned acqDelay = 4095;
//     ipmb.SetChannelAcquisitionWindow(acqLength, acqDelay);
//     unsigned sampleDelay = 100000;
//     ipmb.SetTriggerDelay(sampleDelay);
//     usleep(100);
//     ipmb.SetCalibrationVoltage(calV);
//     usleep(10000);
//     unsigned strobeLength = 400000;
//     ipmb.CalibrationStart(strobeLength);
//     IpimBoardData data = ipmb.WaitData();
//     cout << "data: " << data.GetTimestamp_ticks()<< " " <<  data.GetConfig0()<< " " <<  data.GetConfig1()<< " " <<  data.GetTriggerDelay_ns()<< " " <<  data.GetCh0_V()<< " " <<  data.GetCh1_V()<< " " <<  data.GetCh2_V()<< " " <<  data.GetCh3_V() << endl;;
//   }
// }
