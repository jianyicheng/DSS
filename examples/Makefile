BENCH = gSum

all:
	./../src/dss ${name}

clean:
	for dir in $(BENCH); do \
		rm -r $$dir/_build;\
		rm -r $$dir/vhdl; \
	done
