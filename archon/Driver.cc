#include "Driver.hh"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_CONFIG_LINE 2048
#define BUFFER_SIZE 4096

using namespace Pds::Archon;

Driver::Driver(const char* host, unsigned port) :
  _host(host), _port(port), _socket(-1), _connected(false), _msgref(0), _readbuf_sz(BUFFER_SIZE), _writebuf_sz(BUFFER_SIZE)
{
  _readbuf = new char[_readbuf_sz];
  _writebuf = new char[_writebuf_sz];
  _socket = ::socket(AF_INET, SOCK_STREAM, 0);

  hostent* entries = gethostbyname(host);
  if (entries) {
    unsigned addr = htonl(*(in_addr_t*)entries->h_addr_list[0]);

    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(addr);
    sa.sin_port        = htons(port);

    int nb = ::connect(_socket, (sockaddr*)&sa, sizeof(sa));
    if (nb<0) {
      fprintf(stderr, "Error: failed to connect to Archon at %s on port %d: %s\n", host, port, strerror(errno));
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
  if (_writebuf) {
    delete[] _writebuf;
  }
}

bool Driver::configure(const char* filepath)
{
  if (!_connected) {
    hostent* entries = gethostbyname(_host);
    if (entries) {
     unsigned addr = htonl(*(in_addr_t*)entries->h_addr_list[0]);

      sockaddr_in sa;
      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = htonl(addr);
      sa.sin_port        = htons(_port);

      int nb = ::connect(_socket, (sockaddr*)&sa, sizeof(sa));
      if (nb<0) {
        fprintf(stderr, "Error: failed to connect to Archon at %s on port %d: %s\n", _host, _port, strerror(errno));
        return false;
      } else {
        _connected=true;
      }
    } else {
      fprintf(stderr, "Error: failed to connect to Archon at %s - unknown host\n", _host);
      return false;
    }
  }
  
  printf("Attempting to load Archon configuration file: %s\n", filepath);

  FILE* f = fopen(filepath, "r");
  if (f) {
    int linenum = 0;
    bool is_conf = false;
    size_t line_sz = 2048;
    size_t hdr_sz = 3;
    char* hdr  = (char *)malloc(hdr_sz);
    char* reply = (char *)malloc(line_sz+hdr_sz);
    char* line = (char *)malloc(line_sz);

    command("POLLOFF");
    command("CLEARCONFIG");

    while (getline(&line, &line_sz, f) != -1) {
      if (!strncmp(line, "[SYSTEM]", 8)) {
        is_conf = false;
        continue;
      } else if (!strncmp(line, "[CONFIG]", 8)) {
        is_conf = true;
        continue;
      }
      if (is_conf && strcmp(line, "\n")) {
        for(char* p = line; (p = strchr(p, '\\')); ++p) {
          *p = '/';
        }
        char *pr = line, *pw = line;
        while (*pr) {
          *pw = *pr++;
          pw += (*pw != '"');
        }
        *pw = '\0';
        if (!wr_config_line(linenum, line)) {
          fprintf(stderr, "Error writing configuration line %d: %s\n", linenum, line);
          command("POLLON");
          return false;
        }
        linenum++;
      }
    }
    if (line) {
      free(hdr);
      free(reply);
      free(line);
    }
    fclose(f);

    command("POLLON");
    return command("APPLYALL");
  } else {
    fprintf(stderr, "Error opening %s\n", filepath);
    return false;
  }
}

bool Driver::command(const char* cmd)
{
  int len;
  char buf[16];
  char cmdstr[strlen(cmd) + 3];
  sprintf(cmdstr, ">%02X%s\n", _msgref, cmd);
  sprintf(buf, "<%02X", _msgref);
  ::write(_socket, cmdstr, strlen(cmdstr));
  len = _recv(_readbuf, _readbuf_sz);
  if (len<0) {
    fprintf(stderr, "Error: no response from controller: %s\n", strerror(errno));
    return false;
  }
  if (!strncmp(_readbuf, buf, 3)) {
    _msgref += 1;
    return true;
  } else {
    sprintf(buf, ">%02XFETCHLOG\n", _msgref);
    ::write(_socket, buf, strlen(buf));
    len = _recv(_readbuf, _readbuf_sz);
    if (len < 3) {
      fprintf(stderr, "Error on command (%s): no message from camera\n", cmdstr);
    } else {
      fprintf(stderr, "Error on command (%s): %s\n", cmdstr, _readbuf);
      _msgref += 1;
    }
    return false;
  }
}

bool Driver::wr_config_line(unsigned num, const char* line)
{
  if (strlen(line) > MAX_CONFIG_LINE) {
    fprintf(stderr, "Configuration line %d is longer than controller max of %d\n", num, MAX_CONFIG_LINE);
    return false;
  }

  sprintf(_writebuf, "WCONFIG%04d%s", num, line);
  return command(_writebuf);
}

int Driver::_recv(char* buf, unsigned bufsz)
{
  int len;
  len = ::read(_socket, buf, bufsz);
  if (len<0) {
    buf[0] = '\0'; // make sure buffer is 'empty'
    return len;
  } else if (len>0 && buf[len-1] != '\n') {
    len += _recv(&buf[len], bufsz-len);
  }
  // make sure buffer is properly terminated
  if (len < (int) bufsz) {
    buf[len] = '\0';
  }
  return len;
}

const unsigned long long Driver::time() {
  if (command("TIMER")) {
    return strtoull(_readbuf, NULL, 16);
  } else {
    return 0;
  }
}

#undef MAX_CONFIG_LINE
#undef BUFFER_SIZE
