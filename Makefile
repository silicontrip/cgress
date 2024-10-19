OPTFLAGS=-g
CFLAGS=$(OPTFLAGS) -I/usr/local/include -std=c++17
CC=clang++
S2FLAGS=-ls2
ABSLFLAGS=-labsl_log_internal_message -labsl_log_internal_check_op
JSONCPP=-ljsoncpp
LIBCURLFLAGS=-lcurl -lcurlpp
LDFLAGS=-L/usr/local/lib $(S2FLAGS) $(ABSLFLAGS) $(JSONCPP) $(LIBCURLFLAGS)

OBJ=run_timer.o point.o line.o portal.o link.o portal_factory.o

all: test_run_timer test_point test_line test_portal_factory
	

test_run_timer: $(OBJ) test_run_timer.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_run_timer test_run_timer.o

test_point: $(OBJ) test_point.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_point test_point.o

test_line: $(OBJ) test_line.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_line test_line.o


test_portal_factory: $(OBJ) test_portal_factory.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_portal_factory test_portal_factory.o

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJ)
