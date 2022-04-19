BENCHMARKS = foo doitgenTriple vecNormTrans split_two_loops vecNormTrans seqmem offchip gSum
name=foo

test-dass:
	(cd $(name); dass $(name))

test-baseline:
	(cd $(name); dass-baseline $(name))

clean:
	(cd $(name); rm -rf */; find . -type f -not \( -name "$(name).h*" -or -name "$(name).c*" \) -delete)

test-all:
	for bench in $(BENCHMARKS); do \
		cd $$bench; \
		dass $$bench; \
		cd ..; \
	done

clean-all:
	@for bench in $(BENCHMARKS); do \
		cd $$bench; \
		rm -rf */; \
		find . -type f -not \( -name "$$bench.h*" -or -name "$$bench.c*" \) -delete; \
		cd ..; \
	done
