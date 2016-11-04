/*
 * Pgp.hh
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#ifndef PGP_HH_
#define PGP_HH_

#include "pds/pgp/PgpRSBits.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/Destination.hh"
#include "pds/pgp/PgpStatus.hh"

namespace Pds {
	namespace Pgp {

	  class PgpStatus;

		class Pgp {
			public:
				Pgp(int, bool printFlag = true);
				virtual ~Pgp();

			public:
				enum {BufferWords=8192};
				enum {Success=0, Failure=1, SelectSleepTimeUSec=10000};
				Pds::Pgp::RegisterSlaveImportFrame* read(
						unsigned size = (sizeof(Pds::Pgp::RegisterSlaveImportFrame)/sizeof(uint32_t)));
				unsigned       writeRegister(
						Destination*,
						unsigned,
						uint32_t,
						bool pf = false,
						Pds::Pgp::PgpRSBits::waitState = Pds::Pgp::PgpRSBits::notWaiting);
				// NB size should be the size of data to be written in uint32_t's
				unsigned       writeRegisterBlock(
						Destination*,
						unsigned,
						uint32_t*,
						unsigned size = 1,
						Pds::Pgp::PgpRSBits::waitState = Pds::Pgp::PgpRSBits::notWaiting,
						bool pf=false);

				// NB size should be the size of the block requested in uint32_t's
				unsigned      readRegister(
						Destination*,
						unsigned,
						unsigned,
						uint32_t*,
						unsigned size=1,
						bool pf=false);
				unsigned      checkPciNegotiatedBandwidth();
				unsigned      getCurrentFiducial();
				bool          getLatestLaneStatus();
				int           resetPgpLane();
				int           resetSequenceCount();
				void          printStatus();
				unsigned      stopPolling();
				int           IoctlCommand(unsigned command, unsigned arg = 0);
				int           IoctlCommand(unsigned command, long long unsigned arg = 0);
				void          maskHWerror(bool m) { _maskHWerror = m; }
        bool          G3Flag() {return _myG3Flag;}
        char*         errorString();
        void          errorStringAppend(char*);
        void          clearErrorString();
				static void   portOffset(unsigned p) { _portOffset = p; }
				static unsigned portOffset() { return _portOffset; }
        bool          evrEnabled(bool);
			private:
				int                  _fd;
				unsigned             _readBuffer[BufferWords];
				PgpCardTx            _pt;
				Pds::Pgp::PgpStatus* _status;
				static unsigned      _portOffset;
				unsigned             _maskedHWerrorCount[4];
				bool				         _maskHWerror;
				bool                 _myG3Flag;
		};
	}
}

#endif /* PGP_HH_ */
