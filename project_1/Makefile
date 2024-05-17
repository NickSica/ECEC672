CXX_FLAGS = -Wall -Wextra -Werror
CXX_DEBUG_FLAGS = -Wall -Wextra -Werror -g
CXX_SRC_FILES = main.cpp file_parse.cpp math_functions.cpp

debug_build:
	g++ ${CXX_DEBUG_FLAGS} -o stat ${CXX_SRC_FILES}

all:
	g++ ${CXX_FLAGS} -o stat ${CXX_SRC_FILES}

run: all
	./stat "BENCHMARKS/Combinational1PO" "c17_1PO"

csv: all
	./stat -r "BENCHMARKS/Combinational1PO"

#ex_unsat: all
#	#./sat -l inputs_unsat/uuf50-0100.cnf
#	./sat -l inputs_unsat/uuf50-0163.cnf
#
#ex_sat: all
#	#./sat -l inputs/uf20-01.cnf
#	#./sat -l inputs/uf20-02.cnf
#	./sat -l inputs/uf20-0106.cnf
#	#./sat -l inputs/uf20-03.cnf
#	#./sat -l inputs/uf20-04.cnf
#
#unsat: all
#	for f in inputs_unsat/*; do \
#		./sat $$f; \
#		if [[ $$? != 0 ]]; then \
#			echo "Segmentation fault"; \
#			echo ""; \
#		fi \
#	done
#
#sat: all
#	for f in inputs/*; do \
#		./sat $$f; \
#		if [[ $$? != 0 ]]; then \
#			echo "Segmentation fault"; \
#			echo ""; \
#		fi \
#	done

debug: debug_build
	#gdb --args ./stat "BENCHMARKS/Combinational1PO" "c7552_1PO"
	#gdb --args ./stat "BENCHMARKS/Combinational1PO" "c432_1PO"
	gdb --args ./stat "BENCHMARKS/Combinational" "c17"

clean:
	-rm stat
