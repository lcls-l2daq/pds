#ifndef Pds_Archon_Driver_hh
#define Pds_Archon_Driver_hh


namespace Pds {
  namespace Archon {
    class Driver {
      public:
        Driver(const char* host, unsigned port);
        ~Driver();
        bool configure(const char* filepath);
        bool command(const char* cmd);
        bool wr_config_line(unsigned num, const char* line);
        const unsigned long long time();
      private:
        int           _recv(char* buf, unsigned bufsz);
        const char*   _host;
        unsigned      _port;
        int           _socket;
        bool          _connected;
        unsigned char _msgref;
        unsigned      _readbuf_sz;
        unsigned      _writebuf_sz;
        char*         _readbuf;
        char*         _writebuf;
    };
  }
}

#endif
