OPTFLAGS=-O3
#OPTFLAGS=-g -fsanitize=address
#OPTFLAGS=-g
CXX=/usr/bin/clang++
CC=/usr/bin/clang++
METALCC=xcrun --sdk macosx metal
CFLAGS=$(OPTFLAGS) -I/usr/local/include -std=c++17
#CFLAGS=$(OPTFLAGS) -I/opt/homebrew/include -std=c++17
S2FLAGS=-ls2
ABSLFLAGS=-labsl_log_internal_message -labsl_log_internal_check_op
JSONCPP=-ljsoncpp
LIBCURLFLAGS=-lcurl -lcurlpp
#LDFLAGS=-std=c++17 -L/usr/local/lib $(S2FLAGS) $(ABSLFLAGS) $(JSONCPP) $(LIBCURLFLAGS)
LDFLAGS= $(OPTFLAGS) -std=c++17 -L/opt/homebrew/lib $(S2FLAGS) $(ABSLFLAGS) $(JSONCPP) $(LIBCURLFLAGS)
LDFLAGS_DYN= $(OPTFLAGS) -std=c++17 -L. -lcgress -Wl,-rpath,@loader_path -L/opt/homebrew/lib $(S2FLAGS) $(ABSLFLAGS) $(JSONCPP) $(LIBCURLFLAGS)
OBJ=run_timer.o point.o line.o portal.o link.o portal_factory.o team_count.o \
	link_factory.o field.o field_factory.o draw_tools.o arguments.o \
	uniform_distribution.o json_reader.o cell_factory.o

DYLIB=libcgress.dylib

EXEOBJ=test_run_timer.o test_point.o test_point.o test_point.o test_factory.o \
	test_team_count.o test_field.o test_arguments.o maxlayers.o maxfields.o \
	cyclonefields.o planner.o exofields.o cellfields.o mu.o shadow.o agentassign.o \
	portallist.o exofields.o getlinks.o orion.o  muconverter.o mudump.o mucomp.o

all: $(DYLIB) maxlayers maxfields cyclonefields cellfields planner portallist mu shadow agentassign \
 targetmu exofields getlinks orion

$(DYLIB): $(OBJ)
	$(CC) -dynamiclib -o $(DYLIB) $(OBJ) -Wl,-install_name,@rpath/$(DYLIB) $(LDFLAGS)

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

targetmu: $(OBJ) targetmu.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o targetmu targetmu.o

getlinks: $(OBJ) getlinks.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o getlinks getlinks.o

orion: $(OBJ) orion.o 
	$(CC)  $(LDFLAGS) $(OBJ) -o orion orion.o

muconverter: $(OBJ) muconverter.o 
	$(CC)  $(LDFLAGS)  $^ -o $@

mucomp: $(OBJ) mucomp.o 
	$(CC)  $(LDFLAGS)  $^ -o $@

mudump: $(OBJ) mudump.o 
	$(CC)  $(LDFLAGS)  $^ -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^

maxfields_metal: $(DYLIB) maxfields_metal.o default.metallib
	$(CC)  $(LDFLAGS_DYN) -framework Metal -framework Foundation -o maxfields_metal maxfields_metal.o 

test_clique: $(OBJ) test_clique.o maxfields_metal.o default.metallib
	$(CC)  $(LDFLAGS) $(OBJ) -framework Metal -framework Foundation -o test_clique test_clique.o field_metal.o

default.metallib: field_metal.metal
	$(METALCC) field_metal.metal -o default.metallib

maxfields_metal.o: maxfields_metal.mm
	$(CC) $(CFLAGS) -c maxfields_metal.mm

clean:
	rm -f $(OBJ) $(EXEOBJ) $(DYLIB)
	rm -f field_metal_test.air field_metal_test.metallib test_field_metal
	rm -f test_clique test_clique.o maxfields_metal.o default.metallib

