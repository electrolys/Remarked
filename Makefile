SHELL := /bin/bash

CUSTOM_VARS=-DREMARKABLE=1 -s -pthread -lpthread



bin/remarked: obj/sqlite3.c.o obj/main.cpp.o obj/rmkit.h.o
	$(CXX) -o $@ $^ $(CFLAGS) $(CUSTOM_VARS)

obj/main.cpp.o: main.cpp rmkit.h sqlite3.h
	$(CXX) $(CFLAGS) $(CUSTOM_VARS) -O2 -c -o obj/main.cpp.o main.cpp 

obj/sqlite3.c.o: sqlite3.c rmkit.h sqlite3.h
	$(CC) $(CFLAGS) $(CUSTOM_VARS) -O2 -c -DSQLITE_OMIT_LOAD_EXTENSION -o obj/sqlite3.c.o sqlite3.c 

obj/rmkit.h.o: rmkit.h
	$(CXX) $(CFLAGS) $(CUSTOM_VARS) -O2 -xc++ - -c -DSTB_IMAGE_IMPLEMENTATION -DSTB_IMAGE_RESIZE_IMPLEMENTATION -DSTB_IMAGE_WRITE_IMPLEMENTATION -DSTB_TRUETYPE_IMPLEMENTATION -DRMKIT_IMPLEMENTATION -fpermissive -o obj/rmkit.h.o < rmkit.h


clean:
	rm obj/*
	rm bin/*
