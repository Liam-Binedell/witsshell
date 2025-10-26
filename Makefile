CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g
LDFLAGS =

SRCS = witsshell.c
OBJS = $(SRCS:.c=.o)
TARGET = witsshell

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

# If the repository includes a tester directory with run-tests.sh, run it.
test: $(TARGET)
	@if [ -x ./test-witsshell.sh ]; then \
		( ./test-witsshell.sh ); \
	else \
		echo "No tester/run-tests.sh found or not executable; run tests manually."; \
	fi