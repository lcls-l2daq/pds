/*
 * PGPCardStatus.cc
 *
 *  Created on: Jan 13, 2016
 *      Author: jackp
 */


#include "pds/pgp/Destination.hh"
#include "pds/pgp/PgpStatus.hh"
#include "pds/pgp/PgpCardStatusWrap.hh"
#include "pgpcard/PgpCardStatus.h"
#include <fcntl.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>

namespace Pds {

  namespace Pgp {

      void PgpCardStatusWrap::read() {
        memset(&status,0,sizeof(PgpCardStatus));
        p->cmd   = IOCTL_Read_Status;
        p->data  = (__u32*)&status;
        write(_fd, p, sizeof(PgpCardTx));
      } 
      
      unsigned PgpCardStatusWrap::checkPciNegotiatedBandwidth() {
          this->read();
          return (status.PciLStatus >> 4)  & 0x3f;
      }
      
      void PgpCardStatusWrap::print() {
    	  int           i;
    	  this->read();
    	   std::cout << "PGP Card Status:      0 1 2 3 <-- Lane order" << std::endl;
    	   std::cout << "            Version: " << std::hex << std::setw(8) << std::setfill('0') << status.Version << std::endl;
    	   std::cout << "         ScratchPad: " << std::hex << std::setw(8) << std::setfill('0') << status.ScratchPad << std::endl;
    	   std::cout << "         PciCommand: " << std::hex << std::setw(4) << std::setfill('0') << status.PciCommand << std::endl;
    	   std::cout << "          PciStatus: " << std::hex << std::setw(4) << std::setfill('0') << status.PciStatus << std::endl;
    	   std::cout << "        PciDCommand: " << std::hex << std::setw(4) << std::setfill('0') << status.PciDCommand << std::endl;
    	   std::cout << "         PciDStatus: " << std::hex << std::setw(4) << std::setfill('0') << status.PciDStatus << std::endl;
    	   std::cout << "        PciLCommand: " << std::hex << std::setw(4) << std::setfill('0') << status.PciLCommand << std::endl;
    	   std::cout << "         PciLStatus: " << std::hex << std::setw(4) << std::setfill('0') << status.PciLStatus << std::endl;
    	   std::cout << "Negotiated Lnk Wdth: " << std::dec << std::setw(1) << ((status.PciLStatus >> 4) & 0x3f) << std::endl;
    	   std::cout << "       PciLinkState: " << std::hex << std::setw(1) << std::setfill('0') << status.PciLinkState << std::endl;
    	   std::cout << "        PciFunction: " << std::hex << std::setw(1) << std::setfill('0') << status.PciFunction << std::endl;
    	   std::cout << "          PciDevice: " << std::hex << std::setw(1) << std::setfill('0') << status.PciDevice << std::endl;
    	   std::cout << "             PciBus: " << std::hex << std::setw(2) << std::setfill('0') << status.PciBus << std::endl;
    	   std::cout << "       Pgp0LoopBack: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpLoopBack << " ";
    	   }
    	   std::cout <<  std::endl << "        Pgp0RxReset: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpRxReset << " ";
    	   }
    	   std::cout << std::endl << "        Pgp0TxReset: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpTxReset << " ";
    	   }
    	   std::cout << std::endl << "   Pgp0LocLinkReady: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpLocLinkReady << " ";
    	   }
    	   std::cout << std::endl << "   Pgp0RemLinkReady: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpRemLinkReady << " ";
    	   }
    	   std::cout << std::endl << "        Pgp0RxReady: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpRxReady << " ";
    	   }
    	   std::cout << std::endl << "        Pgp0TxReady: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpTxReady << " ";
    	   }
    	   std::cout << std::endl << "        Pgp0RxCount: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpRxCount << " ";
    	   }
    	   std::cout << std::endl << "     Pgp0CellErrCnt: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpCellErrCnt << " ";
    	   }
    	   std::cout << std::endl << "    Pgp0LinkDownCnt: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpLinkDownCnt << " ";
    	   }
    	   std::cout << std::endl << "     Pgp0LinkErrCnt: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpLinkErrCnt << " ";
    	   }
    	   std::cout << std::endl << "        Pgp0FifoErr: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(1) << status.PgpLink[i].PgpFifoErr << " ";
    	   }
    	   std::cout << std::endl;
    	   std::cout << "        TxDma3AFull: " << std::hex << std::setw(1) << std::setfill('0') << status.TxDma3AFull << std::endl;
    	   std::cout << "        TxDma2AFull: " << std::hex << std::setw(1) << std::setfill('0') << status.TxDma2AFull << std::endl;
    	   std::cout << "        TxDma1AFull: " << std::hex << std::setw(1) << std::setfill('0') << status.TxDma1AFull << std::endl;
    	   std::cout << "        TxDma0AFull: " << std::hex << std::setw(1) << std::setfill('0') << status.TxDma0AFull << std::endl;
    	   std::cout << "        TxReadReady: " << std::hex << std::setw(1) << std::setfill('0') << status.TxReadReady << std::endl;
    	   std::cout << "     TxRetFifoCount: " << std::hex << std::setw(3) << std::setfill('0') << status.TxRetFifoCount << std::endl;
    	   std::cout << "            TxCount: " << std::hex << std::setw(8) << std::setfill('0') << status.TxCount << std::endl;
    	   std::cout << "      TxBufferCount: " << std::hex << std::setw(2) << std::setfill('0') << status.TxBufferCount << std::endl;
    	   std::cout << "             TxRead: " << std::hex << std::setw(2) << std::setfill('0') << status.TxRead  << std::endl;
    	   std::cout << "        RxFreeEmpty: " << std::hex << std::setw(1) << std::setfill('0') << status.RxFreeEmpty << std::endl;
    	   std::cout << "         RxFreeFull: " << std::hex << std::setw(1) << std::setfill('0') << status.RxFreeFull << std::endl;
    	   std::cout << "        RxFreeValid: " << std::hex << std::setw(1) << std::setfill('0') << status.RxFreeValid << std::endl;
    	   std::cout << "    RxFreeFifoCount: " << std::hex << std::setw(3) << std::setfill('0') << status.RxFreeFifoCount << std::endl;
    	   std::cout << "        RxReadReady: " << std::hex << std::setw(1) << std::setfill('0') << status.RxReadReady << std::endl;
    	   std::cout << "     RxRetFifoCount: " << std::hex << std::setw(3) << std::setfill('0') << status.RxRetFifoCount << std::endl;
    	   std::cout << "            RxCount: " << std::hex << std::setw(8) << std::setfill('0') << status.RxCount << std::endl;
    	   std::cout << "      RxBufferCount: " << std::hex << std::setw(2) << std::setfill('0') << status.RxBufferCount << std::endl << "    RxWrite[client]: " ;
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(2) << status.RxWrite[i] << " ";
    	   }
    	   std::cout << std::endl << "     RxRead[client]: ";
    	   for (i=0; i<4; i++ ) {
    	     std::cout << std::hex << std::setw(2) << status.RxRead[i] << " ";
    	   }
    	   std::cout << std::endl;
      }
    }
}

