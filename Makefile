vhls=/scratch/shared/Xilinx
user=$(if $(shell id -u),$(shell id -u),9001)
group=$(if $(shell id -g),$(shell id -g),1000)

# Build Docker container
build-docker:
	(cd Docker; docker build --build-arg UID=$(user) --build-arg GID=$(group) --build-arg VHLS_PATH=$(vhls) . --tag dass-centos8)

# Enter Docker container
shell: build-docker
	docker run -it -u $(user) -v $(vhls):$(vhls) -v $(shell pwd):/workspace dass-centos8:latest /bin/bash

# Sync and update submodules
sync:
	git submodule sync
	git submodule update --init --recursive
	
# Build DASS from scratch
build: 
	bash ./dass/scripts/build-dass.sh
	
rebuild: clean build
	
clean:
	rm -rf ./llvm/build
	rm -rf ./dhls/elastic-circuits/build
	rm -rf ./dass/build

	(cd ./dhls/Buffers; make clean)
	(cd ./dass/tools/HlsVerifier; make clean)
	(cd ./dass/tools/dot2vhdl; make clean)

