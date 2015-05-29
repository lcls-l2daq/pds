/*
 * Get one gray image using libdc1394 and store it as portable gray map
 *    (pgm). Based on 'samplegrab' from Chris Urmson
 *
 * Written by Damien Douxchamps <ddouxchamps@users.sf.net>
 *
 * Modified extensively by Jack Pines
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

#define IMAGE_FILE_NAME "image.pgm"
#define NumberOfFrameBuffers 100
#define HistoSize  100000
#define BehindHistoSize 4

    unsigned             count =0;
    unsigned             secs = 0;
    FILE*                imagefile;
    dc1394video_frame_t* frame;
    dc1394camera_t*      camera;
    dc1394video_mode_t   video_mode = 0;
    unsigned int         width, height;
    uint16_t             frames[NumberOfFrameBuffers][640*480];
    unsigned             frameIndex = 0;
    unsigned             fileIndex = 0;
    unsigned             imageDataBits = 0;
    unsigned*            periodHisto;
    unsigned*            behindHisto;
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
    printf("\nCount %u, Seconds %u\n", count, secs);
    exit(1);
}

void cleanup_and_exit_with_signal(dc1394camera_t *camera, int sig)
{
  unsigned i;
  dc1394_video_set_transmission(camera, DC1394_OFF);
  dc1394_capture_stop(camera);
  dc1394_camera_free(camera);
  printf("\nCount %u, Seconds %u\n", count, secs);
  printf("Frames behind histo:\n");
  for(i=0; i<BehindHistoSize; i++) {
    if (behindHisto[i]) printf("\t%u %7u\n", i, behindHisto[i]);
  }
  printf("Milliseconds between frame arrivals:\n");
  for(i=0; i<HistoSize; i++) {
    if (periodHisto[i]) printf("\t%8.3f %7u\n", i/100.0, periodHisto[i]);
  }
  exit(sig);
}

void sigHandler( int signal ) {
  psignal( signal, "Signal received by grab_gray_image");
  cleanup_and_exit_with_signal(camera, signal);
}

uint64_t lastTime = 0;

void printFrameStats ( dc1394video_frame_t *f) {
  printf("Frame %7u, %u x %u, levels(%u) bytes(%u) padding(%u) pktsPerFrame(%u) timestamp(%lu) framesBehind(%u) littleEndian(%s)\n",
      count, f->size[0], f->size[1], 1<<f->data_depth, f->image_bytes, f->padding_bytes, f->packets_per_frame,
      lastTime? f->timestamp-lastTime : f->timestamp,
      f->frames_behind, f->little_endian ? "true" : "false");
}

void writePgm( char* name, unsigned bits, unsigned char* image) {
  char s[200];
  sprintf(s, "%s%03d.pgm", name, fileIndex++);
  imagefile=fopen(s, "wb");

  if( imagefile == NULL) {
      char buff[200];
      sprintf(buff, "Can't create %s ", s);
      perror( buff);
  }

  dc1394_get_image_size_from_video_mode(camera, video_mode, &width, &height);
  fprintf(imagefile,"P5\n%u %u %u\n", width, height, (1<<bits)-1);
  fwrite(image, 1, height*width*((bits+7)>>3), imagefile);
  fclose(imagefile);
  printf("\twrote: %s %u x %u, %u bytes per pixel\n", s, width, height, (bits+7)/8);

}

int main(int argc, char *argv[])
{
    int i;
    dc1394framerates_t framerates;
    dc1394video_modes_t video_modes;
    dc1394framerate_t framerate = 0;
    dc1394color_coding_t coding;
    dc1394_t * d;
    dc1394camera_list_t * list;

    dc1394error_t err;

    periodHisto = (unsigned*) calloc(sizeof(unsigned), HistoSize);
    behindHisto = (unsigned*) calloc(sizeof(unsigned), BehindHistoSize);

    signal( SIGINT, sigHandler );
    d = dc1394_new ();
    if (!d)
        return 1;
    err=dc1394_camera_enumerate (d, &list);
    DC1394_ERR_RTN(err,"Failed to enumerate cameras");

    if (list->num == 0) {
        dc1394_log_error("No cameras found");
        return 1;
    }

    camera = dc1394_camera_new (d, list->ids[0].guid);
    if (!camera) {
        dc1394_log_error("Failed to initialize camera with guid %"PRIx64, list->ids[0].guid);
        return 1;
    }
    dc1394_camera_free_list (list);

    printf("Using camera with GUID %"PRIx64", model %s, by %s\n", camera->guid, camera->model, camera->vendor);

    /*-----------------------------------------------------------------------
     *  get the best video mode and highest framerate. This can be skipped
     *  if you already know which mode/framerate you want...
     *-----------------------------------------------------------------------*/
    // get video modes:
    err=dc1394_video_get_supported_modes(camera,&video_modes);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Can't get video modes");

    // select the color coding mode we want:
    printf("Found %u video modes:", video_modes.num);
    for (i=video_modes.num-1;i>=0;i--) {
        printf("\n\t%s", videoModenames[video_modes.modes[i]-DC1394_VIDEO_MODE_MIN]);
        if (!dc1394_is_video_mode_scalable(video_modes.modes[i])) {
            printf(" is not scalable");
            dc1394_get_color_coding_from_video_mode(camera,video_modes.modes[i], &coding);
            if (coding==colorCodingWeWant) {
                printf(", Set color coding to %s", codingNames[coding-DC1394_COLOR_CODING_MIN]);
                video_mode=video_modes.modes[i];
            }
        } else printf(" is scalable");
    }
    if (coding == 0) {
        dc1394_log_error("\nCould not find %s mode", colorCodingWeWant);
        cleanup_and_exit(camera);
    }
    printf("\n");

    // get framerate
    err=dc1394_video_get_supported_framerates(camera,video_mode,&framerates);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not get framerates");
    printf("Found %u Internal frame rates supported:", framerates.num);
    for (i=0; i<framerates.num; i++) {
      printf("\n\t%s", frameRateNames[framerates.framerates[i]-DC1394_FRAMERATE_MIN]);
      if (framerates.framerates[i] == DC1394_FRAMERATE_30) {
        printf(" <==Setting this one");
        framerate = framerates.framerates[i];
      }
    }
    printf("\n");

    err=dc1394_get_color_coding_from_video_mode(camera, video_mode,&coding);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not get color coding");


    /*-----------------------------------------------------------------------
     *  setup capture
     *-----------------------------------------------------------------------*/

    err=dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set iso speed");

    err=dc1394_video_set_mode(camera, video_mode);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set video mode");

    err=dc1394_video_set_framerate(camera, framerate);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set framerate");

    err=dc1394_feature_set_power(camera, DC1394_FEATURE_EXPOSURE, DC1394_OFF);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could turn off DC1394_FEATURE_EXPOSURE");

    err=dc1394_feature_set_power(camera, DC1394_FEATURE_GAMMA, DC1394_OFF);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could turn off DC1394_FEATURE_GAMMA");

    err=dc1394_feature_set_power(camera, DC1394_FEATURE_TRIGGER_DELAY, DC1394_OFF);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could turn off DC1394_FEATURE_TRIGGER_DELAY");

    err=dc1394_external_trigger_set_mode(camera, DC1394_TRIGGER_MODE_1);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set DC1394_TRIGGER_MODE_1");

    err=dc1394_external_trigger_set_polarity(camera, DC1394_TRIGGER_ACTIVE_LOW);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set DC1394_TRIGGER_ACTIVE_LOW");

    err=dc1394_external_trigger_set_power(camera, DC1394_ON);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set trigger to DC1394_ON");

    err=dc1394_feature_set_value(camera, DC1394_FEATURE_BRIGHTNESS, 533);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set DC1394_FEATURE_BRIGHTNESS to 533");

    err=dc1394_feature_set_value(camera, DC1394_FEATURE_GAIN, 53);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set DC1394_FEATURE_GAIN to 53");

    err=dc1394_feature_set_value(camera, DC1394_FEATURE_SHARPNESS, 508);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set DC1394_FEATURE_SHARPNESS to 508");

    err=dc1394_capture_setup(camera,10, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not setup camera-\nmake sure that the video mode and framerate are\nsupported by your camera");


    /*-----------------------------------------------------------------------
     *  have the camera start sending us data
     *-----------------------------------------------------------------------*/
    err=dc1394_video_set_transmission(camera, DC1394_ON);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not start camera iso transmission");

    /*-----------------------------------------------------------------------
     *  capture one frame
     *-----------------------------------------------------------------------*/
    printf("\n");
    while (1) {
      //      printf("\ncamera %s, count %u", camera->model, count);

      err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
      DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not capture a frame");
      //    printf("\ncamera %s, count %u", camera->model, count);
      if (imageDataBits == 0) {
              imageDataBits = frame->data_depth;
//        imageDataBits = 16;
        printf("Initializing: camera %s, count %u, bits %u\n", camera->model, count, imageDataBits);
      }
      dc1394_get_image_size_from_video_mode(camera, video_mode, &width, &height);
      if ((secs%1000)==0 && (count%10)==0) {
        writePgm(camera->model, imageDataBits, frame->image);
      }
      dc1394_capture_enqueue(camera, frame);
      count += 1;
      if ((count%10)==0) {
        secs += 1;
        if ((secs%100)==0) {
          printf("Seconds %6u, ", secs);
          printFrameStats(frame);
          //          writePgm(camera->model, imageDataBits, frame->image);
          //          char buf[200];
          //          sprintf(buf, "%s_LittleEndian", camera->model);
          //          writePgm(buf, imageDataBits, (unsigned char*)frames[frameIndex]);
        }
      }
      frameIndex += 1;
      frameIndex %= NumberOfFrameBuffers;
      if (lastTime) {
        uint64_t diff = frame->timestamp - lastTime;
        diff /= 10;
        if (diff>(HistoSize-1)) diff = HistoSize - 1;
        periodHisto[diff] += 1;
      }
      lastTime = frame->timestamp;
      unsigned b = frame->frames_behind < BehindHistoSize ? frame->frames_behind : BehindHistoSize;
      behindHisto[b]+=1;
      swab(frame->image, frames[frameIndex], height*width*((frame->data_depth+7)/16));
    }

    /*-----------------------------------------------------------------------
     *  stop data transmission
     *-----------------------------------------------------------------------*/
    err=dc1394_video_set_transmission(camera,DC1394_OFF);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not stop the camera");

    /*-----------------------------------------------------------------------
     *  stop data transmission and exit
     *-----------------------------------------------------------------------*/
    cleanup_and_exit(camera);

    return 0;
}
