CMAKE=cmake -DCMAKE_BUILD_TYPE=Release

help: all

all:
	@echo "make [unix|gcc49|eclipse|xcode|clean|eclipse]" 

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
	CC=gcc-4.9 CXX=g++-4.9 $(CMAKE) ..

eclipse:remake-build-dir
	cd build;\
	$(CMAKE) -G"Eclipse CDT4 - Unix Makefiles" ../src

xcode:remake-build-dir
	cd build;\
	$(CMAKE) -G Xcode ..

clean:
	$(RM) -rf build
