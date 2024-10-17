OPTFLAGS=-g
CFLAGS=$(OPTFLAGS) -I/usr/local/include
CC=g++
LDFLAGS=-L/usr/local/lib
S2FLAGS=-ls2

OBJ=run_timer.o

test_run_timer: $(OBJ) test_run_timer.cpp
	$(CC) $(S2FLAGS) $(LDFLAGS) $(OBJ) -o test_run_timer test_run_timer.cpp 

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJ)
