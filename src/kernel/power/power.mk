NOSTDLIB=1
include Make.defaults

TARGET := $(BINDIR)/power

CFLAGS += $(CFLAGS_KERNEL)
ASFLAGS += $(ASFLAGS_KERNEL)
LDFLAGS += $(LDFLAGS_KERNEL)

SRCS = power.c

all: $(TARGET)

include ../../Make.rules