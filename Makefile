
DISABLE ?=

# note: these checks are counterintuitive, ifneq checks for the presence of the substring in the variable

# Debug symbols, invoked by make ENABLE=debug
ifneq (,$(findstring debug,$(ENABLE)))
CFLAGS_OPTIMIZE ?= -g
endif

CFLAGS_OPTIMIZE ?= -Ofast

CFLAGS += ${CFLAGS_OPTIMIZE} ${TARGET_ARCH}

CFLAGS += -Wall -Wextra -Wno-unknown-warning-option -Wno-unused-parameter -Wpointer-arith -Wuninitialized -Wno-unused-result -Wvla -Wshadow

LDLIBS += -lm

all : variation_navigator 

variation_navigator: CFLAGS += -I /Library/Frameworks/SDL2.framework/Headers
variation_navigator: LDFLAGS += -F /Library/Frameworks -framework SDL2 -framework Cocoa -rpath ./
variation_navigator: variation_navigator.o bitmap_text.o canvas.o sc88stpro_menu.o sc88stpro_midi_sysex.o

clean :
	rm -f *.o *.gch *.dSYM .DS_Store variation_navigator

*.o : Makefile

