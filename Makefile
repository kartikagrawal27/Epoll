# use gcc with -fsanitize=thread bc clang on student vms (3.4) has buggy thread
# sanitizer
# this means we also need -ltsan in the link step. libtsan must be installed
# (sudo yum install libtsan on student vms)

OBJS_DIR = .objs

# define all the student executables
EXE_SERVER_THREAD=server_thread
EXE_SERVER_SELECT=server_select
EXE_SERVER_EPOLL=server_epoll
EXE_CLIENT=client
EXE_SIMULATOR=simulator
EXES_STUDENT=$(EXE_CLIENT) $(EXE_SERVER_SELECT) $(EXE_SERVER_THREAD) $(EXE_SERVER_EPOLL) $(EXE_SIMULATOR)

# list object file dependencies for each
OBJS_CHATROOM=$(EXE_CLIENT).o
OBJS_SERVER_EPOLL=$(EXE_SERVER_EPOLL).o
OBJS_SERVER_SELECT=$(EXE_SERVER_SELECT).o
OBJS_SERVER_THREAD=$(EXE_SERVER_THREAD).o
# set up compiler
CC = gcc
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter
CFLAGS_DEBUG   = -O0 $(WARNINGS) -g -std=c99 -c -MMD -MP -D_GNU_SOURCE -pthread
CFLAGS_RELEASE = -O2 $(WARNINGS) -g -std=c99 -c -MMD -MP -D_GNU_SOURCE -pthread
CFLAGS_TSAN    = $(CFLAGS_DEBUG)
CFLAGS_TSAN   += -fsanitize=thread -fPIC -pie

# set up linker
LD = gcc
LDFLAGS = -pthread
LD_TSAN_FLAGS = -ltsan -fPIC -pie

# the string in grep must appear in the hostname, otherwise the Makefile will
# not allow the assignment to compile
# IS_VM=$(shell hostname | grep "sp16-cs241")
#
# ifeq ($(IS_VM),)
# $(error This assignment must be compiled on the CS241 VMs)
# endif

.PHONY: all
all: release

# build types
# run clean before building debug so that all of the release executables
# disappear
.PHONY: debug
.PHONY: release
.PHONY: tsan

release: $(EXES_STUDENT)
debug:   clean $(EXES_STUDENT:%=%-debug)
tsan:    clean $(EXES_STUDENT:%=%-tsan)

# include dependencies
-include $(OBJS_DIR)/*.d

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

# patterns to create objects
# keep the debug and release postfix for object files so that we can always
# separate them correctly
$(OBJS_DIR)/%-debug.o: %.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

$(OBJS_DIR)/%-tsan.o: %.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_TSAN) $< -o $@

$(OBJS_DIR)/%-release.o: %.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_RELEASE) $< -o $@

# exes
# you will need a triple of exe and exe-debug and exe-tsan for each exe (other
# than provided exes)

$(EXE_CLIENT): $(OBJS_CHATROOM:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_CLIENT)-debug: $(OBJS_CHATROOM:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_CLIENT)-tsan: $(OBJS_CHATROOM:%.o=$(OBJS_DIR)/%-tsan.o)
	$(LD) $^ $(LD_TSAN_FLAGS) -o $@

$(EXE_SERVER_EPOLL): $(OBJS_SERVER_EPOLL:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_SERVER_EPOLL)-debug: $(OBJS_SERVER_EPOLL:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_SERVER_EPOLL)-tsan: $(OBJS_SERVER_EPOLL:%.o=$(OBJS_DIR)/%-tsan.o)
	$(LD) $^ $(LD_TSAN_FLAGS) -o $@

$(EXE_SERVER_SELECT): $(OBJS_SERVER_SELECT:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_SERVER_SELECT)-debug: $(OBJS_SERVER_SELECT:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_SERVER_SELECT)-tsan: $(OBJS_SERVER_SELECT:%.o=$(OBJS_DIR)/%-tsan.o)
	$(LD) $^ $(LD_TSAN_FLAGS) -o $@

$(EXE_SERVER_THREAD): $(OBJS_SERVER_THREAD:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_SERVER_THREAD)-debug: $(OBJS_SERVER_THREAD:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE_SERVER_THREAD)-tsan: $(OBJS_SERVER_THREAD:%.o=$(OBJS_DIR)/%-tsan.o)
	$(LD) $^ $(LD_TSAN_FLAGS) -o $@

.PHONY: clean
clean:
	-rm -rf .objs $(EXES_STUDENT) $(EXES_STUDENT:%=%-tsan) $(EXES_STUDENT:%=%-debug)
