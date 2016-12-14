CMAKE=cmake -DCMAKE_BUILD_TYPE=Release

ifndef ECLIPSE_VERSION
ECLIPSE_VERSION=3.6
endif

help: all

all:
	@echo "make [unix|eclipse|xcode|clean|eclipse|codelite]";\
	echo "ex) ECLIPSE_VERSION=4.7 make eclipse"

unix:remake-build-dir
	cd build;\
	cmake -DPW_BUILD_TYPE=unix -DCMAKE_BUILD_TYPE=Release ..

remake-build-dir:
	$(RM) -rf build;\
	mkdir build

codelite:remake-build-dir
	cd build;\
	$(CMAKE) -DPW_BUILD_TYPE=codelite -G"CodeLite - Unix Makefiles" ..
	
eclipse:remake-build-dir
	cd build;\
	$(CMAKE) -DPW_BUILD_TYPE=eclipse \
		-DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE \
		-DCMAKE_ECLIPSE_VERSION=$(ECLIPSE_VERSION) \
		-G"Eclipse CDT4 - Unix Makefiles" ../src

xcode:remake-build-dir
	cd build;\
	$(CMAKE) -DPW_BUILD_TYPE=xcode -G Xcode ..

clean:
	$(RM) -rf build
