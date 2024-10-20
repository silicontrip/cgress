OPTFLAGS=-g
CFLAGS=$(OPTFLAGS) -I/usr/local/include -std=c++17
CC=clang++
S2FLAGS=-ls2
ABSLFLAGS=-labsl_log_internal_message -labsl_log_internal_check_op
JSONCPP=-ljsoncpp
LIBCURLFLAGS=-lcurl -lcurlpp
LDFLAGS=-L/usr/local/lib $(S2FLAGS) $(ABSLFLAGS) $(JSONCPP) $(LIBCURLFLAGS)

OBJ=run_timer.o point.o line.o portal.o link.o portal_factory.o team_count.o link_factory.o field.o field_factory.o draw_tools.o

all: test_run_timer test_point test_line test_factory test_team_count test_field
	

test_run_timer: $(OBJ) test_run_timer.o run_timer.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_run_timer test_run_timer.o

test_point: $(OBJ) test_point.o point.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_point test_point.o

test_line: $(OBJ) test_line.o line.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_line test_line.o

test_factory: $(OBJ) test_factory.o portal_factory.o link_factory.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_factory test_factory.o

test_team_count: $(OBJ) test_team_count.o team_count.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_team_count test_team_count.o 

test_field: $(OBJ) test_field.o field.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_field test_field.o

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJ)
