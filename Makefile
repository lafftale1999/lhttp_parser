# --------------------------------------------------------------------------------------------------
# Standalone build of the http_parser library.
#
#   make            build build/libhttp_parser.a
#   make test       build and run every tests/*.c
#   make DEBUG=1    build with -g -O0 and address/undefined sanitizers
#   make clean
#
# To use the library from another project, include http_parser.mk instead of this file.
# --------------------------------------------------------------------------------------------------

ifeq ($(DEBUG),1)
LAFFTALE_HTTP_LIB_BUILD_DIR := build/debug
LAFFTALE_HTTP_LIB_CFLAGS    := -std=c11 -Wall -Wextra -g -O0 -fsanitize=address,undefined
LDFLAGS                     += -fsanitize=address,undefined
endif

include $(dir $(lastword $(MAKEFILE_LIST)))http_parser.mk

TEST_DIR  := $(LAFFTALE_HTTP_LIB_DIR)/tests
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(LAFFTALE_HTTP_LIB_BUILD_DIR)/tests/%,$(TEST_SRCS))

.PHONY: all test clean

all: $(LAFFTALE_HTTP_LIB_A)

$(LAFFTALE_HTTP_LIB_BUILD_DIR)/tests/%: $(TEST_DIR)/%.c $(LAFFTALE_HTTP_LIB_A)
	@mkdir -p $(@D)
	$(CC) $(LAFFTALE_HTTP_LIB_CFLAGS) $(LAFFTALE_HTTP_LIB_INC) -MMD -MP $< $(LAFFTALE_HTTP_LIB_A) $(LDFLAGS) -o $@

test: $(TEST_BINS)
	@failed=0; \
	for t in $(TEST_BINS); do \
		if $$t; then echo "PASS $$t"; else echo "FAIL $$t"; failed=1; fi; \
	done; \
	exit $$failed

clean:
	rm -rf $(LAFFTALE_HTTP_LIB_DIR)/build

-include $(TEST_BINS:=.d)

.DEFAULT_GOAL := all
