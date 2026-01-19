OPTFLAGS=-O3
#OPTFLAGS=-g -fsanitize=address
#OPTFLAGS=-g
CC=clang++
#CFLAGS=$(OPTFLAGS) -I/usr/local/include -std=c++17
CFLAGS=$(OPTFLAGS) -I/opt/homebrew/include -std=c++17
S2FLAGS=-ls2
ABSLFLAGS=-labsl_log_internal_message -labsl_log_internal_check_op
JSONCPP=-ljsoncpp
LIBCURLFLAGS=-lcurl -lcurlpp
#LDFLAGS=-std=c++17 -L/usr/local/lib $(S2FLAGS) $(ABSLFLAGS) $(JSONCPP) $(LIBCURLFLAGS)
LDFLAGS=-std=c++17 -L/opt/homebrew/lib $(S2FLAGS) $(ABSLFLAGS) $(JSONCPP) $(LIBCURLFLAGS)

OBJ=run_timer.o point.o line.o portal.o link.o portal_factory.o team_count.o \
	link_factory.o field.o field_factory.o draw_tools.o arguments.o \
	uniform_distribution.o json_reader.o

EXEOBJ=test_run_timer.o test_point.o test_point.o test_point.o test_factory.o \
	test_team_count.o test_field.o test_arguments.o maxlayers.o maxfields.o \
	cyclonefields.o planner.o exofields.o cellfields.o mu.o shadow.o agentassign.o \
	portallist.o

all: maxlayers maxfields cyclonefields cellfields planner portallist mu shadow agentassign

tests: test_run_timer test_point test_line test_factory test_team_count test_field

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

test_arguments: $(OBJ) test_arguments.o arguments.o
	$(CC)  $(LDFLAGS) $(OBJ) -o test_arguments test_arguments.o

layerlinker: $(OBJ) layerlinker.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o layerlinker layerlinker.o

maxlayers: $(OBJ) maxlayers.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o maxlayers maxlayers.o

maxfields: $(OBJ) maxfields.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o maxfields maxfields.o

cyclonefields: $(OBJ) cyclonefields.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o cyclonefields cyclonefields.o

planner: $(OBJ) planner.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o planner planner.o

portallist: $(OBJ) portallist.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o portallist portallist.o

exofields: $(OBJ) exofields.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o exofields exofields.o

cellfields: $(OBJ) cellfields.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o cellfields cellfields.o

mu: $(OBJ) mu.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o mu mu.o

shadow: $(OBJ) shadow.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o shadow shadow.o

agentassign: $(OBJ) agentassign.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o agentassign agentassign.o

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJ) $(EXEOBJ)
