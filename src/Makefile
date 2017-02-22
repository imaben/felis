# Makefile for Auto Deploy
COMPILER := clang
LIBNAME := felis
BUILD_DIRECTORY := build/
TARGET := $(BUILD_DIRECTORY)$(LIBNAME)
DFA=./dfa/dfa/
LIBDFA = $(DFA)build/libdfa.a
CFLAGS = -g -c `pkg-config --cflags libevent` -I$(LIBDFA)
LDFLAGS = -lm -lpthread `pkg-config --libs libevent`
HEADER_FILES := $(wildcard *.h)
OBJECT_FILES := $(addprefix $(BUILD_DIRECTORY),$(patsubst %.c,%.o,$(wildcard *.c)))

all: $(TARGET)


$(TARGET): $(BUILD_DIRECTORY) $(OBJECT_FILES)
	$(MAKE) -C $(DFA)
	$(COMPILER) -o $(TARGET) $(OBJECT_FILES) $(LIBDFA) $(LDFLAGS)

$(BUILD_DIRECTORY)%.o: %.c $(HEADER_FILES)
	$(COMPILER) -o $@ -c $< $(CFLAGS) -g

$(BUILD_DIRECTORY):
	@mkdir -p $(BUILD_DIRECTORY)

clean:
	rm -rf $(BUILD_DIRECTORY)
	$(MAKE) -C $(DFA) clean
