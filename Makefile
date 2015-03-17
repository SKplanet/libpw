all:
	$(RM) -rf build;\
	mkdir build;\
	cd build;\
	cmake -DCMAKE_BUILD_TYPE=Release ../src/

clean:
	$(RM) -rf build
