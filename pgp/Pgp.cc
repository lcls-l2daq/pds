/*
 * Pgp.cc
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#include "pds/pgp/Pgp.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/PgpRSBits.hh"
#include "pds/pgp/Destination.hh"
#include "pds/pgp/PgpStatus.hh"
#include "pds/pgp/PgpCardStatusWrap.hh"
#include "pds/pgp/PgpCardG3StatusWrap.hh"
#include "pgpcard/PgpCardMod.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <new>

using namespace Pds::Pgp;

unsigned Pds::Pgp::Pgp::_portOffset = 0;

Pgp::Pgp(int f, bool pf) : _fd(f) {
	if (pf) printf("Pgp::Pgp(fd(%d)), offset(%u)\n", f, _portOffset);
	_pt.model = sizeof(&_pt);
	_pt.size = sizeof(PgpCardTx); _pt.pgpLane = 0;  _pt.pgpVc = 0;
	for (unsigned i=0;i<4;i++) {_maskedHWerrorCount[i]= 0;}
	_maskHWerror = false;
	Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
	for (int i=0; i<BufferWords; i++) _readBuffer[i] = i;
	_status = 0;
	// See if we can determine the type of card
	_myG3Flag = false;
	unsigned* ptr = (unsigned*)malloc(sizeof(PgpCardG3Status)); // the larger one
	int r = this->IoctlCommand(IOCTL_Read_Status, (long long unsigned)ptr);
	if (r<0) {
		perror("Unable to read the card status!\n");
	} else {
		_myG3Flag = ((ptr[0]>>12) & 0xf) == 3;
		printf("Pgp::Pgp found card version 0x%x which I see is %sa G3 card\n", ptr[0], _myG3Flag ? "" : "NOT ");
	}
	free(ptr);
	if (_myG3Flag) {
		_status = new PgpCardG3StatusWrap(_fd, 0, this);
	} else {
		_status = new PgpCardStatusWrap(_fd, 0, this);
	}
}

Pgp::~Pgp() {}

bool Pgp::evrEnabled(bool pf) {
	bool ret = true;
	if (_myG3Flag) {
		ret = _status->evrEnabled(pf);
	}
	return ret;
}

unsigned      Pgp::Pgp::checkPciNegotiatedBandwidth() {
  return _status->checkPciNegotiatedBandwidth();
}
unsigned      Pgp::Pgp::getCurrentFiducial() {
  return _status->getCurrentFiducial();
}
bool          Pgp::Pgp::getLatestLaneStatus() {
  return _status->getLatestLaneStatus();
}
int      Pgp::Pgp::resetSequenceCount() {
  return Pgp::IoctlCommand(IOCTL_ClearFrameCounter, (unsigned)(1<<portOffset()));
}
int      Pgp::Pgp::maskRunTrigger(unsigned mask, bool b) {
  unsigned flag = b ? 0 : 1;
  printf("Pgp::maskRunTrigger(0x%x, %s)\n", mask, flag == 0 ? "False" : "True");
  return Pgp::IoctlCommand(IOCTL_Evr_RunMask, (unsigned)((mask<<24) | flag));
}
char*         Pgp::Pgp::errorString() {
  return _status->errorString();
}
void          Pgp::Pgp::errorStringAppend(char* s) {
  _status->errorStringAppend(s);
}
void          Pgp::Pgp::clearErrorString() {
  _status->clearErrorString();
}


void Pgp::Pgp::printStatus() {
	_status->print();
	for(unsigned u=0; u<4; u++) {
		if (_maskedHWerrorCount[u]) {
			printf("Masked Hardware Errors for vc %u are %u\n\n", u, _maskedHWerrorCount[u]);
			_maskedHWerrorCount[u] = 0;
		}
	}
}

Pds::Pgp::RegisterSlaveImportFrame* Pgp::Pgp::read(unsigned size) {
	Pds::Pgp::RegisterSlaveImportFrame* ret = (Pds::Pgp::RegisterSlaveImportFrame*)Pds::Pgp::Pgp::_readBuffer;
	int             sret = 0;
	struct timeval  timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = Pds::Pgp::Pgp::SelectSleepTimeUSec;
	PgpCardRx       pgpCardRx;
	pgpCardRx.model   = sizeof(&pgpCardRx);
	pgpCardRx.maxSize = Pds::Pgp::Pgp::BufferWords;
	pgpCardRx.data    = (__u32*)Pds::Pgp::Pgp::_readBuffer;
	fd_set          fds;
	FD_ZERO(&fds);
	FD_SET(Pds::Pgp::Pgp::_fd,&fds);
	int readRet = 0;
	bool   found  = false;
	while (found == false) {
		if ((sret = select(Pds::Pgp::Pgp::_fd+1,&fds,NULL,NULL,&timeout)) > 0) {
			if ((readRet = ::read(Pds::Pgp::Pgp::_fd, &pgpCardRx, sizeof(PgpCardRx))) >= 0) {
				if ((ret->waiting() == Pds::Pgp::PgpRSBits::Waiting) || (ret->opcode() == Pds::Pgp::PgpRSBits::read)) {
					found = true;
					if (pgpCardRx.eofe || pgpCardRx.fifoErr || pgpCardRx.lengthErr) {
						printf("Pgp::read error eofe(%u), fifoErr(%u), lengthErr(%u)\n",
								pgpCardRx.eofe, pgpCardRx.fifoErr, pgpCardRx.lengthErr);
						printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
						ret = 0;
					} else {
						if (readRet != (int)size) {
							printf("Pgp::read read returned %u, we were looking for %u\n", readRet, size);
							ret->print(readRet);
							ret = 0;
						} else {
							bool hardwareFailure = false;
							uint32_t* u = (uint32_t*)ret;
							if (ret->failed((Pds::Pgp::LastBits*)(u+size-1))) {
								if (_maskHWerror == false) {
									printf("Pgp::read received HW failure\n");
									ret->print();
									hardwareFailure = true;
								} else {
									_maskedHWerrorCount[pgpCardRx.pgpVc] += 1;
								}
							}
							if (ret->timeout((Pds::Pgp::LastBits*)(u+size-1))) {
								printf("Pgp::read received HW timed out\n");
								ret->print();
								hardwareFailure = true;
							}
							if (hardwareFailure) ret = 0;
						}
					}
				}
			} else {
				perror("Pgp::read() ERROR ! ");
				ret = 0;
				found = true;
			}
		} else {
			found = true;  // we might as well give up!
			ret = 0;
			if (sret < 0) {
				perror("Pgp::read select error: ");
				printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
			} else {
				printf("Pgp::read select timed out!\n");
			}
		}
	}
	return ret;
}

unsigned Pgp::Pgp::stopPolling() {
	PgpCardTx* p = &_pt;

	p->cmd   = IOCTL_Clear_Polling;
	p->data  = 0;
	return(write(_fd, &p, sizeof(PgpCardTx)));
}

unsigned Pgp::Pgp::writeRegister(
		Destination* dest,
		unsigned addr,
		uint32_t data,
		bool printFlag,
		Pds::Pgp::PgpRSBits::waitState w) {
	Pds::Pgp::RegisterSlaveExportFrame rsef = Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
			Pds::Pgp::PgpRSBits::write,
			dest,
			addr,
			0x6969,
			data,
			w);
	if (printFlag) rsef.print();
	return rsef.post(sizeof(rsef)/sizeof(uint32_t));
}

unsigned Pgp::Pgp::writeRegisterBlock(
		Destination* dest,
		unsigned addr,
		uint32_t* data,
		unsigned inSize,  // the size of the block to be exported
		Pds::Pgp::PgpRSBits::waitState w,
		bool pf) {
	// the size of the export block plus the size of block to be exported minus the one that's already counted
	unsigned size = (sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) +  inSize -1;
	uint32_t myArray[size];
	Pds::Pgp::RegisterSlaveExportFrame* rsef = 0;
	rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
			Pds::Pgp::PgpRSBits::write,
			dest,
			addr,
			0x6970,
			0,
			w);
	memcpy(rsef->array(), data, inSize*sizeof(uint32_t));
	rsef->array()[inSize] = 0;
	if (pf) rsef->print(0, size);
	return rsef->post(size);
}

unsigned Pgp::Pgp::readRegister(
		Destination* dest,
		unsigned addr,
		unsigned tid,
		uint32_t* retp,
		unsigned size,
		bool pf) {
	Pds::Pgp::RegisterSlaveImportFrame* rsif;
	Pds::Pgp::RegisterSlaveExportFrame  rsef = Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
			Pds::Pgp::PgpRSBits::read,
			dest,
			addr,
			tid,
			size - 1,  // zero = one uint32_t, etc.
			Pds::Pgp::PgpRSBits::Waiting);
	//      if (size>1) {
	//        printf("Pgp::readRegister size %u\n", size);
	//        rsef.print();
	//      }
	if (pf) rsef.print();
	if (rsef.post(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t),pf) != Success) {
		printf("Pgp::readRegister failed, export frame follows.\n");
		rsef.print(0, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t));
		return  Failure;
	}
	unsigned errorCount = 0;
	while (true) {
		rsif = this->read(size + 3);
		if (rsif == 0) {
			printf("Pgp::readRegister _pgp->read failed!\n");
			return Failure;
		}
		if (pf) rsif->print(size + 3);
		if (addr != rsif->addr()) {
			printf("Pds::Pgp::readRegister out of order response lane=%u, vc=%u, addr=0x%x, tid=%u, errorCount=%u\n",
					dest->lane(), dest->vc(), addr, tid, ++errorCount);
			rsif->print(size + 3);
			if (errorCount > 5) return Failure;
		} else {  // copy the data
			memcpy(retp, rsif->array(), size * sizeof(uint32_t));
			return Success;
		}
	}
}

int Pgp::Pgp::IoctlCommand(unsigned c, unsigned a) {
	PgpCardTx* p = &_pt;
	p->cmd   = c;
	p->data  = (__u32*) a;
	printf("IoctlCommand %u writing unsigned 0x%x\n", c, a);
	return(write(_fd, &_pt, sizeof(PgpCardTx)));
}

int Pgp::Pgp::IoctlCommand(unsigned c, long long unsigned a) {
	PgpCardTx* p = &_pt;
	p->cmd   = c;
	p->data  = (__u32*) a;
//	printf("IoctlCommand %u writing long long 0x%llx\n", c, a);
	return(write(_fd, &_pt, sizeof(PgpCardTx)));
}

int Pgp::Pgp::resetPgpLane() {
  unsigned l = _portOffset;
  int ret = 0;
  ret |= IoctlCommand(IOCTL_Set_Tx_Reset, l);
  ret |= IoctlCommand(IOCTL_Clr_Tx_Reset, l);
  ret |= IoctlCommand(IOCTL_Set_Rx_Reset, l);
  ret |= IoctlCommand(IOCTL_Clr_Rx_Reset, l);
  return ret;
}
