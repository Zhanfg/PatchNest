ifndef TARGET_COMPILE
    $(error TARGET_COMPILE not set)
endif

ifndef KP_DIR
    $(error KP_DIR not set)
endif

CC := $(TARGET_COMPILE)gcc
INCLUDE_DIRS := . include patch/include linux/include linux/arch/arm64/include linux/tools/arch/arm64/include
INCLUDE_FLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(KP_DIR)/kernel/$(dir))
CFLAGS += -O2 -Wall -Wextra -Werror=implicit-function-declaration -Werror=int-conversion

TARGET := patchnest-diagnostic-hello.kpm
OBJECTS := diagnostic_hello.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -r -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -Tdiagnostic_hello.lds -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
