all: pci3e-reader pci3e-drain pci3e-test

pci3e-reader: pci3e-reader.cc ../pci3e-wrapper.cc
	g++ -o $@ -I.. $^

pci3e-drain: pci3e-drain.cc ../pci3e-wrapper.cc
	g++ -o $@ -I.. $^

pci3e-test: pci3e-test.c pci3e-funcs.c
	gcc -o $@ -I.. $^

clean:
	rm pci3e-reader pci3e-drain pci3e-test
