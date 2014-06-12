OPTIMIZATION?=-O0
#STD=-std=c99 -pedantic
STD= -pedantic
WARN=-Wall -Werror -frtti
#-fno-rtti
OPT=$(OPTIMIZATION) -I./include/ -rdynamic -std=c++0x
OPT+= -I./thirdparty/gtest-1.7.0/include/ -L./
DEFINE=-DMAX_THREAD_COUNT=32 -DCACHE_LINE_SIZE=64
#-DDEBUG -DREPORT

PREFIX?=/usr/local
INSTALL_BIN=$(PREFIX)/bin
INSTALL=install
DEBUG=-g -ggdb

FINAL_CFLAGS=$(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS) $(REDIS_CFLAGS) $(DEFINE)
FINAL_LDFLAGS=$(LDFLAGS) $(REDIS_LDFLAGS) $(DEBUG)
FINAL_LIBS=-lm -lpthread -lgtest

#FINAL_CFLAGS+= -I../deps/hiredis -I../deps/linenoise -I../deps/lua/src

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
BINCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_INSTALL = @printf '    %b %b\n' $(LINKCOLOR)INSTALL$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
endif

Y_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
Y_CXX=$(QUIET_CC)$(CXX) $(FINAL_CFLAGS)
Y_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)
INSTALL=$(QUIET_INSTALL)$(INSTALL)

$(shell ./build_config.sh build_config.mk)

include build_config.mk

LIBS=$(SOURCES:.cc=.o)
EXE_LIBS=$(MAIN_SRC:.cc=.o)
#LIBS=ywdlist.o ywtest.o ywq.o ywutil.o ywmain.o
BIN=ywtest dumpcnk

all: $(BIN)
	@echo ""
	@echo "Hint: To run 'make test' is a good idea ;)"
	@echo ""

.PHONY: all

include Makefile.dep

dep:
	$(Y_CXX) -MM *.c > Makefile.dep
.PHONY: dep

.cc.o: gtest
	cpplint.py $<
	$(Y_CXX) -o $@ -c $<

gtest:
	#wget https://googletest.googlecode.com/files/gtest-1.7.0.zip
	cd thirdparty/gtest-1.7.0
	cmake .
	make

ywtest: $(LIBS) main/ywtest.cc
	cpplint.py include/*.h
	$(Y_CXX) -o $@ $^ $(ARC) $(FINAL_LIBS)
	nm $@ -f bsd -n -g -l -C  > $@.map

dumpcnk: $(LIBS) main/dumpcnk.cc
	cpplint.py include/*.h
	$(Y_CXX) -o $@ $^ $(ARC) $(FINAL_LIBS)
	nm $@ -f bsd -n -g -l -C  > $@.map


test: $(BIN) all
	@(./runtest)

cachegrind: $(BIN) all
	@(./runcache)

callgrind: $(BIN) all
	@(./runcall)

clean:
	rm $(BIN) $(LIBS) $(EXE_LIBS)
