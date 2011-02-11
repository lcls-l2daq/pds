/*
 * CspadDirectRegisterReader.cc
 *
 *  Created on: Jan 20, 2011
 *      Author: jackp
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "PgpCardMod.h"
#include "CspadDirectRegisterReader.hh"

using namespace Pds::CsPad;

int CspadDirectRegisterReader::_readPgpCard(Pds::Pgp::RegisterSlaveImportFrame* rsif) {
  enum {SleepTimeUSec=100000, BufferWords=2048};
  int ret = 0;
  fd_set          fds;
  int             sret = 0;
  unsigned        _buffer[BufferWords];
  struct timeval  timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = SleepTimeUSec;
  PgpCardRx       pgpCardRx;
  pgpCardRx.model   = sizeof(&pgpCardRx);
  pgpCardRx.maxSize = BufferWords;
  pgpCardRx.data    = (__u32*)_buffer;
  FD_ZERO(&fds);
  FD_SET(_fd,&fds);
  unsigned readRet = 0;
  if ((sret = select(_fd+1,&fds,NULL,NULL,&timeout)) > 0) {
    readRet = ::read(_fd, &pgpCardRx, sizeof(PgpCardRx));
    if (pgpCardRx.eofe || pgpCardRx.fifoErr || pgpCardRx.lengthErr) {
      printf("Cspad Config Receiver pgpcard error eofe(%u), fifoErr(%u), lengthErr(%u)\n",
          pgpCardRx.eofe, pgpCardRx.fifoErr, pgpCardRx.lengthErr);
      printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
      ret = -1;
    } else {
      memcpy(rsif, _buffer, sizeof(Pds::Pgp::RegisterSlaveImportFrame));
      if (readRet != sizeof(Pds::Pgp::RegisterSlaveImportFrame)/sizeof(uint32_t)) {
        printf("CspadDirectRegisterReader::_readPgpCard read returned %u\n", readRet);
        rsif->print();
        ret = -1;
      } else {
        if (rsif->failed()) {
          printf("CspadDirectRegisterReader::_readPgpCard receive HW failed\n");
          printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
          rsif->print();
          ret = -1;
        }
        if (rsif->timeout()) {
          printf("CspadDirectRegisterReader::_readPgpCard receive HW timed out\n");
          printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
          rsif->print();
          ret = -1;
        }
      }
    }
  } else {
    ret = -1;
    if (sret < 0) {
      perror("CspadDirectRegisterReader::_readPgpCard select error: ");
      printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
    } else {
      printf("CspadDirectRegisterReader::_readPgpCard select timed out!\n");
    }
  }
  return ret;
}

int CspadDirectRegisterReader::readRegister(
    Pds::Pgp::RegisterSlaveExportFrame::FEdest dest,
    unsigned addr,
    unsigned tid,
    uint32_t* retp)
{
  enum {Success=0, Failure=-1};
  Pds::Pgp::RegisterSlaveImportFrame rsif;
  Pds::Pgp::RegisterSlaveExportFrame  rsef = Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
      Pds::Pgp::RegisterSlaveExportFrame::read,
      dest,
      addr,
      tid,
      (uint32_t)0,
      Pds::Pgp::RegisterSlaveExportFrame::Waiting);
  if (rsef.post(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) != Success) {
    printf("CspadDirectRegisterReader::readRegister failed, export frame follows.\n");
    rsef.print(0, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t));
    return  Failure;
  }
  unsigned errorCount = 0;
  int ret = 0;
  while (true) {
    ret = _readPgpCard(&rsif);
    if (ret == -1) {
      printf("CspadDirectRegisterReader::readRegister _readPgpCard failed!\n");
      return Failure;
    }
    if (addr != rsif.addr()) {
      printf("CspadDirectRegisterReader::readRegister out of order response dest=%u, addr=0x%x, tid=%u, errorCount=%u\n",
          dest, addr, tid, ++errorCount);
      rsif.print();
      if (errorCount > 5) return Failure;
    } else {
      *retp = rsif.data();
    } return Success;
  }
}
