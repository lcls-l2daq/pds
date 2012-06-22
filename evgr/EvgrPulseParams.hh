/*
 * EvgrPulseParams.hh
 *
 *  Created on: Jun 20, 2012
 *      Author: jackp
 */

#ifndef PDSEVGRPULSEPARAMS_HH_
#define PDSEVGRPULSEPARAMS_HH_

namespace Pds {

  class EvgrPulseParams {
    public:
      EvgrPulseParams() {};
      ~EvgrPulseParams() {};

    public:
      unsigned eventcode;
      unsigned polarity;
      unsigned delay;
      unsigned width;
      unsigned output;
  };
}

#endif /* PDSEVGRPULSEPARAMS_HH_ */
