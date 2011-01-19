
#include "AcqFinder.hh"
#include <stdio.h>
#include <string.h>

using namespace Pds;

AcqFinder::AcqFinder(Method m) {
  ViStatus status;
  ViString options;

  // Find all digitizers
  options = "cal=0,dma=1";
  ViInt32 numInstruments;

  if (m==MultiInstrumentsOnly) {
    status = AcqrsD1_multiInstrAutoDefine(options, &numInstruments);
  }
  else {
    //
    //  The above call does not recognize T3 devices
    //  The following call should recognize all instruments
    //  Will likely need to manually configure multi-instruments
    //
    status = Acqrs_getNbrInstruments(&numInstruments);
  }

  printf("AcqFinder found %d instruments\n", int(numInstruments));

  // Initialize the digitizers
  _numD1 = _numT3 = 0;
  for (int i = 0; i < numInstruments; i++)
    {
      char resourceName[20];
      sprintf(resourceName, "PCI::INSTR%d", i);

      ViSession instrumentId;
      if ((status = Acqrs_InitWithOptions(resourceName, VI_FALSE, VI_FALSE, options,
					  &instrumentId)) != VI_SUCCESS)
        {
          char message[256];
          Acqrs_errorMessage(instrumentId,status,message,256);
          printf("%s\n",message);
        }

      char name[64];
      ViInt32 serNo, busNo, slotNo;
      if ((status = Acqrs_getInstrumentData(instrumentId, name, &serNo, &busNo, &slotNo))
	  != VI_SUCCESS)
        {
          char message[256];
          Acqrs_errorMessage(instrumentId,status,message,256);
          printf("%s\n",message);
        }
      else
	{
	  printf("Found instrument %s : sernum %d  bus %d  slot %d\n",
		 name, int(serNo), int(busNo), int(slotNo));
	  if (strstr(name,"TC890"))
	    _T3Id[_numT3++] = instrumentId;
	  else
	    _D1Id[_numD1++] = instrumentId;
	}
    }
}
