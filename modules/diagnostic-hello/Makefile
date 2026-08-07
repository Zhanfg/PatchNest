ifndef TARGET_COMPILE
    $(error TARGET_COMPILE not set)
endif

ifndef KP_DIR
    $(error KP_DIR not set)
endif

CC := $(TARGET_COMPILE)gcc
INCLUDE_DIRS := . include patch/include linux/include linux/arch/arm64/include linux/tools/arch/arm64/include
INCLUDE_FLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(KP_DIR)/kernel/$(dir))
CFLAGS := -O2 -Wall -Wextra -Werror=implicit-function-declaration -Werror=int-conversion
DEPFLAGS := -MMD -MP

TARGET := patchnest-diagnostic-hello.kpm
MAP := patchnest-diagnostic-hello.map
LINKER_SCRIPT := diagnostic_hello.lds
SOURCES := diagnostic_hello.c
OBJECTS := $(SOURCES:.c=.o)
DEPS := $(OBJECTS:.o=.d)
LDFLAGS := -r -Wl,-T,$(LINKER_SCRIPT) -Wl,-Map,$(MAP)

all: $(TARGET)

$(TARGET): $(OBJECTS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) $(INCLUDE_FLAGS) -c -o $@ $<

print-build-graph:
	@printf 'sources=%s\nobjects=%s\nlinker_script=%s\ntarget=%s\nmap=%s\n' \
		'$(SOURCES)' '$(OBJECTS)' '$(LINKER_SCRIPT)' '$(TARGET)' '$(MAP)'

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET) $(MAP)

-include $(DEPS)

.PHONY: all clean print-build-graph
