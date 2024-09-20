### User defined build options

ARCH ?=
CROSS_COMPILE ?=
OBJDIR = obj

WARN_CFLAGS   := -Wall -Wextra
OPT_CFLAGS    := -O2 -ffunction-sections -fdata-sections
DEBUG_CFLAGS  :=
LDFLAGS       := -Wl,--gc-sections

### Application-specific variables
APP_NAME := net_downlink
APP_SRCS := src/$(APP_NAME).c src/parson.c src/base64.c
APP_OBJS := $(OBJDIR)/$(APP_NAME).o $(OBJDIR)/parson.o $(OBJDIR)/base64.o
APP_LIBS := -lpthread

### Expand build options
CFLAGS := -std=c99 $(WARN_CFLAGS) $(OPT_CFLAGS) $(DEBUG_CFLAGS)
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar

### General build targets
all: $(APP_NAME)

clean:
	rm -f obj/*.o
	rm -f $(APP_NAME)

$(OBJDIR):
	mkdir -p $(OBJDIR)

### Compile main program
$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS) -Iinc -I./inc

### Link everything together
$(APP_NAME): $(APP_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) $(APP_LIBS)

### EOF
