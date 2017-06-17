MUDUO_DIRECTORY ?= /usr/local
#MUDUO_DIRECTORY ?= $(HOME)/build/install
MUDUO_INCLUDE = $(MUDUO_DIRECTORY)/include
MUDUO_LIBRARY = $(MUDUO_DIRECTORY)/lib
SRC = .
JSON_INCLUDE = $(SRC)/jsoncpp

CXXFLAGS = -DMUDUO_STD_STRING -static \
		   -std=c++11 -g -pthread -Wall -Wextra \
		   -Wno-unused-parameter \
		   -Woverloaded-virtual \
		   -Wpointer-arith -Wwrite-strings \
		   -march=native -rdynamic \
		   -I$(MUDUO_INCLUDE) -I$(JSON_INCLUDE) -I$(SRC)

LDFLAGS = -pthread -L$(MUDUO_LIBRARY) -lmuduo_net -lmuduo_base

bin = qcloud
all: $(bin)

OBJS0 = $(JSON_INCLUDE)/jsoncpp.o  $(SRC)/md5/md5.o 
OBJS = \
	   $(SRC)/main.o \
	   $(SRC)/Server.o \
	   $(SRC)/Util.o \
	   $(SRC)/InodeInfo.o \
	   $(SRC)/UserInfo.o \
	   $(SRC)/ConnectionContext.o

qcloud: $(OBJS0) $(OBJS)
	g++ -o $@ $^ $(LDFLAGS)

.cc.o:
	g++ -o $@ $< -c $(CXXFLAGS)
.cpp.o:
	g++ -o $@ $< -c $(CXXFLAGS)

.PHONY: all clean clean2
clean:
	rm -f core.* $(bin) $(OBJS)
clean2: clean
	rm -f $(OBJS0)
