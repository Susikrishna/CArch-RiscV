CC = g++
FLAG = -std=c++17
Object = riscv_sim

all: ${Object}

%: %.cpp
	@${CC} ${FLAG} $< -o $@

clean:
	@find * -type f | grep -v -e "\." -e "Makefile"| xargs rm 
	@rm -f *.s