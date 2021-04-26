BENCHMARKS = foo

test:
	for bench in $(BENCHMARKS); do \
		cd $$bench; \
		dass $$bench; \
	done

clean:
	@for bench in $(BENCHMARKS); do \
		cd $$bench; \
		rm -rf */; \
		find . -type f -not \( -name "$$bench.h*" -or -name "$$bench.c*" \) -delete; \
	done
