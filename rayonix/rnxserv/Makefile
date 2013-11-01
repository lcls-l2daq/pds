RAYONIX_ROOT = /opt/rayonix
CRAYDL_INC = $(RAYONIX_ROOT)/include/craydl
RAYONIX_LIB = $(RAYONIX_ROOT)/lib64
BOOST_INC = $(RAYONIX_ROOT)/include

rnxserv: rnxserv.o helper.o workThread.o triggerThread.o rnxlog.o
	g++ -o rnxserv rnxserv.o helper.o workThread.o triggerThread.o rnxlog.o -lpthread -Wall -rdynamic -L $(RAYONIX_LIB) -lcraydl

rnxserv.o: rnxserv.c helper.h workThread.h triggerThread.h rnxState.h rnxlog.h rnxcommon.h
	g++ -o rnxserv.o rnxserv.c -c -ansi -pedantic -Wall

rnxlog.o: rnxlog.c rnxlog.h
	g++ -o rnxlog.o rnxlog.c -c -ansi -pedantic -Wall

rnxtrigger.o: rnxtrigger.c rnxtrigger.h
	g++ -o rnxtrigger.o rnxtrigger.c -c -ansi -pedantic -Wall

helper.o: helper.c helper.h
	g++ -o helper.o helper.c -c -ansi -pedantic -Wall

workThread.o: workThread.c workThread.h rnxState.h rnxlog.h rnxcommon.h rnxdata.h
	g++ -I $(CRAYDL_INC) -I $(BOOST_INC) -o workThread.o workThread.c -c -ansi -Wall

triggerThread.o: triggerThread.c triggerThread.h workThread.h rnxlog.h rnxcommon.h
	g++ -o triggerThread.o triggerThread.c -c -ansi -pedantic -Wall

