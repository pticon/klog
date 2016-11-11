TARGET=klog


CC:=gcc
LD:=gcc
CFLAGS:=-Wall -Wunused -Werror
LDFLAGS:=


ifeq ($(DEBUG),1)
CFLAGS+=-O0 -g -DDEBUG
else
CFLAGS+=-O3 -DNDEBUG
endif


ifeq ($(VERBOSE),1)
Q=
echo-cmd =
else
Q=@
echo-cmd = @echo $(1)
endif


SRCS:=main.c
SRCS+=mapper.c
SRCS+=logger.c

OBJS:=$(SRCS:%.c=%.o)


all: $(TARGET)


$(TARGET): $(OBJS)
	$(call echo-cmd, "  LD   $@")
	$(Q)$(LD) $(LDFLAGS) -o $@ $^


%.o: %.c
	$(call echo-cmd, "  CC   $@")
	$(Q)$(CC) $(CFLAGS) -c $< -o $@


.PHONY: clean


clean:
	$(call echo-cmd, "  CLEAN")
	$(Q)rm -f $(TARGET) $(OBJS)
