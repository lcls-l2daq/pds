/*
 * CspadServer.cc
 *
 *  Created on: Nov 15, 2010
 *      Author: jackp
 */

#include "CspadServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/config/CsPadDataType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/csPad/CspadConfigurator.hh"
#include "PgpCardMod.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <string.h>

using namespace Pds;
//using namespace Pds::CsPad;

CspadServer::CspadServer( const Pds::Src& client )
   : _xtc( _CsPadDataType, client ) {}

unsigned CspadServer::configure(CsPadConfigType& config) {
  unsigned ret = 0;
  _cfg = new Pds::CsPad::CspadConfigurator::CspadConfigurator(config, fd());
  if ((ret = _cfg->configure())) {
    printf("CspadServer::configure failed %u\n", ret);
  } else {
    _quads = 0;
    for (unsigned i=0; i<4; i++) if ((1<<i) & config.quadMask()) _quads += 1;
    _payloadSize = config.payloadSize();
    _xtc.extent = (_payloadSize * _quads) + sizeof(Xtc);
    printf("CspadServer::configure _quads(%u) _payloadSize(%u) _xtc.extent(%u)\n",
        _quads, _payloadSize, _xtc.extent);
  }
  _count = _quadsThisCount = 0;
  return ret;
}

unsigned Pds::CspadServer::unconfigure(void) { return 0; }

int Pds::CspadServer::fetch( char* payload, int flags ) {
   int ret = 0;
   PgpCardRx       pgpCardRx;
   char*           dest = payload;

   _xtc.damage = 0;

   _quadsThisCount %= _quads;

   if (!_quadsThisCount) {
     memcpy( payload, &_xtc, sizeof(Xtc) );
     dest += sizeof(Xtc);
   }

   pgpCardRx.model   = sizeof(&pgpCardRx);
   pgpCardRx.maxSize = _payloadSize / sizeof(__u32);
   pgpCardRx.data    = (__u32*)dest;

   ret = read(fd(), &pgpCardRx, sizeof(PgpCardRx));
   if (ret < 0) {
     perror ("CspadServer::fetch pgpCard read error");
   } else {
     unsigned damageMask = 0;
     if (pgpCardRx.eofe)      damageMask |= 1;
     if (pgpCardRx.fifoErr)   damageMask |= 2;
     if (pgpCardRx.lengthErr) damageMask |= 4;
     if (damageMask) {
       damageMask |= 0xe0;
       _xtc.damage.increase(Pds::Damage::UserDefined);
       _xtc.damage.userBits(damageMask);
       printf("CsPadServer::fetch setting user damage 0x%x\n", damageMask);
     } else {
       Pds::Pgp::DataImportFrame* data = (Pds::Pgp::DataImportFrame*)dest;
       unsigned oldCount = _count;
       _count = data->frameNumber() - 1;  // cspad starts counting at 1, not zero
       if ((_count != oldCount) && (_quadsThisCount)) {
         _quadsThisCount = 0;
         memcpy( payload, &_xtc, sizeof(Xtc) );
         ret = sizeof(Xtc);
       }
     }
     _quadsThisCount += 1;
   }
   return ret;
}

unsigned CspadServer::offset() const {
  return (_quadsThisCount==1 ? 0 : sizeof(Xtc) + _payloadSize * (_quadsThisCount-1));
}

unsigned CspadServer::count() const {
   if( ( _count % 1000 ) == 0 ) {
      printf( "in CspadServer::count, fd(%d) _count(%u)\n",
              fd(), _count);
   }
   return _count;
}

void CspadServer::setCspad( int f ) {
   fd( f );
   Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
}
