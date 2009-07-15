/**display.cc
 * This file contain the display primitives for
 * camreceiver
 * It is using Qt and because of that is written in C++
 */

#include <stdio.h>
#include <errno.h>
#include <QtCore/QVector>
#include <QtGui/QPixmap>
#include <QtGui/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include "display.h"

static pthread_mutex_t display_lock;
static pthread_cond_t display_cond;

#define DEFAULT_WIDTH	640
#define DEFAULT_HEIGHT	480
#define NVIDEO_BUFFERS	2
#define MAX_BIT_DEPTH   8

class DisplayImage: public QWidget {	//Q_OBJECT
public:
  DisplayImage(QWidget *parent = 0);
  ~DisplayImage();
  int display(char *image, unsigned long size, unsigned int width, unsigned int height);
  void set_color_mode(int bw_mode);
protected:
  void paintEvent(QPaintEvent *event);

private:
  QImage *images[2];
  int image_display;
  QVector<QRgb> *color_table;
  unsigned int image_width;
  unsigned int image_height;
  pthread_mutex_t lock;
  unsigned last_scale;
};

DisplayImage::DisplayImage(QWidget *parent): QWidget(parent)
{	int ret;

	ret = pthread_mutex_init(&lock, NULL);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to initialize mutex: %s.\n", strerror(ret));
	}
	setWindowTitle("Video");
	image_display = 0;
	image_width = 0;
	image_height = 0;
	color_table = new QVector<QRgb>((1<<MAX_BIT_DEPTH));
}

DisplayImage::~DisplayImage()
{	int i;
	for(i = 0; i < NVIDEO_BUFFERS; i++)
		delete images[i];
	delete color_table;
}

void DisplayImage::paintEvent(QPaintEvent *evt)
{
	QPainter painter(this);
	pthread_mutex_lock(&lock);
	painter.drawImage(QPoint(), *images[image_display]);
	pthread_mutex_unlock(&lock);
}

void DisplayImage::set_color_mode(int bw_mode)
{
  if (bw_mode) {
    for (int i = 0; i < (1<<MAX_BIT_DEPTH); i++) {
      unsigned v = i<256 ? i : 255;
      color_table->insert(i, qRgb(v,v,v));
    }
  }
  else {
    for (int i = 0; i < (1<<MAX_BIT_DEPTH); i++) {
      if (i<256)  // black -> blue
	color_table->insert(i, qRgb(0,0,i));
      else if (i<512) // blue -> green
	color_table->insert(i, qRgb(0,i-256,511-i));
      else if (i<768) // green -> red
	color_table->insert(i, qRgb(i-512,i-257,0));
      else if (i<1024) // red -> white
	color_table->insert(i, qRgb(255,i-1024,i-1024));
      else // white
	color_table->insert(i, qRgb(255,255,255));
    }	
  }
  for(int i = 0; i < NVIDEO_BUFFERS; i++) {
    images[i] = new QImage(DEFAULT_WIDTH, DEFAULT_HEIGHT, QImage::Format_Indexed8);
    images[i]->setColorTable(*color_table);
    images[i]->fill(128);
  }
}

int DisplayImage::display(char *data, unsigned long size, unsigned int width, unsigned int height)
{	int image_next = (image_display + 1) % NVIDEO_BUFFERS;
	if((image_width != width) || (image_height != height)) {
		int i;
		pthread_mutex_lock(&lock);
		for(i = 0; i < NVIDEO_BUFFERS; i++) {
			delete images[i];
			images[i] = new QImage(width, height, QImage::Format_Indexed8);
			images[i]->setColorTable(*color_table);
		}
		image_width = width;
		image_height = height;
		pthread_mutex_unlock(&lock);
	}
	// saturate the color range
 	if (size != (unsigned long)width*height) {  // 2 bytes per pixel
	  unsigned short scale=0;
	  unsigned short* d=reinterpret_cast<unsigned short*>(data)+4;
	  unsigned short* const e=reinterpret_cast<unsigned short*>(data)+(size>>1);
// 	  do {
// 	    if (*d > scale) scale = *d;
// 	  } while (++d < e);
	  scale = 1;
	  unsigned char * dst = reinterpret_cast<unsigned char*>(images[image_next]->bits());
	  d = reinterpret_cast<unsigned short*>(data); 
	  while ( d < e )
	    *dst++ = ((*d++)<<8) / scale;
 	}
 	else {  // 1 byte per pixel
	  unsigned char scale=0;
// 	  unsigned char* d=reinterpret_cast<unsigned char*>(data)+4;
// 	  unsigned char* e=reinterpret_cast<unsigned char*>(data)+(size);
// 	  do {
// 	    if (*d > scale) scale = *d;
// 	  } while (++d < e);
	  scale = 1;
	  for (unsigned i = 0; i < 256; i++) {
	    unsigned v = (i<<8)/scale;
	    color_table->insert(i, qRgb(v,v,v));
	  }
	  images[image_next]->setColorTable(*color_table);
	  memcpy(images[image_next]->bits(), data, size);
 	}
	image_display = image_next;
	update();
	return 0;
}

DisplayImage *video_screen;

void *display_main(void *arg)
{	int argc = 1;
	char *argv[] = { "app", NULL };
	QApplication app(argc, argv);

	// Create the display
	video_screen = new DisplayImage();
	video_screen->resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	video_screen->show();
	// Now the program can use the display APIs
	pthread_mutex_lock(&display_lock);
	pthread_cond_signal(&display_cond);
	pthread_mutex_unlock(&display_lock);
	return (void *)app.exec();
}

int display_init(int bw_mode=1)
{	pthread_t qt_thread;
	int ret;

	ret = pthread_mutex_init(&display_lock, NULL);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to initialize mutex: %s.\n", strerror(ret));
		return -ret;
	}
	ret = pthread_cond_init(&display_cond, NULL);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to initialize condition variable: %s.\n", strerror(ret));
		return -ret;
	}
	pthread_mutex_lock(&display_lock);
	ret = pthread_create(&qt_thread, NULL, display_main, NULL);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to start the display thread: %s.\n", strerror(ret));
		return -ret;
	}
	// Qt is very sensitive on using QT before the QApplication has been created
	pthread_cond_wait(&display_cond, &display_lock);
	pthread_mutex_unlock(&display_lock);

	video_screen->set_color_mode(bw_mode);

	return 0;
}

int display_image(char *image, unsigned long size, unsigned int width, unsigned int height)
{
	return video_screen->display(image, size, width, height);
}

