# $Id$
# Author: Chris Ford <caf@slac.stanford.edu>

rnxread: rnxread.o rayonix_data.o
	g++ -o rnxread rnxread.o rayonix_data.o

rnxread.o: rnxread.cc ../rayonix_data.hh ../rayonix_common.hh
	g++ -I .. -o rnxread.o rnxread.cc -c -ansi -pedantic -Wall

rayonix_data.o: ../rayonix_data.cc ../rayonix_data.hh ../rayonix_common.hh
	g++ -I .. -o rayonix_data.o ../rayonix_data.cc -c -ansi -pedantic -Wall
