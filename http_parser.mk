# --------------------------------------------------------------------------------------------------
# http_parser.mk - include this from another project's Makefile:
#
#   include path/to/http_parser/http_parser.mk
#
# Option A, link the static library:
#   CFLAGS += $(LAFFTALE_HTTP_LIB_INC)
#   app: $(OBJS) $(LAFFTALE_HTTP_LIB_A)
#   	$(CC) -o $@ $^
#
# Option B, compile the sources straight into your project:
#   SRCS   += $(LAFFTALE_HTTP_LIB_SRCS)
#   CFLAGS += $(LAFFTALE_HTTP_LIB_INC)
#
# Optional, set before including:
#   LAFFTALE_HTTP_LIB_BUILD_DIR   where objects and the library go   (default: <lib>/build)
#   LAFFTALE_HTTP_LIB_CONFIG_DIR  directory holding your own http_config.h (default: <lib>)
#   LAFFTALE_HTTP_LIB_CFLAGS      flags used when building the library
# --------------------------------------------------------------------------------------------------

# Must come before any other include in this file
LAFFTALE_HTTP_LIB_DIR        := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

LAFFTALE_HTTP_LIB_BUILD_DIR  ?= $(LAFFTALE_HTTP_LIB_DIR)/build
LAFFTALE_HTTP_LIB_CONFIG_DIR ?= $(LAFFTALE_HTTP_LIB_DIR)
LAFFTALE_HTTP_LIB_CFLAGS     ?= -std=c11 -Wall -Wextra -O2

LAFFTALE_HTTP_LIB_SRCS := $(wildcard $(LAFFTALE_HTTP_LIB_DIR)/src/*.c) \
                          $(wildcard $(LAFFTALE_HTTP_LIB_DIR)/src/content/*.c)

# Config dir comes first so a project-supplied http_config.h wins
LAFFTALE_HTTP_LIB_INC  := -I$(LAFFTALE_HTTP_LIB_CONFIG_DIR) \
                          -I$(LAFFTALE_HTTP_LIB_DIR)/include \
                          -I$(LAFFTALE_HTTP_LIB_DIR)/include/content \
                          -I$(LAFFTALE_HTTP_LIB_DIR)/include/common

LAFFTALE_HTTP_LIB_OBJS := $(patsubst $(LAFFTALE_HTTP_LIB_DIR)/src/%.c,$(LAFFTALE_HTTP_LIB_BUILD_DIR)/obj/%.o,$(LAFFTALE_HTTP_LIB_SRCS))
LAFFTALE_HTTP_LIB_A    := $(LAFFTALE_HTTP_LIB_BUILD_DIR)/libhttp_parser.a

# Don't let the rules below hijack the including project's default goal
_LAFFTALE_HTTP_LIB_SAVED_GOAL := $(.DEFAULT_GOAL)

$(LAFFTALE_HTTP_LIB_A): $(LAFFTALE_HTTP_LIB_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^

# Record the compile command so objects are rebuilt when flags change (e.g. toggling sanitizers)
_LAFFTALE_HTTP_LIB_FLAGS_FILE := $(LAFFTALE_HTTP_LIB_BUILD_DIR)/.cflags
_LAFFTALE_HTTP_LIB_FLAGS      := $(CC) $(LAFFTALE_HTTP_LIB_CFLAGS) $(LAFFTALE_HTTP_LIB_INC)

$(_LAFFTALE_HTTP_LIB_FLAGS_FILE): _lafftale_http_lib_force
	@mkdir -p $(@D)
	@echo '$(_LAFFTALE_HTTP_LIB_FLAGS)' | cmp -s - $@ || echo '$(_LAFFTALE_HTTP_LIB_FLAGS)' > $@

_lafftale_http_lib_force:

$(LAFFTALE_HTTP_LIB_BUILD_DIR)/obj/%.o: $(LAFFTALE_HTTP_LIB_DIR)/src/%.c $(_LAFFTALE_HTTP_LIB_FLAGS_FILE)
	@mkdir -p $(@D)
	$(CC) $(LAFFTALE_HTTP_LIB_CFLAGS) $(LAFFTALE_HTTP_LIB_INC) -MMD -MP -c $< -o $@

.PHONY: lafftale_http_lib_clean
lafftale_http_lib_clean:
	rm -rf $(LAFFTALE_HTTP_LIB_BUILD_DIR)

-include $(LAFFTALE_HTTP_LIB_OBJS:.o=.d)

.DEFAULT_GOAL := $(_LAFFTALE_HTTP_LIB_SAVED_GOAL)
