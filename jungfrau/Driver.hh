#ifndef Pds_Jungfrau_Driver_hh
#define Pds_Jungfrau_Driver_hh

#include "pds/config/JungfrauConfigType.hh"

#include <stdint.h>
#include <string>

#define MAX_JUNGFRAU_CMDS 3

class slsDetectorUsers;

namespace Pds {
  namespace Jungfrau {
    class DacsConfig {
      public:
        DacsConfig(uint16_t vb_ds, uint16_t vb_comp, uint16_t vb_pixbuf, uint16_t vref_ds,
                   uint16_t vref_comp, uint16_t vref_prech, uint16_t vin_com, uint16_t  vdd_prot);
        ~DacsConfig();
      public:
        uint16_t vb_ds() const;
        uint16_t vb_comp() const;
        uint16_t vb_pixbuf() const;
        uint16_t vref_ds() const;
        uint16_t vref_comp() const;
        uint16_t vref_prech() const;
        uint16_t vin_com() const;
        uint16_t vdd_prot() const;
      private:
        uint16_t  _vb_ds;
        uint16_t  _vb_comp;
        uint16_t  _vb_pixbuf;
        uint16_t  _vref_ds;
        uint16_t  _vref_comp;
        uint16_t  _vref_prech;
        uint16_t  _vin_com;
        uint16_t  _vdd_prot;
    };

    class Driver {
      public:
        enum Status { IDLE, RUNNING, WAIT, DATA, ERROR };
        Driver(const int id, const char* control, const char* host, const unsigned port, const char* mac, const char* det_ip, bool config_det_ip=false);
        ~Driver();
        bool configure(uint64_t nframes, JungfrauConfigType::GainMode gain, JungfrauConfigType::SpeedMode speed, double trig_delay, double exposure_time, double exposure_period, uint32_t bias, const DacsConfig& dac_config);
        bool check_size(uint32_t num_modules, uint32_t num_rows, uint32_t num_columns);
        std::string put_command(const char* cmd, const char* value, int pos=-1);
        std::string put_command(const char* cmd, const short value, int pos=-1);
        std::string put_command(const char* cmd, const unsigned short value, int pos=-1);
        std::string put_command(const char* cmd, const int value, int pos=-1);
        std::string put_command(const char* cmd, const unsigned int value, int pos=-1);
        std::string put_command(const char* cmd, const long value, int pos=-1);
        std::string put_command(const char* cmd, const unsigned long value, int pos=-1);
        std::string put_command(const char* cmd, const long long value, int pos=-1);
        std::string put_command(const char* cmd, const unsigned long long value, int pos=-1);
        std::string put_command(const char* cmd, const double value, int pos=-1);
        std::string put_register(const int reg, const int value, int pos=-1);
        std::string setbit(const int reg, const int bit, int pos=-1);
        std::string clearbit(const int reg, const int bit, int pos=-1);
        std::string put_adcreg(const int reg, const int value, int pos=-1);
        std::string get_command(const char* cmd, int pos=-1);
        uint64_t nframes();
        Status status();
        std::string status_str();
        bool start();
        bool stop();
        void reset();
        int32_t get_frame(uint16_t* data);
        const char* error();
      private:
        //int           _recv(char* buf, unsigned bufsz);
        const int         _id;
        const char*       _control;
        const char*       _host;
        const unsigned    _port;
        const char*       _mac;
        const char*       _det_ip;
        int               _socket;
        bool              _connected;
        bool              _boot;
        unsigned          _sockbuf_sz;
        unsigned          _readbuf_sz;
        unsigned          _frame_sz;
        unsigned          _frame_elem;
        char*             _readbuf;
        char*             _msgbuf;
        char*             _cmdbuf[MAX_JUNGFRAU_CMDS];
        slsDetectorUsers* _det;
        JungfrauConfigType::SpeedMode _speed;
    };
  }
}

#endif
