SUDO := sudo
CFLAGS = -I. -g -D_GNU_SOURCE
CXXFLAGS = -I. -I/usr/local/include
LDXXFLAGS = -L/usr/local/lib -lCppUTest
TARGET = scanner
TARGET_OBJ = main.o
TEST_TARGET = tests/test
TEST_OPS := -c -v
SRC := scanner.c tracker.c \
	scanner4.c scanner4_tcp.c scanner4_udp.c \
	scanner6.c scanner6_tcp.c scanner6_udp.c
OBJ := $(SRC:.c=.o)
TMP := *~ *.swp a.out **/*~ **/*.swp **/a.out
DEPS = utils.h scanner.h tracker.h \
       scanner4.h scanner4_tcp.h scanner4_udp.h \
       scanner6.h scanner6_tcp.h scanner6_udp.h
TEST = tests/main.c tests/scanner_test.c tests/tracker_test.c
TEST_OBJ := $(TEST:.c=.o)

.PHONY: all test clean
all: $(TARGET)
$(TARGET): main.o $(OBJ)
	$(CC) -o $@ $^

test: $(TEST_TARGET)
	$(SUDO) ./$(TEST_TARGET) $(TEST_OPS)
$(TEST_TARGET): $(TEST_OBJ) $(OBJ)
	$(CXX) -o $@ $^ $(LDXXFLAGS)

tests/%.o: tests/%.c $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(RM) $(OBJ) $(TARGET_OBJ) $(TEST_OBJ) $(TMP) $(TARGET) $(TEST_TARGET)
