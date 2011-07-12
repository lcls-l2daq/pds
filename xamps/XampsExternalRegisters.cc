/*
 * XampsExternalRegisters.cc
 *
 *  Created on: May 10, 2011
 *      Author: jackp
 */

#include <stdio.h>
#include <string.h>
#include "pds/xamps/XampsExternalRegisters.hh"
#include "pds/xamps/XampsConfigurator.hh"
#include "pds/xamps/XampsDestination.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"

namespace Pds {
  namespace Xamps {

    class XampsDestination;
    class XtrnalRegister;

    static uint32_t _xrfoo[][2] = {
        {   0x11,  0xFFFF },  // Xamps Firmware Version
        {   0x30,  0x1FF  },  // Temp_Ana0
        {   0x31,  0x1FF  },  // Temp_Ana1
        {   0x32,  0x1FF  },  // Temp_Ana2
        {   0x33,  0x1FF  },  // Temp_Ana3
        {   0x34,  0x1FF  },  // Temp_Ana4
        {   0x35,  0x1FF  },  // Temp_Dig0
        {   0x36,  0x1FF  },  // Temp_Dig1
        {   0x37,  0x1FF  },  // Temp_Dig2
        {   0x38,  0xFFF  },  // Thermmistor_A
        {   0x39,  0xFFF  },  // Thermmistor_B
        {   0x3a,  0xFFF  },  // Thermmistor_C
        {   0x41,  0xFFF  },  // HVV_ReadBack
        {   0x42,  0xFFF  },  // HVI_ReadBack
        {   0x60,  0xFFF  },  // Sw_Lvl_Ch0
        {   0x61,  0xFFF  },  // Sw_Lvl_Ch1
        {   0x62,  0xFFF  },  // Sw_Lvl_Ch2
        {   0x63,  0xFFF  },  // Sw_Lvl_Ch3
        {   0x64,  0xFFF  }   // Sw_Lvl_Ch4
    };

    static XtrnalRegister* _xr = (XtrnalRegister*) _xrfoo;

    static char* _names[NumberOfExternalRegisters] = {
        "Xamps Firmware Version",
        "Temp_Ana0",
        "Temp_Ana1",
        "Temp_Ana2",
        "Temp_Ana3",
        "Temp_Ana4",
        "Temp_Dig0",
        "Temp_Dig1",
        "Temp_Dig2",
        "Thermmistor_A",
        "Thermmistor_B",
        "Thermmistor_C",
        "HVV_ReadBack",
        "HVI_ReadBack",
        "Sw_Lvl_Ch0",
        "Sw_Lvl_Ch1",
        "Sw_Lvl_Ch2",
        "Sw_Lvl_Ch3",
        "Sw_Lvl_Ch4"
    };

    void XampsExternalRegisters::print() {
      printf("XampsExternalRegisters:\n");
      for (unsigned i=0; i<NumberOfExternalRegisters; i++) {
        printf("\t%s(0x%03x)", _names[i], values[i] & _xr[i].mask);
        if (!strncmp("Temp_", _names[i], 5)) {
          unsigned raw = values[i] & _xr[i].mask;
          if (raw & 0x100) raw |= 0xfffffe00;
          int temp = (int) raw;
          printf(" %5.1f degrees", temp/2.0);
        }
        if (!strncmp("Thermmistor", _names[i], 11)) printf(" %8.3f Volts", (3.0*(values[i] & _xr[i].mask)/4096.0*100.0));
        if (!strncmp("HVV_ReadBack", _names[i], 12)) printf(" %8.3f Volts", (3.0*(values[i] & _xr[i].mask)/4096.0*200.0/2.048));
        if (!strncmp("HVI_ReadBack", _names[i], 12)) printf(" %8.6f microAmps", (3.0*(values[i] & _xr[i].mask)/409.6));
        printf("\n");
      }
    }

    int XampsExternalRegisters::read() {
      int ret = 0;
      XampsDestination d;
      unsigned i;
      d.dest(XampsDestination::External);
      for (i=0; i<NumberOfExternalRegisters && ret==0; i++) {
        ret = pgp->readRegister(
            &d,
            _xr[i].addr,
            0x999,
            values + i);
      }
      if (ret != 0) {
        printf("\tXamps External Registers encountered error while reading! %u\n", i);
      }
      return ret;
    }

    unsigned XampsExternalRegisters::readVersion(uint32_t* retp) {
      XampsDestination d;
      d.dest(XampsDestination::External);
      unsigned ret = pgp->readRegister(
          &d,
          _xr[0].addr,
          0x999,
          retp);
      return(ret);
    }
  }
}
