/*
 * Query camera using libdc1394 and print what is found
 *
 * Written by Jack Pines
 *
 */

#define _XOPEN_SOURCE
#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <dc1394/dc1394.h>
#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>
#include <string.h>

    dc1394camera_t*      camera;
    dc1394video_mode_t   video_mode = 0;
    dc1394color_coding_t   colorCodingWeWant = DC1394_COLOR_CODING_MONO16;

char    videoModenames[DC1394_VIDEO_MODE_NUM][120] = {
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

char     codingNames[DC1394_COLOR_CODING_NUM][120] = {
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

char     featureNames[DC1394_FEATURE_NUM][120] = {
        {"DC1394_FEATURE_BRIGHTNESS"},
        {"DC1394_FEATURE_EXPOSURE"},
        {"DC1394_FEATURE_SHARPNESS"},
        {"DC1394_FEATURE_WHITE_BALANCE"},
        {"DC1394_FEATURE_HUE"},
        {"DC1394_FEATURE_SATURATION"},
        {"DC1394_FEATURE_GAMMA"},
        {"DC1394_FEATURE_SHUTTER"},
        {"DC1394_FEATURE_GAIN"},
        {"DC1394_FEATURE_IRIS"},
        {"DC1394_FEATURE_FOCUS"},
        {"DC1394_FEATURE_TEMPERATURE"},
        {"DC1394_FEATURE_TRIGGER"},
        {"DC1394_FEATURE_TRIGGER_DELAY"},
        {"DC1394_FEATURE_WHITE_SHADING"},
        {"DC1394_FEATURE_FRAME_RATE"},
        {"DC1394_FEATURE_ZOOM"},
        {"DC1394_FEATURE_PAN"},
        {"DC1394_FEATURE_TILT"},
        {"DC1394_FEATURE_OPTICAL_FILTER"},
        {"DC1394_FEATURE_CAPTURE_SIZE"},
        {"DC1394_FEATURE_CAPTURE_QUALITY"}
};

char      frameRateNames[][120] = {
      {"DC1394_FRAMERATE_1_875"},
      {"DC1394_FRAMERATE_3_75"},
      {"DC1394_FRAMERATE_7_5"},
      {"DC1394_FRAMERATE_15"},
      {"DC1394_FRAMERATE_30"},
      {"DC1394_FRAMERATE_60"},
      {"DC1394_FRAMERATE_120"},
      {"DC1394_FRAMERATE_240"}
};

/*-----------------------------------------------------------------------
 *  Releases the cameras and exits
 *-----------------------------------------------------------------------*/
void cleanup_and_exit(dc1394camera_t *camera)
{
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_camera_free(camera);
    exit(1);
}

void cleanup_and_exit_with_signal(dc1394camera_t *camera, int sig)
{
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_camera_free(camera);
    exit(sig);
}

void sigHandler( int signal ) {
  psignal( signal, "Signal received by query_camera");
  cleanup_and_exit_with_signal(camera, signal);
}


int main(int argc, char *argv[])
{
    int i;
    unsigned j;
    dc1394featureset_t features;
    dc1394framerates_t framerates;
    dc1394video_modes_t video_modes;
    dc1394color_coding_t coding = 0;
    dc1394_t * device;
    dc1394camera_list_t * list;
    unsigned printAll = 0;

    dc1394error_t err;
    
    if (argc>1) printAll = 1;

    signal( SIGINT, sigHandler );
    device = dc1394_new ();
    if (!device)
        return 1;
    err=dc1394_camera_enumerate (device, &list);
    DC1394_ERR_RTN(err,"Failed to enumerate cameras");

    if (list->num == 0) {
        dc1394_log_error("No cameras found");
        return 1;
    }

    camera = dc1394_camera_new (device, list->ids[0].guid);
    if (!camera) {
        dc1394_log_error("Failed to initialize camera with guid %"PRIx64, list->ids[0].guid);
        return 1;
    }
    dc1394_camera_free_list (list);

    printf("\nFound camera with GUID 0x%"PRIx64", model %s, by %s\n", camera->guid, camera->model, camera->vendor);

    // get video modes:
    err=dc1394_video_get_supported_modes(camera,&video_modes);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Can't get video modes");

    // select highest res mode:
    printf("\nFound %u video modes:", video_modes.num);
    for (i=video_modes.num-1;i>=0;i--) {
        printf("\n\t%s", videoModenames[video_modes.modes[i]-DC1394_VIDEO_MODE_MIN]);
        if (!dc1394_is_video_mode_scalable(video_modes.modes[i])) {
            printf(" is not scalable");
            dc1394_get_color_coding_from_video_mode(camera,video_modes.modes[i], &coding);
            if (coding==colorCodingWeWant) {
                video_mode=video_modes.modes[i];
            }
        } else printf(" is scalable");
    }
    if (coding == 0) {
        dc1394_log_error("\nCould not find %s mode", codingNames[colorCodingWeWant-DC1394_COLOR_CODING_MIN]);
        cleanup_and_exit(camera);
    }
    printf("\n");

    // get framerates
    err=dc1394_video_get_supported_framerates( camera, video_mode, &framerates);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not get framerates");
    printf("\nFound %u Internal frame rates supported:\n", framerates.num);
    for (j=0; j<framerates.num; j++) {
      printf("\t%s\n", frameRateNames[framerates.framerates[j]-DC1394_FRAMERATE_MIN]);
    }
    printf("\n");

    /*-----------------------------------------------------------------------
     *  report camera's features
     *-----------------------------------------------------------------------*/
    err=dc1394_feature_get_all(camera,&features);
    if (err!=DC1394_SUCCESS) {
        dc1394_log_warning("Could not get feature set");
    }
    else {
        if (printAll) dc1394_feature_print_all(&features, stdout);
    }

    if (argc > 2) {
      printf("Feature Switchablility:\n");
      for (j=0; j<DC1394_FEATURE_NUM; j++) {
        unsigned feat = j + DC1394_FEATURE_MIN;
        unsigned val = 0;
        err=dc1394_feature_is_switchable(camera, feat, &val );
        DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not test switchablility!");
        if (val) printf("\t%s - %s\n", featureNames[j], val ? "true" : "false");
      }
    }

    cleanup_and_exit(camera);

    return 0;
}
