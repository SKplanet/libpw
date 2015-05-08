CMAKE=cmake -DCMAKE_BUILD_TYPE=Release

all:
	@echo "make [unix|gcc49|eclipse|xcode|clean]" 

unix:remake-build-dir
	cd build;\
	cmake -DCMAKE_BUILD_TYPE=Release ..

remake-build-dir:
	$(RM) -rf build;\
	mkdir build

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
