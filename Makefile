vhls=/mnt/applications/Xilinx/20.1
dass=/workspace
user=$(if $(shell id -u),$(shell id -u),9001)
group=$(if $(shell id -g),$(shell id -g),1000)

build-docker: test-docker
	docker run -it -u $(user) -v $(vhls):/tools -v $(shell pwd):/workspace dass20:latest /bin/bash \
	-c "make build"
	echo "DASS has been installed successfully!"

test-docker:
	(cd docker; docker build --build-arg UID=$(user) --build-arg GID=$(group) . --tag dass20)

shell:
	docker run -it -u $(user) -v $(vhls):/tools -v $(shell pwd):/workspace dass20:latest /bin/bash

build:
	set -e # Abort if one of the commands fail
	# build LLVM
	mkdir -p $(dass)/llvm/build
	(cd $(dass)/llvm/build; \
	 cmake ../llvm -DLLVM_ENABLE_PROJECTS="clang;polly" \
	 -DLLVM_INSTALL_UTILS=ON -DLLVM_TARGETS_TO_BUILD="X86" \
	 -DLLVM_ENABLE_ASSERTIONS=ON \
	 -DLLVM_BUILD_EXAMPLES=OFF \
	 -DLLVM_ENABLE_RTTI=OFF \
	 -DLLVM_USE_LINKER=lld \
	 -DCMAKE_BUILD_TYPE=DEBUG || exit 1)
	(cd $(dass)/llvm/build; \
	 make -j4 || exit 1)
	
	# build Dynamatic
	mkdir -p $(dass)/dhls/elastic-circuits/build
	(cd $(dass)/dhls/elastic-circuits/build; \
	 cmake .. \
	 -DLLVM_ROOT=../../../llvm/build \
	 -DCMAKE_BUILD_TYPE=DEBUG || exit 1)
	(cd $(dass)/dhls/elastic-circuits/build; \
	 make -j4 || exit 1)
	
	# buffer
	mkdir -p $(dass)/dhls/Buffers/bin
	(cd $(dass)/dhls/Buffers; make || exit 1)
	# dot2vhdl
	mkdir -p $(dass)/dhls/dot2vhdl/bin
	(cd $(dass)/dhls/dot2vhdl; make || exit 1)
	# simulator
	(cd $(dass)/dhls/Regression_test/hls_verifier/HlsVerifier; make || exit 1)
	# TODO: LSQ generator

	# build DASS
	mkdir -p $(dass)/dass/build
	(cd $(dass)/dass/build; \
	 cmake .. \
	 -DLLVM_ROOT=../../llvm/build \
	 -DDHLS_ROOT=../dhls/elastic-circuits \
	 -DCMAKE_BUILD_TYPE=DEBUG || exit 1)
	(cd $(dass)/dass/build; \
	 make -j4 || exit 1)
	
	# build PRISM
	(cd $(dass)/prism/prism; make -j4 || exit 1)	
	
rebuild:
	set -e # Abort if one of the commands fail
	
	# build LLVM
	(cd $(dass)/llvm/build; \
	 make -j4 || exit 1)
	
	# build Dynamatic
	(cd $(dass)/dhls/elastic-circuits/build; \
	 make -j4 || exit 1)

	# build DASS
	(cd $(dass)/dass/build; \
	 make -j4 || exit 1)
	
clean:
	rm -rf $(dass)/llvm/build
	rm -rf $(dass)/dhls/elastic-circuits/build
	rm -rf $(dass)/dhls/Buffers/bin
	rm -rf $(dass)/dhls/dot2vhdl/bin
	rm -rf $(dass)/dass/build


