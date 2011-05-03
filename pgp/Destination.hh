/*
 * Destination.hh
 *
 *  Created on: Apr 13, 2011
 *      Author: jackp
 */

#ifndef DESTINATION_HH_
#define DESTINATION_HH_

namespace Pds {

  namespace Pgp {

    class Destination {
      public:
        Destination() {};
        Destination(unsigned d) : _dest(d) {};
        virtual ~Destination() {};

      public:
        void dest(unsigned d) { _dest = d; }
        virtual unsigned lane() = 0;
        virtual unsigned vc() = 0;
        virtual char*    name() = 0;

      protected:
        unsigned _dest;
    };

  }

}

#endif /* DESTINATION_HH_ */
