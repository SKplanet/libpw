all:
	$(RM) -rf build; mkdir build; cd build; cmake ../src/

clean:
	$(RM) -rf build
