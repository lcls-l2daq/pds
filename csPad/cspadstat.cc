 /*
  * Main function for cxi interaction
 *
 *  Author:  <Jack Pines: jackp@slac.stanford.edu>
 *  Created: <2009-08-25>
 *  Time-stamp: <Tue Apr 6 16:00:50 PDT 2010 jackp>
 *
 *
 *  based on work by Jim Panneta
 *
 */

//#include "rce/shell/image_tools.hh"    // prototype for open_filespec

#define __need_getopt_newlib
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
using std::string;

#include <list>
using std::list;

#include <rtems.h>

#include "pds/csPad/CxiReadOnlyRegisters.hh"
#include "pds/csPad/CxiConfigurator.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/config/CsPadConfigType.hh"

//To enable the EVR:  This will go away eventually so do by this command option
//
//write 0x00008200  to register 0xFF
//0x146 = OpCode select
//   Write chosen opcode for acquisition to this address
//# 40=120hz
//# 41=60hz
//# 42=30hz
//# 43=10hz
//# 44=5hz
//# 45=1hz

//0x1A = Run Mode
//   0x00 = No Running
//   0x01 = Run front end but drop data
//   0x02 = Run front end, send data to RCE
//   0x03 = Run front end, data mode depends on TTL input

//TrigDelay = 0x101, Set to 2
//TrigWidth = 0x102, Set to 5

enum {compBias1=0xff, compBias2=0xc8, iss2=0x3c, iss5=0x25, vref=0xba};
enum {rampcurrR1=0x4, rampCurrR2=0x25, rampCurrRef=0, rampVoltRef=0x61, analogPrst=0xfc};
enum {vinj=0xba};

uint8_t myPots[80] = {
    vref, vref, rampcurrR1, 0,
    compBias1, compBias1, compBias1, compBias1, compBias1, compBias1, compBias1, compBias1,
    compBias1, compBias1, compBias1, compBias1, compBias1, compBias1, compBias1, compBias1,
    vref, vref, rampCurrR2, vinj,
    compBias2, compBias2, compBias2, compBias2, compBias2, compBias2, compBias2, compBias2,
    compBias2, compBias2, compBias2, compBias2, compBias2, compBias2, compBias2, compBias2,
    vref, vref, rampCurrRef, 0,
    iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2, iss2,
    vref, vref, rampVoltRef, analogPrst,
    iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5, iss5
//    0,  1,  2,  3,  4,  5,  6,  7,
//    8,  9, 10, 11, 12, 13, 14, 15,
//   16, 17, 18, 19, 20, 21, 22, 23,
//   24, 25, 26, 27, 28, 29, 30, 31,
//   32, 33, 34, 35, 36, 37, 38, 39,
//   40, 41, 42, 43, 44, 45, 46, 47,
//   48, 49, 50, 51, 52, 53, 54, 55,
//   56, 57, 58, 59, 60, 61, 62, 63,
//   64, 65, 66, 67, 68, 69, 70, 71,
//   72, 73, 74, 75, 76, 77, 78, 79
 };

//shiftSelect    0x00000004
//edgeSelect     0x00000000
//readClkSet     0x00000002
//readClkHold    0x00000001
//dataMode       0x0
//prstSel        0x00000000
//acqDelay       0x00000118
//intTime        0x000005dc
//digDelay       0x000003c0
//ampIdle        0x00000000
//injTotal       0x00
//rowColShiftPer 0x00000005
//gainMap        0xffff

uint32_t myQuad[18] = {4,4,4,4, 0,0,0,0, 2, 1, Pds::CsPad::normal, 0, 0x118, 0x5dc, 0x3c0, 0, 0, 5};

class pGain {
  public:
    pGain() {
      for (int c=0; c<Pds::CsPad::ColumnsPerASIC; c++) {
        for (int r=0; r<Pds::CsPad::MaxRowsPerASIC; r++) {
          map[c][r] = 0xffff;
        }
      }
    }
  public:
    uint16_t map[Pds::CsPad::ColumnsPerASIC][Pds::CsPad::MaxRowsPerASIC];
};

class pPots {
  public:
    pPots() {
      for(int i=0; i<80; i++) pots[i] = myPots[i];
    };
  public:
    uint8_t pots[80];
};

class pReadOnly {
  public:
    pReadOnly() {
      ro[0] = ro[1] = 0;
    }
  public:
    uint32_t ro[2];
};

class pQuad {
  public:
    pQuad() {
      for(int i=0; i<18; i++) wr[i] = myQuad[i];
    }
    void print(int i) {
      printf("  Quad[%d]:\n\t_shiftSelect[0] %u\n\t_shiftSelect[1] %u\n\t_shiftSelect[2] %u\n\t_shiftSelect[3] %u\n\t", i,
          (unsigned)wr[0], (unsigned)wr[1], (unsigned)wr[2], (unsigned)wr[3]);
      printf("_edgeSelect[0] %u\n\t_edgeSelect[1] %u\n\t_edgeSelect[2] %u\n\t_edgeSelect[3] %u\n\t", (unsigned)wr[4], (unsigned)wr[5], (unsigned)wr[6], (unsigned)wr[7]);
      printf("_readClkSet %u\n\t_readClkHold %u\n\t_dataMode %u\n\t", (unsigned)wr[8], (unsigned)wr[9], (unsigned)wr[10]);
      printf("_prstSel %u\n\t_acqDelay %u\n\t_intTime %u\n\t", (unsigned)wr[11], (unsigned)wr[12], (unsigned)wr[13]);
      printf("_digDelay %u\n\t_ampIdle %u\n\t_injTotal %u\n\t", (unsigned)wr[14], (unsigned)wr[15], (unsigned)wr[16]);
      printf("_rowColShiftPer 0x%x\n", (unsigned)wr[17]);
    }
  public:
    uint32_t wr[18];
    pReadOnly ro;
    pPots     pots;
    pGain     map;
};

class pCfg {
  public:
    pCfg(uint32_t t, uint32_t m) : ver(0), rd(2), ec(40), irm(Pds::CsPad::RunButDrop),
				   arm(Pds::CsPad::RunAndSendTriggeredByTTL), tdi(t), ppq(1148516), bam0(0), bam1(0), am(0xf), qm(m)
  { rm = ((m&1)*0xff) | (((m>>1)&1)*0xff00) | (((m>>2)&1)*0xff0000) | (((m>>3)&1)*0xff000000); }
  public:
    void print() {
      printf("Config:\n\t_concentratorVersion 0x%x\n\t_runDelay %u\n\t_eventCode %u\n\t_inactiveRunMode %u\n\t",
          (unsigned)ver, (unsigned)rd, (unsigned)ec, (unsigned)irm);
      printf("_activeRunMode %u\n\t_testDataIndex %u\n\t_payloadPerQuad %u\n\t_badAsicMask 0x%x,0x%x\n\t_AsicMask 0x%x\n\t_quadMask 0x%x\n\t_roiMask 0x%x\n",
	  (unsigned)arm, (unsigned)tdi, (unsigned)ppq, (unsigned)bam0, (unsigned)bam1, (unsigned)am, (unsigned)qm, (unsigned)rm);
      for (int i=0; i<4; i++) {
        quads[i].print(i);
      }
    }
    void testDataIndex(uint32_t i) {tdi=i;}
    void quadMask(uint32_t m) {qm=m;}

  public:
    uint32_t ver;
    uint32_t rd;
    uint32_t ec;
    uint32_t irm;
    uint32_t arm;
    uint32_t tdi;
    uint32_t ppq;
    uint32_t bam0;
    uint32_t bam1;
    uint32_t am;
    uint32_t qm;
    uint32_t rm;
    pQuad  quads[4];
};

int parseParams(char *in, unsigned* p) {
  int ret = sscanf(in, "%u,%x,%u,%u", p, p+1, p+2, p+3);
  return ret;
}

static string cxistatUsage =  "Usage:\n" \
    "  cxistat  [-h] \n" \
    "  Show the cxi status.\n" \
    "   with no arguments, print the CsPad front end debug registers\n" \
    "  Arguments: \n" \
    "  -P print the current state of the concentrator\n" \
    "  -R print the current state of the unmasked quads\n" \
    "  -T print only the temperatures of the unmasked quads\n" \
    "  -C reset the concentrator(N.B. might require restarting quads)\n" \
    "  -c reset the concentrator counters\n" \
    "  -L <mask> reset the concentrator's 6 links set in mask\n" \
    "  -s clear the concentrator's Sequence counter\n" \
    "  -Q <mask> reset the quads set in mask, note that mask\n" \
    "            value 0x10, will reset all using concentrator.\n" \
    "  -q <mask> reset the quads' counters for quad set in mask\n" \
    "  -f <flag> if flag, enable pgp forwarding else stop it.\n" \
    "  -m <numb> quad mask, 0 to 15, bit high means quad is present\n" \
    "  -G put hard wired configuration into memory for use later\n" \
    "  -g <numb> configure CsPad with test image numb, 0 to 7.\n" \
    "  -e eanble the EVR\n" \
    "  -E <code> set the EVR event code as follows\n" \
    "           40=120hz\n" \
    "           41=60hz\n" \
    "           42=30hz\n" \
    "           43=10hz\n" \
    "           44=5hz\n" \
    "           45=1hz\n" \
    "  -r <mode> set the CXI run mode as follows\n" \
    "           0x00 = No Running\n" \
    "           0x01 = Run front end but drop data\n" \
    "           0x02 = Run front end, send data to RCE\n" \
    "           0x03 = Run front end, data mode depends on TTL input\n" \
    "  -p print the current configuration\n" \
    "  -S byte swap the configuration\n" \
    "  -i <d,a[,n]> inspect dest,addr or dest,addr,numb\n" \
    "           comma separated, no spaces\n" \
    "           dest and numb are decimal, addr is hex\n" \
    "           dest 0->3 are quads, 4 is concentrator\n" \
    "  -h print this message\n" \
    "\n";

RcePgp::Driver* pgpd = 0;
RceCxi::CxiProxyThread* pt = 0;
RceCxi::CxiPgpHandler* myHandler=0;
RcePic::Pool* pool=0;
rtems_id      iq=0;
RceCxi::CxiReadOnlyRegisters* ro=0;
RcePgp::Handler* savedHandler=0;
unsigned quadMask = RceCxi::CxiReadOnlyRegisters::AllQuadsMask;
RceCxi::CxiConfigurator* cfgrtr = 0;
pCfg*  pConfig = 0;
RcePgpFwd::PgpForwardHandler* pgpFwdHdlr = 0;
bool doEvrEnable;
bool doEventCodeWrite;
bool doRunModeWrite;
bool configLock = true;
unsigned value1, value2;
unsigned myp[4];
uint32_t _value;
uint32_t ret[1000];

int cxistatParseOptions(int argc, char **argv) {
  int theOpt;
  /* Use reentrant form of getopt as per RTEMS Shell Users Guide (4.9.99.0) */
  struct getopt_data getopt_reent;
  memset(&getopt_reent, 0, sizeof(getopt_data));
  char paramString[80] = {0};
  while ( (theOpt = getopt_r(argc, argv, "CQ:i:csq:L:f:m:g:GeE:r:phSPRTxz", &getopt_reent)) != EOF ) {
    unsigned mask, configCount;
    unsigned configResult = 0;
    bool replacedHandler = false;
    switch(theOpt) {
      case 'z':
        _value = 0xdeadbabe;
        asm volatile( "mfspr %0,980" : "=r" (_value) );
        printf("Exception-Syndrome Register value 0x%x\n", (unsigned)_value);
        break;
      case 'x':
        for (unsigned l=0; l<80; l++) printf("0x%x - 0x%x\n", l, myPots[l]);
        break;
      case 'i':
        strncpy(paramString, getopt_reent.optarg, 80);
        switch(parseParams(paramString, myp)) {
          case 2:
            cfgrtr->readRegister((RcePgp::RegisterSlaveExportFrame::FEdest)*myp, myp[1], 0xbeeb, ret);
            printf("%u - 0x%06x - 0x%x\n", *myp, myp[1], (unsigned)ret[0]);
            break;
          case 3:
            for (unsigned m=0; m<myp[2]; m++) {
              cfgrtr->readRegister((RcePgp::RegisterSlaveExportFrame::FEdest)*myp, myp[1]+m, 0xbead, ret+m);
              printf("%u - 0x%06x - 0x%x\n", *myp, myp[1]+m, (unsigned)ret[m]);
              rtems_task_wake_after(1);
            }
            break;
          default:
            printf("Incorrect number of paramters, should be 'd,a' or 'd,a,n'");
            break;
        }
        break;
      case 'P':
        ro->read(RceCxi::CxiReadOnlyRegisters::ConcentratorMask);
        ro->print(RceCxi::CxiReadOnlyRegisters::ConcentratorMask);
        break;
      case 'R':
        ro->read(quadMask);
        ro->print(quadMask);
       break;
      case 'T':
      ro->read(quadMask);
      ro->print(quadMask + RceCxi::CxiReadOnlyRegisters::QuadTemperaturesOnly);
      break;
      case 'h':
        printf(cxistatUsage.c_str());
        break;
      case 'p':
        printf("Size of whole config %d\n", sizeof(Pds::CsPad::ConfigV2));
//        printf("Size of a quad config %d\n", sizeof(Pds::CsPad::ConfigV1QuadReg));
//        printf("Size of a gain map %d\n", sizeof(Pds::CsPad::CsPadGainMapCfg));
        printf("Config is at %p\n", pConfig);
        pConfig->print();
        configLock = false;
       break;
      case 'C':
        ro->resetConcentrator();
        break;
      case 'c':
        ro->resetConcentratorCounters();
        break;
      case 's':
        ro->clearConcentratorSeqCount();
        break;
      case 'e':
        doEvrEnable = true;
        break;
      case 'E':
        value1 = strtoul(getopt_reent.optarg, 0, 0);
        doEventCodeWrite = true;
        break;
      case 'r':
        value2 = strtoul(getopt_reent.optarg, 0, 0);
        doRunModeWrite = true;
        break;
     case 'L':
         mask = strtoul(getopt_reent.optarg, 0, 0);
         mask &= quadMask;
         ro->resetConcentratorLinks(mask);
         break;
     case 'g':
       if (!configLock) {
         mask = strtoul(getopt_reent.optarg, 0, 0);
         configCount = 1 + ((mask>>8)&0xff);
         pConfig->testDataIndex((uint32_t)mask&7);
         pConfig->quadMask((uint32_t)quadMask);
         if ((savedHandler = pgpd->handler())!= myHandler) {
           replacedHandler = true;
           pgpd->handler((RcePgp::Handler*)myHandler);
         }
         for (unsigned c=0; c<configCount && !configResult; c++) {
           configResult |= cfgrtr->configure((mask>>3)&0x201f);
         }
         if (!configResult) printf("Succeeded!\n");
         else printf("Configuration FAILED 0x%x\n", configResult);
         if (replacedHandler) pgpd->handler(savedHandler);
       } else printf("Ensure there is a configuration first with -p or -G if necessary\n");
       break;
     case 'G' :
       printf("Warning, we are writing over any preexisting configuration!\n");
       configLock = false;
       new (pConfig) pCfg(4, quadMask);
       break;
     case 'S' : {
//       char foo[] = {"0123456789abcdef"};
//       printf("%s\n", foo);
//       uint16_t* hwp = (uint16_t*) foo;
//       for (int s=0; s<8; s++) {
//         *(hwp+s) = _swap_u16(hwp+s);
//       }
//       printf("%s\n", foo);
       if (cfgrtr) {
         uint64_t start = RceSvc::lticks()/35000LL;
         cfgrtr->swap((CsPadConfigType*)pConfig);
         uint64_t diff = (RceSvc::lticks()/35000LL) - start;
         printf("Swapping took %lld.%lld milliseconds\n", diff/10LL, diff%10LL);
       } else printf("Configure before trying this\n");
     }
     break;
     case 'Q':
       mask = strtoul(getopt_reent.optarg, 0, 0);
       ro->resetQuads(mask);
       break;
     case 'q':
       mask = strtoul(getopt_reent.optarg, 0, 0);
       mask &= quadMask;
       ro->resetQuadCounters(mask);
       break;
     case 'f':
       mask = strtoul(getopt_reent.optarg, 0, 0);
       if (!RcePgpFwd::PgpForward::instance()) {
         printf("You must start the pgpforwarding thread first!\n");
       } else {
         if (mask) {
           if (!pgpFwdHdlr) pgpFwdHdlr = RcePgpFwd::PgpForward::instance()->handler();
           pgpd->handler((RcePgp::Handler*)pgpFwdHdlr);
         } else {
           pgpd->handler((RcePgp::Handler*)myHandler);
         }
         RcePgpFwd::PgpForward::forwarding(mask);
       }
       break;
     case 'm':
       quadMask = strtoul(getopt_reent.optarg, 0, 0);
       quadMask &= 0xf;
       break;
     default:
       printf("Option: -%c is not parsable.\n", theOpt);
       printf(cxistatUsage.c_str());
       return 2;
    }
  }
  return 0;
}

void writeData(unsigned addr, uint32_t data) {
  cfgrtr->writeRegister(RcePgp::RegisterSlaveExportFrame::CR, addr, data, true);
}

int main_cxistat(int argc, char **argv) {
  if (!pt) pt = (RceCxi::CxiProxyThread*) RceCxi::CxiProxyThread::instance();
  if (!pgpd) pgpd = RcePgp::Driver::instance();
  if (!myHandler) {
    if (pt) {
      printf("Using Proxy Thread PGP handler\n");
      myHandler = pt->handler();
    } else {
      printf("Making my own PGP handler\n");
      myHandler = new RceCxi::CxiPgpHandler();
    }
  }
  if (!pool) {
    if (pt) {
      printf("Using Proxy Thread pool\n");
      pool = pt->txPool();
    } else {
      printf("Making new Pool\n");
      RcePgp::Params params =  pgpd->params();
      params.tx.type = RcePic::Contiguous;
      params.tx.nbuffers = RceCxi::CxiProxyThread::numbTxBuffers;
      params.tx.maxpayload = 4096;
      pool = new RcePic::Pool::Pool(params.tx, (RcePic::Handler*)myHandler);
      RcePic::Descr* tbd = pool->descriptors.head();
      while (tbd != pool->descriptors.empty()) {
        tbd->flush(0);
        tbd = tbd->flink(); //--??
      }
    }
  }
  if (!iq) {
    if (pt) {
      printf("Using Proxy Thread input queue\n");
      iq = pt->iq();
    } else {
      printf("Making the Interactive input queue\n");
      rtems_name name = rtems_build_name('I', 'N', 'I', 'Q');
      rtems_status_code s = rtems_message_queue_create(name, 512,
          sizeof(RcePgp::RegisterSlaveImportFrame), RTEMS_FIFO | RTEMS_LOCAL, &iq);
      if (s!=RTEMS_SUCCESSFUL) {
        printf("Interactive message queue create failed %d\n", s);
        return 1;
      }
    }
  }
  if (!cfgrtr) {
    if (!pt) {
      printf("Making new configurator and configuration\n");
      pConfig = new pCfg(5, quadMask);
      cfgrtr = new RceCxi::CxiConfigurator(iq, myHandler, pool, (CsPadConfigType*)pConfig);
    } else {
      printf("Using Proxy Thread configurator and configuration\n");
      if (pt->allocConfig()) {
        printf("\tBut making configuration to be overwritten later by the daq system\n");
        new (pt->config()) pCfg(5, quadMask);
      }
      cfgrtr = pt->configurator();
      pConfig = (pCfg*) pt->config();
    }
  }
  if (!ro) {
    ro = new RceCxi::CxiReadOnlyRegisters(iq, pool, myHandler, cfgrtr);
  }
  if (!pgpd->handler()) {
    printf("Found no PGP handler installed !!!\n");
  }
  doEvrEnable = doEventCodeWrite = doRunModeWrite = false;

  int parseRet = cxistatParseOptions(argc, argv);
  if (parseRet) return parseRet;

  if (doEvrEnable) writeData(RceCxi::CxiConfigurator::EnableEvrAddr, RceCxi::CxiConfigurator::EnableEvrValue);
  if (doEventCodeWrite) writeData(RceCxi::CxiConfigurator::EventCodeAddr, value1);
  if (doRunModeWrite) writeData(RceCxi::CxiConfigurator::RunModeAddr, value2);

  if (argc == 1) {
    printf(cxistatUsage.c_str());
  }

  return 0;
}


/** RTEMS structures defining the commands and aliases.
 */
rtems_shell_cmd_t CXISTAT_Command = {
    (char*)"cxistat",                      /* command name */
    (char*)"cxistat [-P -R -T -C -c -L <m> -s -Q <m> -q <m> -f <f> -m <m>-h]",      /* usage */
    (char*)"LCLS",                     /* help topic */
    main_cxistat,                         /* main function */
    NULL,                                 /* alias */
    NULL                                  /* next */
};
