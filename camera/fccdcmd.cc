//! fccdcmd.cc
//! This tool allows you to send and receive data through
//! the camera link serial port of a Leutron frame grabber
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!
//
//  An example looks like this:
//
// build/pds/bin/i386-linux-opt/serialcmd --camera Pulnix_TM6740CL_8bit --grabber "PicPortX CL Mono" --baudrate 9600
// build/pds/bin/i386-linux-opt/serialcmd --camera Adimec_Opal-1000m/Q_F8bit --grabber "PicPortX CL Mono PMC" --baudrate 115200
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dsylib.h>
#include <grabber.h>


#define DEFAULT_CAMERA      "LBNL_FCCD"
//#define DEFAULT_CAMERA      "LBNL_FCCD_4x8"
//#define DEFAULT_CAMERA      "Adimec_Opal-1000m/Q_F8bit"
#define DEFAULT_GRABBER     "PicPortExpress CL Stereo"
//#define DEFAULT_GRABBER     "PicPortX CL Mono PMC"
//#define DEFAULT_CAMERA      "Pulnix_TM6740CL_8bit"
//#define DEFAULT_GRABBER     "PicPortX CL Mono"
#define DEFAULT_BAUDRATE    115200
#define DEFAULT_PARITY      'n'
#define DEFAULT_DATASIZE    8
#define DEFAULT_STOPSIZE    1

#define SERIALPORT_ID       0
#define RECEIVEBUFFER_SIZE  256
#define RECEIVE_TIMEOUT_MS  10000

void help(char *prog_name)
{
    printf("%s: send/receive data over a camera link serial port.\n" \
            "Syntax: %s <command> \\\n" \
            "\t[ --grabber <grabber> ] [ --camera <camera> ] \\\n" \
            "\t[ --baudrate <baudrate> ] [ --parity <parity> ] \\\n" \
            "\t[ --data <databits> ] [ --stop <stopbits> ] [ --eot ]\\\n" \
            "\t| [ --help ]\n" \
            "<command>: string to send over the serial port.\n" \
            "--grabber <grabber>: name of the frame grabber to use.\n" \
            "--camera <camera>: name of the camera connected to the frame\n" \
            "\tgrabber.\n" \
            "--baudrate <baudrate>: baud rate of the serial connection.\n" \
            "--parity <parity>: parity of the serial connection ('n'=none,\n" \
            "\t'o'=odd, 'e'=even).\n" \
            "--data <databits>: number of bits per character.\n" \
            "--stop <stopabits>: number of stop bits.\n" \
            "--eot <char>: character to use as end of transmission.\n" \
            "\n", prog_name, prog_name);
}

int display_serialcmd(char *cmd, int len)
{   int i;
    for(i=0; i<len; i++) {
        char *str;
        switch (cmd[i]) {
        case 0: str="\\0"; break;
        case '\r': str="<CR>"; break;
        case 0x02: str="<STX>"; break;
        case 0x03: str="<ETX>"; break;
        case 0x06: str="<ACK>"; break;
        case 0x15: str="<NAK>"; break;
        default: 
            char str_data[2];
            str=str_data;
            str[0]=isprint(cmd[i]) ? cmd[i] : '?';
            str[1]=0;
            break;
        };
        printf(str);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    char *szGrabber = DEFAULT_GRABBER;
    char *szCamera = DEFAULT_CAMERA;
    int i;
    STATUS ret;
    LvGrabberNode *lvpGrabber;
    LvCameraNode *lvpCamera;
    LvCameraInfo lvciCamera;
    HCAMERA hCamera = HANDLE_INVALID;
    int iBaudrate = DEFAULT_BAUDRATE;
    char cParity = DEFAULT_PARITY;
    unsigned long lvParity;
    char bDataSize = DEFAULT_DATASIZE;
    unsigned long lvByteSize;    
    char bStopSize = DEFAULT_STOPSIZE;
    unsigned long lvStopSize;
    char pReceiveBuffer[RECEIVEBUFFER_SIZE];
    unsigned long ulReceiveBufferSize = RECEIVEBUFFER_SIZE;
    U32BIT ulReceivedLength = 0;
    LvConnectionInfo *lvpciConnections;

    // Parse arguments
    for(i=1; i<argc; i++) {
        if(strcmp(argv[i], "--help") == 0) {
            help(argv[0]);
            return 0;
        } else if(strcmp(argv[i], "--grabber") == 0) {
            szGrabber = argv[++i];
        } else if(strcmp(argv[i], "--camera") == 0) {
            szCamera = argv[++i];
        } else if(strcmp(argv[i], "--baudrate") == 0) {
            iBaudrate = atol(argv[++i]);
        } else if(strcmp(argv[i], "--parity") == 0) {
            cParity = argv[++i][0];
        } else if(strcmp(argv[i], "--data") == 0) {
            bDataSize = atoi(argv[++i]);
        } else if(strcmp(argv[i], "--stop") == 0) {
            bStopSize = atoi(argv[++i]);
        } else {
            printf("ERROR: invalid argument %s, try --help.\n\n", argv[i]);
            return 1;
        }
    }

    switch(cParity) {
    case 'n': 
    case 'N': 
        lvParity = LvComm_ParityNone; break;
    case 'e': 
    case 'E': 
        lvParity = LvComm_ParityEven; break;
    case 'o': 
    case 'O': 
        lvParity = LvComm_ParityOdd; break;
    default:
        printf("ERROR: invalid parity '%c', try --help.\n\n",cParity);
        return 1;
    }
    switch(bDataSize) {
    case 7: 
        lvByteSize = LvComm_Data7; break;
    case 8: 
        lvByteSize = LvComm_Data8; break;
    default:
        printf("ERROR: invalid data size %d, try --help.\n\n",bDataSize);
        return 1;
    }
    switch(bStopSize) {
    case 1: 
        lvStopSize = LvComm_Stop1; break;
    case 2: 
        lvStopSize = LvComm_Stop2; break;
    default:
        printf("ERROR: invalid stop size %d, try --help.\n\n",bStopSize);
        return 1;
    }

    printf("call DsyInit\n");

    DsyInit();

    printf("find the grabber\n");

    // Find the grabber
    for(i=0; (lvpGrabber = DsyGetGrabber(i)) != NULL; i++) {
        printf("found grabber %s\n",lvpGrabber->GetName());
        if (stricmp(szGrabber, lvpGrabber->GetName()) == 0)
            break;
    }
    if (lvpGrabber == NULL) {
        printf("ERROR: no grabber named %s found.\n\n", szGrabber);
        DsyClose();
        return 2;    
    }

    printf("find the camera\n");

    // Find the camera
    for(i=0; (ret = DsyEnumCameraType(i, &lvciCamera))==DSY_I_NoError; i++) {
	printf("found camera (%d) %s\n",i,lvciCamera.Name);
        if (stricmp(szCamera, lvciCamera.Name) == 0)
            break;
    }
    if (ret != DSY_I_NoError) {
        printf("ERROR: no camera named %s found.\n\n", szCamera);
        DsyClose();
        return 3;    
    }

    printf("connect and create\n");

    // Connect them and create the camera object
    i = lvpGrabber->GetNrFreeConnectorEx(lvciCamera.Id);
    if (i == 0) {
        printf("ERROR: no connector found that can connect %s to %s.\n\n", 
            szCamera, szGrabber);
        DsyClose();
        return 4;
    }
    lvpciConnections = new LvConnectionInfo[i];
    lvpGrabber->GetFreeConnectorInfoEx(lvciCamera.Id, lvpciConnections);
    hCamera = lvpGrabber->ConnectCamera(lvciCamera.Id, 
                                    lvpciConnections[0].hConn,
                                    lvpciConnections[0].SyncNr);
    if (hCamera == HANDLE_INVALID) {
        printf("ERROR: failed to connect %s to %s.\n\n", 
            szCamera, szGrabber);
        delete lvpciConnections;
        DsyClose();
        return 5;
    }
    lvpCamera = lvpGrabber->GetCameraPtr(hCamera);

    printf("activate camera\n");

    // Activate the Camera
    ret = lvpCamera->Activate();
    if (ret != DSY_I_NoError) {
        printf("ERROR: failed to activate camera %s.\n\n", szCamera);
        lvpGrabber->DisconnectCamera(hCamera);
        delete lvpciConnections;
        DsyClose();
        return 6;
    }

    // Open the communication channel
    ret = lvpCamera->CommOpen();
    if (ret != DSY_I_NoError) {
        printf("ERROR: failed to open communication channel to camera %s" \
                    ", error is %d.\n\n", szCamera, ret);
        lvpGrabber->DeactivateCamera(hCamera);
        lvpGrabber->DisconnectCamera(hCamera);
        delete lvpciConnections;
        DsyClose();
        return 7;
    }
    ret = lvpCamera->CommSetParam(iBaudrate, lvParity, lvByteSize, lvStopSize);
    if (ret != DSY_I_NoError) {
        printf("ERROR: failed to change communication parameters to " \
                "%d%c%d,%d.\n\n", iBaudrate, cParity, bDataSize, bStopSize);
        lvpGrabber->DeactivateCamera(hCamera);
        lvpGrabber->DisconnectCamera(hCamera);
        delete lvpciConnections;
        DsyClose();
        return 8;
    }

    char szCommand[32];
    do {
      int lenresult;
      const int maxlen=128;
      char line[maxlen];
      printf("Enter command (ping, outN, expose): ");
      char* result = fgets(line, maxlen, stdin);
      unsigned long cEOT = '\003';    // FCCD
      int argtmp = -1;

      if (!result) break;

      lenresult = strlen(result) - 1;
      result[lenresult] = '\0';   // remove \r

      if (strcmp(result, "ping") == 0) {
        result[0] = 0xff;
        lenresult = 1;
      } else if (strcmp(result, "railson") == 0) {
        result[0] = 0x04;
        result[1] = 0x01;
        lenresult = 2;
      } else if (strcmp(result, "railsoff") == 0) {
        result[0] = 0x04;
        result[1] = 0x00;
        lenresult = 2;
      } else if (strcmp(result, "expose") == 0) {
        result[0] = 0x07;
        result[1] = 0x01; // generate an exposure pulse
        lenresult = 2;
      } else if (strcmp(result, "count") == 0) {
        // number of exposures to take after a trigger
        result[0] = 0x0e;
        result[1] = 0x00; // MS Byte
        result[2] = 0x01; // LS Byte
        lenresult = 3;
      } else if (strncmp(result, "line", 4) == 0) {
        switch (result[4]) {
        case '1':
          result[0] = 0x08;
          result[1] = 0x00;
          result[2] = 0x00;
          result[3] = 0x10;
          lenresult = 4;
          break;
        case '2':
          result[0] = 0x09;
          result[1] = 0x00;
          result[2] = 0x02;
          result[3] = 0x00;
          lenresult = 4;
          break;
        case '3':
          result[0] = 0x13;
          result[1] = 0x00;
          lenresult = 2;
          break;
        case '4':
          result[0] = 0x0e;
          result[1] = 0x00;
          result[2] = 0x01;
          lenresult = 3;
          break;
        case '5':
          // set exposure delay to zero (should be default)
          result[0] = 0x0f;
          result[1] = 0x00;
          result[2] = 0x00;
          result[3] = 0x00;
          lenresult = 4;
          break;
        case '6': default:
          // generate a soft exposure
          result[0] = 0x07;
          result[1] = 0x01;
          lenresult = 2;
          break;
        }
      } else if (strncmp(result, "out", 3) == 0) {
        result[0] = 0x03;
        lenresult = 2;
        switch (result[3]) {
        case '1':
          result[1] = 0x01; break;
        case '2':
          result[1] = 0x02; break;
        case '3':
          result[1] = 0x03; break;
        case '4':
          result[1] = 0x04; break;
        case '0': default:
          result[1] = 0x00; break;
        }
      } else if (strncmp(result, "dacread", 7) == 0) {
        if ((sscanf(result + 7, "%d", &argtmp) == 1) &&
            (argtmp >= 1) && (argtmp <= 17)) {
          result[0] = 0x06;
          result[1] = (argtmp - 1) * 2;
          lenresult = 2;
        } else {
          printf("Usage: dacreadN, N=1-17\n");
          continue;
        }
      } else if (strncmp(result, "dacwrite", 8) == 0) {
        if ((sscanf(result + 8, "%d", &argtmp) == 1) &&
            (argtmp >= 1) && (argtmp <= 17)) {
          result[0] = 0x05;
          result[1] = (argtmp - 1) * 2;
          result[2] = 0x08;
          result[3] = 0x00;
          lenresult = 4;
        } else {
          printf("Usage: dacwriteN, N=1-17\n");
          continue;
        }
      } else if (strncmp(result, "sramread", 8) == 0) {
        if ((sscanf(result + 8, "%d", &argtmp) == 1) &&
            (argtmp >= 0) && (argtmp <= 31)) {
          result[0] = 0x02;                         // SRAM Memory Read
          // adrs
          result[1] = result[2] = result[3] = 0x0;
          result[2] = 0x02;
          result[4] = argtmp;
          lenresult = 5;
        } else {
          printf("Usage: sramreadN, N=0-31\n");
          continue;
        }
      } else if (strncmp(result, "dmsramread", 10) == 0) {
        if ((sscanf(result + 10, "%d", &argtmp) == 1) &&
            (argtmp >= 1) && (argtmp <= 2)) {
          result[0] = (argtmp == 1) ? 0x10 : 0x11;  // DM1 or DM2
          result[1] = 0x02;                         // SRAM Memory Read
          result[2] = result[3] = result[4] = result[5] = 0x0;  // adrs
          lenresult = 6;
        } else {
          printf("Usage: dmsramreadN, N=1-2\n");
          continue;
        }
      } else if (strncmp(result, "sramset", 7) == 0) {
        if ((sscanf(result + 7, "%d", &argtmp) == 1) &&
            (argtmp >= 1) && (argtmp <= 2)) {
          result[0] = (argtmp == 1) ? 0x10 : 0x11;  // DM1 or DM2
          result[1] = 0x01;                         // SRAM Memory Write
          result[2] = result[3] = result[4] = result[5] = 0x0;  // adrs
          result[6] = 0x55; // data
          result[7] = 0xaa; // data
          lenresult = 8;
        } else {
          printf("Usage: sramsetN, N=1-2\n");
          continue;
        }
      } else if (strncmp(result, "dmsramclear", 11) == 0) {
        if ((sscanf(result + 11, "%d", &argtmp) == 1) &&
            (argtmp >= 1) && (argtmp <= 2)) {
          result[0] = (argtmp == 1) ? 0x10 : 0x11;  // DM1 or DM2
          result[1] = 0x01;                         // SRAM Memory Write
          result[2] = result[3] = result[4] = result[5] = 0x0;  // adrs
          result[6] = result[7] = 0x00; // data
          lenresult = 8;
        } else {
          printf("Usage: dmsramclearN, N=1-2\n");
          continue;
        }
      } else if (strncmp(result, "--", 2) == 0) {
          // skip over "--" comment
          printf("%s\n", result);
          continue;
      } else if (strncmp(result, "0x", 2) == 0) {
          // parse hex commands (0xMM 0xNN ...)
          int ix, ibuf;
          for (ix = 0; strlen(result + ix*5) >= 4; ix++) {
            if (sscanf(result + 2 + ix*5, "%02x", &ibuf) != 1) {
              ix = 0;
              break;
            }
            result[ix] = (char)ibuf;
          }
          if (0 == ix) {
            printf("ERROR: malformed hex input (0xMM 0xNN ...)\n");
            continue;
          }
          lenresult = ix;
      } else if (strncmp(result, "sramclear", 9) == 0) {
        if ((sscanf(result + 9, "%d", &argtmp) == 1) &&
            (argtmp >= 0) && (argtmp <= 31)) {
          result[0] = 0x01;                         // SRAM Memory Write
          // adrs
          result[1] = result[2] = result[3] = 0x0;
          result[2] = 0x02;
          result[4] = argtmp;
          result[5] = result[6] = 0x00; // data
          lenresult = 7;
        } else {
          printf("Usage: sramclearN, N=0-31\n");
          continue;
        }
      } else {
        printf("Unknown command\n");
        continue;
      }

      for(int k=0; k<lenresult; k++) {
        if (k > 0) {
          printf(":");
        }
        printf("%02x", 0xffu & result[k]);
      }
      printf("\n");

      // Send command
      ret = lvpCamera->CommSend(result, lenresult, pReceiveBuffer,
				ulReceiveBufferSize, RECEIVE_TIMEOUT_MS, cEOT, 
				(U32BIT *)&ulReceivedLength);
      if (ret != DSY_I_NoError) {
        printf("ERROR: failed to send command ");
        display_serialcmd(szCommand, strlen(szCommand));
        printf(" to camera, error %d.\n", ret);
        if(ulReceivedLength != 0) {
	  printf("Received: ");
	  display_serialcmd(pReceiveBuffer, ulReceivedLength);
	  printf("\n");
        }
//         printf("\n");
//         lvpGrabber->DeactivateCamera(hCamera);
//         lvpGrabber->DisconnectCamera(hCamera);
//         delete lvpciConnections;
//         DsyClose();
//         return 10;
      }
      display_serialcmd(pReceiveBuffer, ulReceivedLength);
      printf("\n\n");
      
    } while(1);
        
    // Close everything
    lvpCamera->CommClose();
    lvpGrabber->DeactivateCamera(hCamera);
    lvpGrabber->DisconnectCamera(hCamera);
    delete lvpciConnections;
    DsyClose();
    return 0;
}
