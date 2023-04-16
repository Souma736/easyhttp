CC=g++
SUBDIRS=comm
ROOT_DIR=$(shell pwd)
OBJS_DIR=obj
BIN_DIR=bin
CUR_SOURCE=${wildcard *.cpp}
CUR_OBJS=${patsubst %.cpp, %.o, $(CUR_SOURCE)}
export CC BIN OBJS_DIR BIN_DIR ROOT_DIR
$(SUBDIRS):ECHO
	make -C $@

ECHO:
	@echo $(SUBDIRS)

clean:
	@rm $(OBJS_DIR)/*.o
	@rm -rf $(BIN_DIR)/*

easyhttp:$(SUBDIRS)
	$(CC) -o bin/easyhttp easyhttp.cpp $(OBJS_DIR)/*.o -std=c++17 -lpthread -Wall -O3