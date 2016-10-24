CMAKE=cmake -DCMAKE_BUILD_TYPE=Release

ifndef ECLIPSE_VERSION
ECLIPSE_VERSION=4.6
endif

help: all

all:
	@echo "make [unix|gcc49|eclipse|xcode|clean]";\
	echo "ex) ECLIPSE_VERSION=4.7 make eclipse"

unix:remake-build-dir
	cd build;\
	cmake -DCMAKE_BUILD_TYPE=Release ..

remake-build-dir:
	$(RM) -rf build;\
	mkdir build

codelite:remake-build-dir
	cd build;\
	$(CMAKE) -G"CodeLite - Unix Makefiles" ../src
	
gcc49:remake-build-dir
	cd build;\
	CC=gcc-4.9 CXX=g++-4.9 $(CMAKE) ../src

eclipse:remake-build-dir
	cd build;\
	$(CMAKE) -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE \
		-DCMAKE_ECLIPSE_VERSION=$(ECLIPSE_VERSION) \
		-G"Eclipse CDT4 - Unix Makefiles" ../src

xcode:remake-build-dir
	cd build;\
	$(CMAKE) -G Xcode ../src

clean:
	$(RM) -rf build
