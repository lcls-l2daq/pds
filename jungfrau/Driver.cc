#include "Driver.hh"
#include "slsDetectorUsers.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define EVENTS_TO_BUFFER 10
#define BUFFER_SIZE 8214
#define PACKET_NUM 128
#define DATA_SIZE 8192
#define CMD_LEN 128

// Temporary fixed sizes :(
#define NUM_MODULES 1
#define NUM_ROWS 512
#define NUM_COLUMNS 1024

#define put_command_print(cmd, val, ...) \
  { printf("cmd_put %s: %s\n", cmd, put_command(cmd, val, ##__VA_ARGS__).c_str()); }

#define put_register_print(cmd, val, ...) \
  { printf("reg_put %#x - %#x: %s\n", cmd, val, put_register(cmd, val, ##__VA_ARGS__).c_str()); }

#define put_adcreg_print(cmd, val, ...) \
  { printf("adc_put %#x - %#x: %s\n", cmd, val, put_adcreg(cmd, val, ##__VA_ARGS__).c_str()); }

#define setbit_print(cmd, val, ...) \
  { printf("setbit %#x - %d: %s\n", cmd, val, setbit(cmd, val, ##__VA_ARGS__).c_str()); }

#define clearbit_print(cmd, val, ...) \
  { printf("clearbit %#x - %d: %s\n", cmd, val, clearbit(cmd, val, ##__VA_ARGS__).c_str()); }

using namespace Pds::Jungfrau;

static const char* ADCREG   = "adcreg";
static const char* REG      = "reg";
static const char* SETBIT   = "setbit";
static const char* CLEARBIT = "clearbit";
static struct sockaddr_in clientaddr;
static socklen_t clientaddrlen = sizeof(clientaddr);

#pragma pack(push)
#pragma pack(2)
  struct jungfrau_dgram {
    char emptyheader[6];
    uint32_t reserved;
    char packetnum;
    char framenum[3];
    uint64_t bunchid;
    /* uint64_t framenum;   //  modified dec 15 */
    /* uint64_t packetnum;     */
    uint16_t data[DATA_SIZE];
  };
#pragma pack(pop)

Driver::Driver(const int id, const char* control, const char* host, unsigned port, const char* mac, const char* det_ip, bool config_det_ip) :
  _id(id), _control(control), _host(host), _port(port), _mac(mac), _det_ip(det_ip),
  _socket(-1), _connected(false), _boot(true),
  _sockbuf_sz(BUFFER_SIZE*PACKET_NUM*EVENTS_TO_BUFFER), _readbuf_sz(BUFFER_SIZE), _frame_sz(DATA_SIZE), _frame_elem(DATA_SIZE / sizeof(uint16_t)),
  _speed(JungfrauConfigType::Quarter)
{
  _readbuf = new char[_readbuf_sz];
  for (int i=0; i<MAX_JUNGFRAU_CMDS; i++) {
    _cmdbuf[i] = new char[CMD_LEN];
  }
  _socket = ::socket(AF_INET, SOCK_DGRAM, 0);
  ::setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, &_sockbuf_sz, sizeof(unsigned));

  _det = new slsDetectorUsers(_id);
  std::string reply = put_command("hostname", _control);
  std::string type  = get_command("type");

  bool needs_config = false;
  std::string rx_ip  = get_command("rx_udpip");
  std::string rx_mac = get_command("rx_udpmac");

  if (!strcmp(rx_ip.c_str(), "none") || !strcmp(rx_mac.c_str(), "none")) {
    printf("detector udp_rx interface appears to be unset\n");
    needs_config = true;
  }

  if (config_det_ip || needs_config) {
    printf("setting up detector udp_rx interface\n");
    put_command_print("rx_udpport", _port);
    put_command_print("rx_udpip",   _host);
    put_command_print("rx_udpmac", _mac);
    put_command_print("detectorip", _det_ip);
    put_command_print("detectormac", "00:aa:bb:cc:dd:ee");
    put_command_print("configuremac", 0);
  } else {
    printf("detector udp_rx interface is being set externally\n");
  }

  hostent* entries = gethostbyname(host);
  if (entries) {
    unsigned addr = htonl(*(in_addr_t*)entries->h_addr_list[0]);

    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(addr);
    sa.sin_port        = htons(port);

    int nb = ::bind(_socket, (sockaddr*)&sa, sizeof(sa));
    if (nb<0) {
      fprintf(stderr, "Error: failed to bind to Jungfrau data receiver at %s on port %d: %s\n", host, port, strerror(errno));
    } else if (strncmp(control, reply.c_str(), strlen(control)) != 0) {
      fprintf(stderr, "Error: failed to connect to Jungfrau control interface at %s: %s\n", control, reply.c_str());
    } else if (strcmp(type.c_str(), "Jungfrau+") !=0) {
      fprintf(stderr, "Error: detector at %s on port %d is not a Jungfrau: %s\n", host, port, type.c_str());
    } else {
      _connected=true;
    }
  }
}

Driver::~Driver()
{
  if (_socket >= 0) {
    ::close(_socket);
  }
  if (_readbuf) {
    delete[] _readbuf;
  }
  if (_det) {
    put_command("free", "0");
    delete _det;
  }
  for (int i=0; i<MAX_JUNGFRAU_CMDS; i++) {
    if (_cmdbuf[i]) delete[] _cmdbuf[i];
  }
}

bool Driver::configure(uint64_t nframes, JungfrauConfigType::GainMode gain, JungfrauConfigType::SpeedMode speed, double trig_delay, double exposure_time, uint32_t bias)
{
  if (!_connected) {
    fprintf(stderr, "Error: failed to configure to Jungfrau at %s\n", _control);
    return false;
  }

  // stop the detector if not!
  if (status() == RUNNING) {
    if (!stop()) {
      fprintf(stderr, "Error: can't configure when the detector is in a running state\n");
      return false;
    }
  }

  if (_boot) {
    // Setting Dacs 12bit on 2.5V  (i.e. 2.5v=4096)
    printf("Setting Dacs:\n");
    // TODO - should these be in the config object?
    int vb_ds = 1000;
    int vb_comp = 1220;
    int vb_pixbuf = 750;
    int vref_ds = 480;
    int vref_comp = 420;
    int vref_prech = 1450;
    int vin_com = 1053;
    int vdd_prot = 3000;
  
    // setting vb_ds
    printf("setting vb_ds to %d\n", vb_ds);
    put_command_print("dac:5", vb_ds);

    // setting vb_comp
    printf("setting vb_comp to %d\n", vb_comp);
    put_command_print("dac:0", vb_comp);

    // setting vb_pixbuf
    printf("setting vb_pixbuf to %d\n", vb_pixbuf);
    put_command_print("dac:4", vb_pixbuf);

    // setting vref_ds
    printf("setting vref_ds to %d\n", vref_ds);
    put_command_print("dac:6", vref_ds);

    // setting vref_comp
    printf("setting vref_comp to %d\n", vref_comp);
    put_command_print("dac:7", vref_comp);

    // setting vref_prech
    printf("setting vref_prech to %d\n", vref_prech);  
    put_command_print("dac:3", vref_prech);

    // setting vin_com
    printf("setting vin_com to %d\n", vin_com);
    put_command_print("dac:2", vin_com);

    // setting vdd_prot
    printf("setting vdd_prot to %d\n", vdd_prot);
    put_command_print("dac:1", vdd_prot);

    // power on the chips
    printf("powering on the chip\n");
    put_register_print(0x5e, 0x1);

    // reset adc
    printf("resetting the adc\n");
    put_adcreg_print(0x08, 0x3);
    put_adcreg_print(0x08, 0x0);
    put_adcreg_print(0x14, 0x40);
    put_adcreg_print(0x4, 0xf);
    put_adcreg_print(0x5, 0x3f);
    put_adcreg_print(0x18, 0x2);
    put_register_print(0x43, 0x453b2a9c);
  }

  if (_boot || speed != _speed) {
    // set speed
    switch (speed) {
    case JungfrauConfigType::Quarter:
      printf("setting detector to quarter speed\n");
      put_register_print(0x59, 0x8981);
      put_register_print(0x42, 0x10);
      put_register_print(0x5d, 0xf0000f00);
      put_command_print("adcphase", 25);
      break;
    case JungfrauConfigType::Half:
      printf("setting detector to half speed\n");
      put_register_print(0x59, 0x7f7c);
      put_register_print(0x42, 0x20);
      put_register_print(0x5d, 0x00000f00);
      put_command_print("adcphase", 65);
      break;
    default:
      fprintf(stderr,"Error: invalid clock speed setting for the camera %d\n", speed);
      return false;
    } 
    _speed = speed;
    sleep(1);
  }

  reset();

  printf("setting trigger delay to %.6f\n", trig_delay);
  put_command("delay", trig_delay);

  if (!nframes) {
    printf("configuring triggered mode\n");
    put_register_print(0x4e, 0x3333);
    put_command_print("cycles", 10000000000);
    put_command_print("frames", 1);
    put_command_print("period", 0.005);
  } else {
    printf("configuring for free run\n");
    put_register_print(0x4e, 0x0000);
    put_command_print("cycles", 1);
    put_command_print("frames", nframes);
    put_command_print("period", 0.2);
  }

  printf("setting exposure time to %.6f seconds\n", exposure_time);
  put_command_print("exptime", 0.000010);

  printf("setting bias voltage to %d volts\n", bias);
  put_command_print("vhighvoltage", bias);

  printf("setting gain mode %d\n", gain);
  // Clear all the bits
  clearbit_print(0x5d, 0);
  clearbit_print(0x5d, 1);
  clearbit_print(0x5d, 2);
  clearbit_print(0x5d, 12);
  clearbit_print(0x5d, 13);
  // Now set the ones of the gain we want
  switch(gain) {
    case JungfrauConfigType::Normal:
      break;
    case JungfrauConfigType::FixedGain1:
      setbit_print(0x5d, 1);
      break;
    case JungfrauConfigType::FixedGain2:
      setbit_print(0x5d, 1);
      setbit_print(0x5d, 2);
      break;
    case JungfrauConfigType::ForcedGain1:
      setbit_print(0x5d, 12);
      break;
    case JungfrauConfigType::ForcedGain2:
      setbit_print(0x5d, 12);
      setbit_print(0x5d, 13);
      break;
    case JungfrauConfigType::HighGain0:
      setbit_print(0x5d, 0);
      break;
  }

  _boot = false;

  return status() != ERROR;
}

bool Driver::check_size(uint32_t num_modules, uint32_t num_rows, uint32_t num_columns)
{
  return (num_modules == NUM_MODULES) && (num_rows == NUM_ROWS) && (num_columns == NUM_COLUMNS);
}

std::string Driver::put_command(const char* cmd, const char* value, int pos)
{
  if (strlen(cmd) >= CMD_LEN || strlen(value) >= CMD_LEN) {
    return "invalid command or value length";
  } 
  strcpy(_cmdbuf[0], cmd);
  strcpy(_cmdbuf[1], value);
  if (value) {
    return _det->putCommand(2, _cmdbuf, pos);
  } else {
    return _det->putCommand(1, _cmdbuf, pos);
  }
}

std::string Driver::put_command(const char* cmd, const int value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%d", value);
  return _det->putCommand(2, _cmdbuf, pos); 
}

std::string Driver::put_command(const char* cmd, const unsigned int value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%u", value);
  return _det->putCommand(2, _cmdbuf, pos);
}

std::string Driver::put_command(const char* cmd, const long value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%ld", value);
  return _det->putCommand(2, _cmdbuf, pos);
}

std::string Driver::put_command(const char* cmd, const unsigned long value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%ld", value);
  return _det->putCommand(2, _cmdbuf, pos);
}

std::string Driver::put_command(const char* cmd, const long long value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%lld", value);
  return _det->putCommand(2, _cmdbuf, pos);
}

std::string Driver::put_command(const char* cmd, const unsigned long long value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%llu", value);
  return _det->putCommand(2, _cmdbuf, pos);
}

std::string Driver::put_command(const char* cmd, const double value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%f", value);
  return _det->putCommand(2, _cmdbuf, pos);
}

std::string Driver::get_command(const char* cmd, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  return _det->getCommand(1, _cmdbuf, pos);
}

std::string Driver::put_register(const int reg, const int value, int pos)
{
  strcpy(_cmdbuf[0], REG);
  sprintf(_cmdbuf[1], "%#x", reg);
  sprintf(_cmdbuf[2], "%#x", value);
  return _det->putCommand(3, _cmdbuf, pos);
}

std::string Driver::put_adcreg(const int reg, const int value, int pos)
{
  strcpy(_cmdbuf[0], ADCREG);
  sprintf(_cmdbuf[1], "%#x", reg);
  sprintf(_cmdbuf[2], "%#x", value);
  return _det->putCommand(3, _cmdbuf, pos);
}

std::string Driver::setbit(const int reg, const int bit, int pos)
{
  strcpy(_cmdbuf[0], SETBIT);
  sprintf(_cmdbuf[1], "%#x", reg);
  sprintf(_cmdbuf[2],  "%d", bit);
  return _det->putCommand(3, _cmdbuf, pos);
}

std::string Driver::clearbit(const int reg, const int bit, int pos)
{
  strcpy(_cmdbuf[0], CLEARBIT);
  sprintf(_cmdbuf[1], "%#x", reg);
  sprintf(_cmdbuf[2],  "%d", bit);  
  return _det->putCommand(3, _cmdbuf, pos);
}

uint64_t Driver::nframes()
{
  std::string reply = get_command("nframes");
  return strtoull(reply.c_str(), NULL, 10);
}

Driver::Status Driver::status()
{
  std::string reply = get_command("status");
  if (!strcmp(reply.c_str(), "waiting"))
    return WAIT;
  else if (!strcmp(reply.c_str(), "idle"))
    return IDLE;
  else if (!strcmp(reply.c_str(), "running"))
    return RUNNING;
  else if (!strcmp(reply.c_str(), "data"))
    return DATA;
  else
    return ERROR;
}

std::string Driver::status_str()
{
  return get_command("status");
}

bool Driver::start()
{
  printf("starting detector: %s\n", put_command("status", "start").c_str());
  return status() == RUNNING || status() == WAIT;
}

bool Driver::stop()
{
  std::string reply = put_command("status", "stop");
  if (!strcmp(reply.c_str(), "error")) {
    printf("stopping detector:  done\n");
    reset();
  } else {
    printf("stopping detector: %s\n", reply.c_str());
  }
  return status() != RUNNING;
}

void Driver::reset()
{
  printf("reseting run control ...");
  // reset mem machine fifos fifos
  put_register(0x4f, 0x4000);
  put_register(0x4f, 0x0);
  // reset run control
  put_register(0x4f, 0x0400);
  put_register(0x4f, 0x0);
  printf(" done\n");
}

int32_t Driver::get_frame(uint16_t* data)
{
  int nb;
  int packetnum_fixed = -1;
  int32_t frame = -1;
  int32_t pframe = 0;
  bool drop = false;

  for (int i=0; i<PACKET_NUM; i++) {
    nb = ::recvfrom(_socket, _readbuf, BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, &clientaddrlen);
    if (nb<0) {
      fprintf(stderr,"Error: failure receiving packet from Jungfru at %s on port %d: %s\n", _host, _port, strerror(errno));
      return -1;
    }
    jungfrau_dgram* packet = (jungfrau_dgram*) _readbuf;
    packetnum_fixed = (PACKET_NUM-1) - packet->packetnum;

    if (packetnum_fixed != i) {
      fprintf(stderr,"Error: data out-of-order received packet %d, but was expecting packet %d\n", packetnum_fixed, i);
      i = packetnum_fixed;
      drop = true;
    }

    memcpy(&pframe, packet->framenum, sizeof(char)*3);
    if (frame < 1) {
      frame = pframe;
    } else if(frame != pframe) {
      fprintf(stderr,"Error: data out-of-order got data for frame %d, but was expecting frame %d\n", pframe, frame);
      return -1;
    }
    
    memcpy(&data[_frame_elem*i], packet->data, _frame_sz);
  }

  // Drop frames where packets were missing - TODO keep these, but assert damage?
  return drop?-1:frame;
}

#undef EVENTS_TO_BUFFER
#undef BUFFER_SIZE
#undef PACKET_NUM
#undef DATA_SIZE
#undef CMD_LEN
#undef NUM_MODULES
#undef NUM_ROWS
#undef NUM_COLUMNS
#undef put_command_print
#undef put_register_print
#undef put_adcreg_print
#undef setbit_print
#undef clearbit_print
