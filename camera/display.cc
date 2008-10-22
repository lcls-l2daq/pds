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

class DisplayImage: public QWidget
{	//Q_OBJECT

	public:
		DisplayImage(QWidget *parent = 0);
		~DisplayImage();
		int display(char *image, unsigned long size, unsigned int width, unsigned int height);

	protected:
		void paintEvent(QPaintEvent *event);

	private:
		QImage *images[2];
		int image_display;
		QVector<QRgb> *gray8_color_table;
		unsigned int image_width;
		unsigned int image_height;
		pthread_mutex_t lock;
};

DisplayImage::DisplayImage(QWidget *parent): QWidget(parent)
{	int i, ret;

	ret = pthread_mutex_init(&lock, NULL);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to initialize mutex: %s.\n", strerror(ret));
	}
	setWindowTitle("Video");
	image_display = 0;
	image_width = 0;
	image_height = 0;
	gray8_color_table = new QVector<QRgb>(256);
	for (i = 0; i < 256; i++) {
// should be 		gray8_color_table[i]=qRgb(i,i,i);
		gray8_color_table->insert(i, qRgb(i,i,i));
	}
	for(i = 0; i < NVIDEO_BUFFERS; i++) {
		images[i] = new QImage(DEFAULT_WIDTH, DEFAULT_HEIGHT, QImage::Format_Indexed8);
		images[i]->setColorTable(*gray8_color_table);
		images[i]->fill(128);
	}
}

DisplayImage::~DisplayImage()
{	int i;
	for(i = 0; i < NVIDEO_BUFFERS; i++)
		delete images[i];
	delete gray8_color_table;
}

void DisplayImage::paintEvent(QPaintEvent *evt)
{
	QPainter painter(this);
	pthread_mutex_lock(&lock);
	painter.drawImage(QPoint(), *images[image_display]);
	pthread_mutex_unlock(&lock);
}

int DisplayImage::display(char *data, unsigned long size, unsigned int width, unsigned int height)
{	int image_next = (image_display + 1) % NVIDEO_BUFFERS;
	if (size != (unsigned long)width*height) {
		fprintf(stderr, "ERROR: only format support is 8bits gray images for now.\n");
		return -ENOTSUP;
	}
	if((image_width != width) || (image_height != height)) {
		int i;
		pthread_mutex_lock(&lock);
		for(i = 0; i < NVIDEO_BUFFERS; i++) {
			delete images[i];
			images[i] = new QImage(width, height, QImage::Format_Indexed8);
			images[i]->setColorTable(*gray8_color_table);
		}
		image_width = width;
		image_height = height;
		pthread_mutex_unlock(&lock);
	}
	memcpy(images[image_next]->bits(), data, size);
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

int display_init(void)
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
	return 0;
}

int display_image(char *image, unsigned long size, unsigned int width, unsigned int height)
{
	return video_screen->display(image, size, width, height);
}

