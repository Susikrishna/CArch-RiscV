COMPILER = g++
FLAG = -std=c++17
FINAL = riscv_sim

SOURCEFILES = $(wildcard *.cpp)
OBJECTFILES = $(SOURCEFILES: .cpp = .o)

all: ${FINAL}

${FINAL}:${OBJECTFILES}
	$(COMPILER) $(FLAG) $^ -o $@

%.o: %.cpp
	${COMPILER} ${FLAG} -c $<

clean:
	@rm -f ${OBJECTFILES} ${FINAL}