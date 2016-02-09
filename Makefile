all: parse_elf

%: %.cc
	$(CXX) -std=c++0x $< -o $@

clean:
	rm -f parse_elf

.PHONY: all clean
