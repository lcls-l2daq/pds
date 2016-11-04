/*
 * PgpCardG3StatusWrap.cc
 *
 *  Created: Mon Feb  1 11:08:11 PST 2016
 *      Author: jackp
 */

#include "pds/pgp/PgpStatus.hh"
#include "pds/pgp/PgpCardG3StatusWrap.hh"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sstream>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <termios.h>


namespace Pds {
  namespace Pgp {

  void PgpCardG3StatusWrap::read() {
      memset(&status,0,sizeof(PgpCardG3Status));
      p->cmd   = IOCTL_Read_Status;
      p->data  = (__u32*)(&status);
      write(_fd, p, sizeof(PgpCardTx));
    }

    unsigned PgpCardG3StatusWrap::checkPciNegotiatedBandwidth() {
      this->read();
      unsigned val = (status.PciLStatus >> 4)  & 0x3f;
      if (val != 4) {
        sprintf(esp, "Negotiated bandwidth too low, %u\n Try reinstalling or replacing PGP G3 card\n", val);
        printf("%s", esp);
        esp = es + strlen(es);
      }
      return  val;
    }

    unsigned PgpCardG3StatusWrap::getCurrentFiducial() {
      this->read();
      return (status.EvrFiducial);
    }

    bool PgpCardG3StatusWrap::getLatestLaneStatus() {
      return ((status.EvrLaneStatus >> pgp()->portOffset())&1);
    }

    bool PgpCardG3StatusWrap::evrEnabled(bool pf) {
      this->read();
      bool enabled = status.EvrEnable && status.EvrReady;
      if ((enabled == false) && pf) {
        printf("PgpCardG3StatusWrap: EVR not enabled, enable %s, ready %s\n",
            status.EvrEnable ? "true" : "false", status.EvrReady ? "true" : "false");
        if (status.EvrReady == false) {
          sprintf(esp, "PgpCardG3StatusWrap: EVR not enabled\nMake sure MCC fiber is connected to pgpG3 card\n");
          printf("%s", esp);
          esp = es+strlen(es);
        }
      }
      return enabled;
    }

    void PgpCardG3StatusWrap::print() {
  	  int           x;
  	  int           y;
      this->read();
      printf("\nPGP Card Status:\n");

      __u64 SerialNumber = status.SerialNumber[0];
      SerialNumber = SerialNumber << 32;
      SerialNumber |= status.SerialNumber[1];

      printf("           Version: 0x%x\n", status.Version);
      printf("      SerialNumber: 0x%llx\n", SerialNumber);
      printf("        BuildStamp: %s\n",(char *) (status.BuildStamp) );
      printf("        CountReset: 0x%x\n", status.CountReset);
      printf("         CardReset: 0x%x\n", status.CardReset);
      printf("        ScratchPad: 0x%x\n\n", status.ScratchPad);

      printf("          PciCommand: 0x%x\n", status.PciCommand);
      printf("           PciStatus: 0x%x\n", status.PciStatus);
      printf("         PciDCommand: 0x%x\n", status.PciDCommand);
      printf("          PciDStatus: 0x%x\n", status.PciDStatus);
      printf("         PciLCommand: 0x%x\n", status.PciLCommand);
      printf("          PciLStatus: 0x%x\n", status.PciLStatus);
      printf("Negotiated Link Width:  %d", (status.PciLStatus >> 4) & 0x3f);
      printf("        PciLinkState: 0x%x\n", status.PciLinkState);
      printf("         PciFunction: 0x%x\n", status.PciFunction);
      printf("           PciDevice: 0x%x\n", status.PciDevice);
      printf("              PciBus: 0x%x\n", status.PciBus);
      printf("         PciBaseAddr: 0x%x\n", status.PciBaseHdwr);
      printf("       PciBaseLength: 0x%x\n\n", status.PciBaseLen);

      printf("            PgpRate(Gbps): %f\n",  ((double)status.PpgRate)*1.0E-3);
      printf("         PgpLoopBack[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u", status.PgpLoopBack[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("          PgpTxReset[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u", status.PgpTxReset[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("          PgpRxReset[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u", status.PgpRxReset[x]);
        if(x!=7) printf(", "); else printf("\n");
      }


      printf(" PgpTxCommonPllReset[3:0],[7:4]: 0x%x 0x%x\n", status.PgpTxPllRst[0], status.PgpTxPllRst[1]);


      printf(" PgpRxCommonPllReset[3:0],[7:4]: 0x%x 0x%x\n", status.PgpRxPllRst[0], status.PgpRxPllRst[1]);


      printf("PgpTxCommonPllLocked[3:0],[7:4]: 0x%x 0x%x\n", status.PgpTxPllRdy[0], status.PgpTxPllRdy[1]);


      printf("PgpRxCommonPllLocked[3:0],[7:4]: 0x%x 0x%x\n", status.PgpRxPllRdy[0], status.PgpRxPllRdy[1]);

      printf("     PgpLocLinkReady[0:7]: ");
      for(x=0;x<8;x++) {
         printf("%u", status.PgpLocLinkReady[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("     PgpRemLinkReady[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u", status.PgpRemLinkReady[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      for(x=0;x<8;x++) {
        printf("       PgpRxCount[%u][0:3]:", x );
        for(y=0;y<4;y++) {
          printf("%u",status.PgpRxCount[x][y]);
          if(y!=3) printf(", "); else printf("\n");
        }
      }

      printf("       PgpCellErrCnt[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u",status.PgpCellErrCnt[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("      PgpLinkDownCnt[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u",status.PgpLinkDownCnt[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("       PgpLinkErrCnt[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u",status.PgpLinkErrCnt[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("       PgpFifoErrCnt[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u",status.PgpFifoErrCnt[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("\n");
      printf("        EvrRunCode[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrRunCode[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("     EvrAcceptCode[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrAcceptCode[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("       EvrRunDelay[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrRunDelay[x]);
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("    EvrAcceptDelay[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrAcceptDelay[x]);
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("   EvrRunCodeCount[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrRunCodeCount[x]);
        if (x==7) printf("\n"); else printf(", ");
      }

      for(x=0;x<8;x++) {
        printf("EvrLutDropCount[%x][0:3]: ", x);
        for(y=0;y<4;y++) {
          printf("%d", ((status.EvrLutDropCount[x]>>(y*8))&0xff));
          if(y!=3) printf(", "); else printf("\n");
        }
      }

      printf("    EvrAcceptCount[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrAcceptCount[x]);
        if (x==7) printf("\n"); else printf(", ");
      }


      printf("       EvrLaneMode[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", ((status.EvrLaneMode>>x)&1));
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("  EvrLaneFiducials[0:7]: ");
      for(x=0;x<8;x++) {
        printf("0x%x", status.EvrLaneFiducials[x]);
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("     EvrLaneEnable[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", ((status.EvrLaneEnable>>x)&1));
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("     EvrLaneStatus[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", ((status.EvrLaneStatus>>x)&1));
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("              EvrEnable: %d\n", status.EvrEnable);
      printf("               EvrReady: %d\n", status.EvrReady);
      printf("               EvrReset: %d\n", status.EvrReset);
      printf("              EvrPllRst: %d\n", status.EvrPllRst);
      printf("              EvrErrCnt: %d\n", status.EvrErrCnt);
      printf("              EvrFiducial: 0x%x\n", status.EvrFiducial);
      for(x=0;x<8;x++) {
        printf("  EvrEnHdrCheck[%d][0:3]: ", x);
        for(y=0;y<4;y++) {
          printf("%d", status.EvrEnHdrCheck[x][y]);
          if(y!=3) printf(", "); else printf("\n");
        }
      }
      printf("\n");

      printf("      TxDmaFull[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.TxDmaAFull[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("         TxReadReady: 0x%x\n", status.TxReadReady);
      printf("      TxRetFifoCount: 0x%x\n", status.TxRetFifoCount);
      printf("             TxCount: 0x%x\n", status.TxCount);
      printf("              TxRead: 0x%x\n", status.TxRead);
      printf("\n");

      printf("     RxFreeFull[0:7]: ");
      for(x=0;x<8;x++) {
         printf("%d", status.RxFreeFull[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("    RxFreeValid[0:7]: ");
      for(x=0;x<8;x++) {
         printf("%d", status.RxFreeValid[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("RxFreeFifoCount[0:7]: ");
      for(x=0;x<8;x++) {
        printf("0x%x", status.RxFreeFifoCount[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("         RxReadReady: 0x%x\n", status.RxReadReady);
      printf("      RxRetFifoCount: 0x%x\n", status.RxRetFifoCount);
      printf("             RxCount: 0x%x\n", status.RxCount);
      printf("        RxWrite[0:7]: ");
      for(x=0;x<8;x++) {
        printf("0x%x", status.RxWrite[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("         RxRead[0:7]: ");
      for(x=0;x<8;x++) {
        printf("0x%x", status.RxRead[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("\n");

    }
  }
}
