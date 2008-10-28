
#include "AcqFinder.hh"
#include <stdio.h>

using namespace Pds;

AcqFinder::AcqFinder() {
  ViStatus status;
  ViString options;

  // Find all digitizers
  options = "cal=0 dma=1";

  // The following call will find the number of digitizers on the computer, regardless of
  // their connection(s) to ASBus.
  //		status = AcqrsD1_getNbrPhysicalInstruments(&NumInstruments);

  // The following call will automatically detect ASBus connections between digitizers and
  // combine connected digitizers (they must be the same model!) into multi-instruments.
  // The call returns the number of multi-instruments and/or single instruments.
  status = AcqrsD1_multiInstrAutoDefine(options, &_numInstruments);

  // Initialize the digitizers
  for (int i = 0; i < _numInstruments; i++)
    {
      char resourceName[20];
      sprintf(resourceName, "PCI::INSTR%d", i);

      status = AcqrsD1_InitWithOptions(resourceName, VI_FALSE, VI_FALSE, options,
                                       &(_instrumentId[i]));

      if(status != VI_SUCCESS)
        {
          char message[256];
          AcqrsD1_errorMessage(_instrumentId[i],status,message);
          printf("%s\n",message);
        }
    }
}
