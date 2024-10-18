OPTFLAGS=-g
CFLAGS=$(OPTFLAGS) -I/usr/local/include -std=c++17
CC=clang++
S2FLAGS=-ls2
ABSLFLAGS=-labsl_log_internal_message -labsl_log_internal_check_op
LDFLAGS=-L/usr/local/lib $(S2FLAGS) $(ABSLFLAGS)

OBJ=run_timer.o point.o

all: test_run_timer test_point
	

test_run_timer: $(OBJ) test_run_timer.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_run_timer test_run_timer.o

test_point: $(OBJ) test_point.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_point test_point.o

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJ)
