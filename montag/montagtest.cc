#include "pds/montag/Client.hh"
#include "pds/montag/Server.hh"
#include "pds/service/Ins.hh"

#include <unistd.h>

static void usage(const char* p)
{
  printf("Usage: %s [options]\n",p);
  printf("Options: -a <server IP dotted notation>\n");
  printf("         -g <group name>\n");
}

int main(int argc, char* argv[])
{
  unsigned short port = 12000;

  const char* addr = 0;
  const char* group = "MonTagGroup";
  int c;
  while ( (c=getopt( argc, argv, "a:g:h")) != EOF ) {
    switch(c) {
    case 'a':
      addr = optarg;
      break;
    case 'g':
      group = optarg;
      break;
    case 'h':
      usage(argv[0]);
      return 1;
    default:
      printf("Unrecognized option -%c\n",c);
      return -1;
    }
  }

  if (addr) {  // Client
    Pds::Ins svc(ntohl(inet_addr(addr)), port);
    Pds::MonTag::Client client(svc, group);

    int c;
    while( (c=getchar()) != EOF ) {
      int v = c-'0';
      if (v>=0 && v<16)
        client.request(v);
    }
  }
  else {
    Pds::MonTag::Server server(port);
    while(1) {
      usleep(1000000);
      unsigned tag = server.next();
      printf("%x\n",tag);
    }
  }
  
  return 0;
}
