/*
 * PhasicsConfigurator.cc
 *
 *  Created on: September 23, 2011
 *      Author: jackp
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pds/phasics/PhasicsServer.hh"
#include "pds/phasics/PhasicsConfigurator.hh"
#include "pds/config/PhasicsConfigType.hh"

using namespace Pds::Phasics;

PhasicsConfigurator::PhasicsConfigurator() : _rhisto(0) {
  printf("PhasicsConfigurator constructor\n");
}

PhasicsConfigurator::~PhasicsConfigurator() {}

static char    videoModenames[DC1394_VIDEO_MODE_NUM][120] = {
      {"DC1394_VIDEO_MODE_160x120_YUV444"},
      {"DC1394_VIDEO_MODE_320x240_YUV422"},
      {"DC1394_VIDEO_MODE_640x480_YUV411"},
      {"DC1394_VIDEO_MODE_640x480_YUV422"},
      {"DC1394_VIDEO_MODE_640x480_RGB8"},
      {"DC1394_VIDEO_MODE_640x480_MONO8"},
      {"DC1394_VIDEO_MODE_640x480_MONO16"},
      {"DC1394_VIDEO_MODE_800x600_YUV422"},
      {"DC1394_VIDEO_MODE_800x600_RGB8"},
      {"DC1394_VIDEO_MODE_800x600_MONO8"},
      {"DC1394_VIDEO_MODE_1024x768_YUV422"},
      {"DC1394_VIDEO_MODE_1024x768_RGB8"},
      {"DC1394_VIDEO_MODE_1024x768_MONO8"},
      {"DC1394_VIDEO_MODE_800x600_MONO16"},
      {"DC1394_VIDEO_MODE_1024x768_MONO16"},
      {"DC1394_VIDEO_MODE_1280x960_YUV422"},
      {"DC1394_VIDEO_MODE_1280x960_RGB8"},
      {"DC1394_VIDEO_MODE_1280x960_MONO8"},
      {"DC1394_VIDEO_MODE_1600x1200_YUV422"},
      {"DC1394_VIDEO_MODE_1600x1200_RGB8"},
      {"DC1394_VIDEO_MODE_1600x1200_MONO8"},
      {"DC1394_VIDEO_MODE_1280x960_MONO16"},
      {"DC1394_VIDEO_MODE_1600x1200_MONO16"},
      {"DC1394_VIDEO_MODE_EXIF"},
      {"DC1394_VIDEO_MODE_FORMAT7_0"},
      {"DC1394_VIDEO_MODE_FORMAT7_1"},
      {"DC1394_VIDEO_MODE_FORMAT7_2"},
      {"DC1394_VIDEO_MODE_FORMAT7_3"},
      {"DC1394_VIDEO_MODE_FORMAT7_4"},
      {"DC1394_VIDEO_MODE_FORMAT7_5"},
      {"DC1394_VIDEO_MODE_FORMAT7_6"},
      {"DC1394_VIDEO_MODE_FORMAT7_7"}
    };

static char     codingNames[DC1394_COLOR_CODING_NUM][120] = {
      {"DC1394_COLOR_CODING_MONO8"},
      {"DC1394_COLOR_CODING_YUV411"},
      {"DC1394_COLOR_CODING_YUV422"},
      {"DC1394_COLOR_CODING_YUV444"},
      {"DC1394_COLOR_CODING_RGB8"},
      {"DC1394_COLOR_CODING_MONO16"},
      {"DC1394_COLOR_CODING_RGB16"},
      {"DC1394_COLOR_CODING_MONO16S"},
      {"DC1394_COLOR_CODING_RGB16S"},
      {"DC1394_COLOR_CODING_RAW8"},
      {"DC1394_COLOR_CODING_RAW16"}
};

//static char     featureNames[DC1394_FEATURE_NUM][120] = {
//        {"DC1394_FEATURE_BRIGHTNESS"},
//        {"DC1394_FEATURE_EXPOSURE"},
//        {"DC1394_FEATURE_SHARPNESS"},
//        {"DC1394_FEATURE_WHITE_BALANCE"},
//        {"DC1394_FEATURE_HUE"},
//        {"DC1394_FEATURE_SATURATION"},
//        {"DC1394_FEATURE_GAMMA"},
//        {"DC1394_FEATURE_SHUTTER"},
//        {"DC1394_FEATURE_GAIN"},
//        {"DC1394_FEATURE_IRIS"},
//        {"DC1394_FEATURE_FOCUS"},
//        {"DC1394_FEATURE_TEMPERATURE"},
//        {"DC1394_FEATURE_TRIGGER"},
//        {"DC1394_FEATURE_TRIGGER_DELAY"},
//        {"DC1394_FEATURE_WHITE_SHADING"},
//        {"DC1394_FEATURE_FRAME_RATE"},
//        {"DC1394_FEATURE_ZOOM"},
//        {"DC1394_FEATURE_PAN"},
//        {"DC1394_FEATURE_TILT"},
//        {"DC1394_FEATURE_OPTICAL_FILTER"},
//        {"DC1394_FEATURE_CAPTURE_SIZE"},
//        {"DC1394_FEATURE_CAPTURE_QUALITY"}
//};

static char      frameRateNames[][120] = {
      {"DC1394_FRAMERATE_1_875"},
      {"DC1394_FRAMERATE_3_75"},
      {"DC1394_FRAMERATE_7_5"},
      {"DC1394_FRAMERATE_15"},
      {"DC1394_FRAMERATE_30"},
      {"DC1394_FRAMERATE_60"},
      {"DC1394_FRAMERATE_120"},
      {"DC1394_FRAMERATE_240"}
};

long long int PhasicsConfigurator::timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

void PhasicsConfigurator::print() {}

void PhasicsConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

void PhasicsConfigurator::printError(char* m, unsigned* rp) {
  if ((err>0)||(err<=-DC1394_ERROR_NUM))
    err=DC1394_INVALID_ERROR_CODE;
  if (err!=DC1394_SUCCESS) {
    printf("%s: in %s (%s, line %d): %s\n",
        dc1394_error_get_string(err),
        __FUNCTION__, __FILE__, __LINE__, m);
    *rp <<= 1;
    *rp |= 1;
  }
}

unsigned PhasicsConfigurator::configure( PhasicsConfigType* c) {
  _config = c;
  dc1394color_coding_t       colorCodingWeWant = DC1394_COLOR_CODING_MONO16;
  dc1394framerate_t          framerateWeWant = DC1394_FRAMERATE_30;
  dc1394video_mode_t         video_mode = DC1394_VIDEO_MODE_160x120_YUV444;
  dc1394framerates_t         framerates;
  dc1394video_modes_t        video_modes;
  dc1394framerate_t          framerate = DC1394_FRAMERATE_1_875;
  dc1394color_coding_t       coding = DC1394_COLOR_CODING_MONO8;
  dc1394camera_list_t *      list;
  timespec                   start, end;
  printf("Phasics config(%p)\n", _config);
  unsigned ret = 0;
  clock_gettime(CLOCK_REALTIME, &start);

  d = dc1394_new ();
  if (!d) {
    printf("PhasicsConfigurator failed to open library libdc1394!\n");
    return 1;
  }
  err=dc1394_camera_enumerate (d, &list);
  printError("Failed to enumerate cameras", &ret);

  if (list->num == 0) {
    printf("PhasicsConfigurator no cameras found");
    return 1;
  }
  // assume we are using the first camera we found.
  // if there are more than one, we'll have to qualify the one we want
  PhasicsConfigurator::_camera = dc1394_camera_new (d, list->ids[0].guid);
  if (!_camera) {
    printf("Failed to initialize camera with guid %llx\n", (long long unsigned)list->ids[0].guid);
    return 1;
  }
  dc1394_camera_free_list (list);
  printf("Using camera with GUID %llx, model %s, by %s\n", (long long unsigned)_camera->guid, _camera->model, _camera->vendor);
//  err = dc1394_iso_release_channel(_camera, 0);
//  printError("Failed to release  ISO channel", &ret);

  //-----------------------------------------------------------------------
  //  N.B. even though we are using external trigger, because of an idiosyncrasy
  //  in the camera, we need to set the frame rate to the highest available frame rate.
  //-----------------------------------------------------------------------
  // first, get video modes:
  err=dc1394_video_get_supported_modes(_camera,&video_modes);
  printError( "Can't get video modes", &ret);

  // select the color coding mode we want:
  printf("Found %u video modes:", video_modes.num);
  bool found = false;
  for (int i=video_modes.num-1; i>=0 && !found; i--) {
    if (!dc1394_is_video_mode_scalable(video_modes.modes[i])) {
      dc1394_get_color_coding_from_video_mode(_camera,video_modes.modes[i], &coding);
      if (coding==colorCodingWeWant) {
        found = true;
        printf("\n\t%s", videoModenames[video_modes.modes[i]-DC1394_VIDEO_MODE_MIN]);
        printf(", Set color coding to %s\n", codingNames[coding-DC1394_COLOR_CODING_MIN]);
        video_mode=video_modes.modes[i];
      }
    }
  }
  if (!found) {
    ret <<= 1; ret |= 1;
    printf("\nCould not find %s mode\n", codingNames[colorCodingWeWant-DC1394_COLOR_CODING_MIN]);
  }

  // get framerate
  err=dc1394_video_get_supported_framerates(_camera,video_mode,&framerates);
  printError( "Could not get framerates", &ret);
  printf("Found %u Internal frame rates supported:\n", framerates.num);
  for (unsigned i=0; i<framerates.num; i++) {
    if (framerates.framerates[i] == framerateWeWant) {
      printf("\t%s", frameRateNames[framerates.framerates[i]-DC1394_FRAMERATE_MIN]);
      printf(" <==Setting this one\n");
      framerate = framerates.framerates[i];
    }
  }

  err=dc1394_get_color_coding_from_video_mode(_camera, video_mode,&coding);
  printError( "Could not get color coding", &ret);

  err=dc1394_video_set_iso_speed(_camera, DC1394_ISO_SPEED_400);
  printError( "Could not set iso speed", &ret);

  err=dc1394_video_set_mode(_camera, video_mode);
  printError( "Could not set video mode", &ret);

  err=dc1394_video_set_framerate(_camera, framerate);
  printError( "Could not set framerate", &ret);

  err=dc1394_feature_set_power(_camera, DC1394_FEATURE_EXPOSURE, DC1394_ON);
  printError( "Could turn off DC1394_FEATURE_EXPOSURE", &ret);

  err=dc1394_feature_set_power(_camera, DC1394_FEATURE_GAMMA, DC1394_ON);
  printError( "Could turn off DC1394_FEATURE_GAMMA", &ret);

  err=dc1394_feature_set_power(_camera, DC1394_FEATURE_TRIGGER_DELAY, DC1394_OFF);
  printError( "Could turn off DC1394_FEATURE_TRIGGER_DELAY", &ret);

  err=dc1394_external_trigger_set_mode(_camera, DC1394_TRIGGER_MODE_1);
  printError( "Could not set DC1394_TRIGGER_MODE_1", &ret);

  err=dc1394_external_trigger_set_polarity(_camera, DC1394_TRIGGER_ACTIVE_HIGH);
  printError( "Could not set DC1394_TRIGGER_ACTIVE_HIGH", &ret);

//  err=dc1394_external_trigger_set_power(_camera, DC1394_ON);
//  printError( "Could not set trigger to DC1394_ON", &ret);

  err=dc1394_feature_set_value(_camera, DC1394_FEATURE_BRIGHTNESS, _config->get(ConfigV1::Brightness));
  printError( "Could not set DC1394_FEATURE_BRIGHTNESS", &ret);

  err=dc1394_feature_set_value(_camera, DC1394_FEATURE_EXPOSURE, _config->get(ConfigV1::Exposure));
  printError( "Could not set DC1394_FEATURE_EXPOSURE", &ret);

  err=dc1394_feature_set_value(_camera, DC1394_FEATURE_SHARPNESS, _config->get(ConfigV1::Sharpness));
  printError( "Could not set DC1394_FEATURE_SHARPNESS", &ret);

  err=dc1394_feature_set_value(_camera, DC1394_FEATURE_GAMMA, _config->get(ConfigV1::Gamma));
  printError( "Could not set DC1394_FEATURE_GAMMA", &ret);

  err=dc1394_feature_set_value(_camera, DC1394_FEATURE_SHUTTER, _config->get(ConfigV1::Shutter));
  printError( "Could not set DC1394_FEATURE_SHUTTER", &ret);

  err=dc1394_feature_set_value(_camera, DC1394_FEATURE_GAIN, _config->get(ConfigV1::Gain));
  printError( "Could not set DC1394_FEATURE_GAIN", &ret);

  err=dc1394_feature_set_value(_camera, DC1394_FEATURE_PAN, _config->get(ConfigV1::Pan));
  printError( "Could not set DC1394_FEATURE_PAN", &ret);

  err=dc1394_feature_set_value(_camera, DC1394_FEATURE_TILT, _config->get(ConfigV1::Tilt));
  printError( "Could not set DC1394_FEATURE_TILT", &ret);

  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  printf("- 0x%x - \n\tdone \n", ret);
  printf(" it took %lld.%lld milliseconds\n", diff/1000000LL, diff%1000000LL);

  dc1394_camera_free(_camera);

  return ret;
}

#undef PRINT_ERROR
