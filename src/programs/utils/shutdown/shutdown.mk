# src/programs/utils/shutdown/shutdown.mk
include Make.defaults

TARGET := $(BINDIR)/shutdown

LDFLAGS += 

all: $(TARGET)

.PHONY: all

include Make.rules