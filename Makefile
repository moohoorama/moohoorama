OPTIMIZATION?=-O3
STD=-std=c99 -pedantic
WARN=-Wall -Wimplicit-function-declaration
#-Werror
OPT=$(OPTIMIZATION)

PREFIX?=/usr/local
INSTALL_BIN=$(PREFIX)/bin
INSTALL=install
DEBUG=-g -ggdb

FINAL_CFLAGS=$(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS) $(REDIS_CFLAGS)
FINAL_LDFLAGS=$(LDFLAGS) $(REDIS_LDFLAGS) $(DEBUG)
FINAL_LIBS=-lm -lpthread

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
Y_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)
INSTALL=$(QUIET_INSTALL)$(INSTALL)

$(shell ./build_config.sh build_config.mk)

include build_config.mk

LIBS=$(SOURCES:.c=.o)
#LIBS=ywdlist.o ywtest.o ywq.o ywutil.o ywmain.o
BIN=ywtest

all: $(BIN)
	@echo ""
	@echo "Hint: To run 'make test' is a good idea ;)"
	@echo ""

.PHONY: all

include Makefile.dep

dep:
	$(Y_CC) -MM *.c > Makefile.dep
.PHONY: dep

%.o: %.c 
	$(Y_CC) -c $<

$(BIN): $(LIBS)
	$(Y_CC) -o $@ $^ $(ARC) $(FINAL_LIBS)

test: $(BIN) all
	@(./runtest)

clean:
	rm -rf $(BIN) $(LIBS)
