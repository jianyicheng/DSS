#!make
include ../env.tcl

BENCH = gSum
BUILDDIR_FILE = $(name)/_build
BUILDDIR_DS = $(BUILDDIR_FILE)/ds
BUILDDIR_DSS = $(BUILDDIR_FILE)/dss
BUILDDIR_SS = $(BUILDDIR_FILE)/ss
DS_BUILD = $(DHLS)/elastic-circuits/_build
VHDLDIR_FILE = $(name)/vhdl
VHDLDIR_DS = $(VHDLDIR_FILE)/ds
VHDLDIR_SS = $(VHDLDIR_FILE)/ss
VHDLDIR_DSS = $(VHDLDIR_FILE)/dss
#CLANG = ../llvm-6.0/bin/clang
#OPT = ../llvm-6.0/bin/opt


default: dss

demo: ds_frontend
	cp $(name)/support/$(name).dot $(name)/_build/ds
	cp $(name)/support/$(name)_bbgraph.dot $(name)/_build/ds/
	cp $(name)/support/$(name)_call_arg_analysis.rpt $(name)/_build/ds/
	python $(DSS)/src/bb_fix.py $(name) ds unbuf
	mkdir -p $(BUILDDIR_DSS)
	rm -rf $(BUILDDIR_DSS)/*
	mkdir -p $(BUILDDIR_DSS)/src
	mkdir -p $(VHDLDIR_FILE)
	mkdir -p $(VHDLDIR_DSS)
	rm -rf $(VHDLDIR_DSS)/*
	cp -r $(BUILDDIR_DS) $(BUILDDIR_DSS)/ds_$(name)
	@python $(DSS)/src/preprocess_dss.py $(name)
	@python $(DSS)/src/frontEndAnalysis.py $(name) || (echo "Error: DSS synthesis failed at the front end."; exit 3)
	@echo "Synthesising SS functions..."
	@cd $(BUILDDIR_DSS); $(VHLS) static.tcl || (echo "Error: SS in DSS synthesis failed."; exit 3)
	@echo "Synthesising DS functions..."
	#@bash $(BUILDDIR_DSS)/dynamic.sh || (echo "Error: DS in DSS synthesis failed."; exit 3)
	cp $(name)/support/$(name)_graph_buf.dot $(BUILDDIR_DSS)/ds_$(name)/$(name).dot
	@python $(DSS)/src/backEndAnalysis.py $(name) || (echo "Error: DSS synthesis failed at the back end."; exit 3)
	@echo "DSS synthesis done."


dss: ds_frontend
	python $(DSS)/src/bb_fix.py $(name) ds unbuf
	mkdir -p $(BUILDDIR_DSS)
	mkdir -p $(BUILDDIR_DSS)/src
	mkdir -p $(VHDLDIR_FILE)
	mkdir -p $(VHDLDIR_DSS)
	rm -rf $(BUILDDIR_DSS)/*
	cp -r $(BUILDDIR_DS) $(BUILDDIR_DSS)/ds_$(name)
	@python $(DSS)/src/preprocess_dss.py $(name)
	@python $(DSS)/src/frontEndAnalysis.py $(name) || (echo "Error: DSS synthesis failed at the front end."; exit 3)
	@echo "Synthesising SS functions..."
	@cd $(BUILDDIR_DSS); $(VHLS) static.tcl || (echo "Error: SS in DSS synthesis failed."; exit 3)
	@echo "Synthesising DS functions..."
	#@bash $(BUILDDIR_DSS)/dynamic.sh || (echo "Error: DS in DSS synthesis failed."; exit 3)
	@python $(DSS)/src/backEndAnalysis.py $(name) || (echo "Error: DSS synthesis failed at the back end."; exit 3)
	@echo "DSS synthesis done."

ds: 
	make name=$(name) ds_frontend
	make name=$(name) ds_backend

ds_backend:
	$(DHLS)/Buffers/bin/buffers buffers -filename=$(BUILDDIR_DS)/$(name) -period=10 || (echo "Error: Buffering DS circuit failed."; exit 3)
	cp $(BUILDDIR_DS)/$(name)_graph_buf.dot $(BUILDDIR_DS)/$(name).dot
	$(DHLS)/dot2vhdl/bin/dot2vhdl $(BUILDDIR_DS)/$(name)
	mkdir -p $(VHDLDIR_FILE)
	mkdir -p $(VHDLDIR_DS)
	cp $(BUILDDIR_DS)/*.vhd $(VHDLDIR_DS)
	@echo "DS synthesis done."

ds_frontend: csim
	mkdir -p $(BUILDDIR_FILE)
	mkdir -p $(BUILDDIR_DS)
	rm -rf $(BUILDDIR_DS)/*
	@echo "Preprocessing the files..."
	@python $(DSS)/src/preprocess_ds.py $(name) || (echo "Error: DSS synthesis failed at the front end - preprocessing."; exit 3)
	@llvm-link -S -v -o $(BUILDDIR_DS)/$(name).ll $(BUILDDIR_DS)/*.ll
	@$(OPT) -mem2reg  $(BUILDDIR_DS)/$(name).ll -S -o $(BUILDDIR_DS)/$(name)_mem2reg.ll
	@$(OPT) -loop-rotate -constprop  $(BUILDDIR_DS)/$(name)_mem2reg.ll -S -o $(BUILDDIR_DS)/$(name)_mem2reg_constprop.ll
	@$(OPT) -simplifycfg $(BUILDDIR_DS)/$(name)_mem2reg_constprop.ll -S -o $(BUILDDIR_DS)/$(name)_mem2reg_constprop_simplifycfg.ll
	@$(OPT) -die -instcombine -lowerswitch $(BUILDDIR_DS)/$(name)_mem2reg_constprop_simplifycfg.ll -S -o $(BUILDDIR_DS)/$(name)_mem2reg_constprop_simplifycfg_die.ll
	@cc -c ../dynamatic/elastic-circuits/FrequencyCounterPass/log_FrequencyCounter.c
	@$(CLANG) -fPIC -Xclang -load -Xclang $(DS_BUILD)/FrequencyCounterPass/libFrequencyCounterPass.so -c $(BUILDDIR_DS)/$(name)_mem2reg_constprop_simplifycfg_die.ll
	@cc -fPIC $(name)_mem2reg_constprop_simplifycfg_die.o log_FrequencyCounter.o
	@./a.out
	@rm a.out $(name)_mem2reg_constprop_simplifycfg_die.o log_FrequencyCounter.o
	@-$(CLANG) -Xclang -load -Xclang $(DS_BUILD)/FrequencyDataGatherPass/libFrequencyDataGatherPass.so $(BUILDDIR_DS)/$(name)_mem2reg_constprop_simplifycfg_die.ll -S
	@-$(OPT) -load $(DS_BUILD)/MemElemInfo/libLLVMMemElemInfo.so -load $(DS_BUILD)/ElasticPass/libElasticPass.so -load $(DS_BUILD)/OptimizeBitwidth/libLLVMOptimizeBitWidth.so -load $(DS_BUILD)/DSSAnalysis/libDSSAnalysis.so -load $(DS_BUILD)/MyCFGPass/libMyCFGPass.so -polly-process-unprofitable -mycfgpass  $(BUILDDIR_DS)/$(name)_mem2reg_constprop_simplifycfg_die.ll -S -use-lsq=true > /dev/null
	@mv print.txt $(BUILDDIR_DS) || true
	@mv *_graph.dot  $(BUILDDIR_DS)
	@mv *bbgraph.dot  $(BUILDDIR_DS)
	@mv *_call_arg_analysis.rpt $(BUILDDIR_DS)
	@mv *_pn_mem_analysis.rpt $(BUILDDIR_DS)
	@rm *.txt *.s debug_func stats
	@echo "Generating the dot graphs..."
	@python $(DSS)/src/graph_rename.py $(name) || (echo "Error: DSS synthesis failed at the front end - graph naming."; exit 3)
	@dot -Tpng $(BUILDDIR_DS)/*_graph.dot > $(BUILDDIR_DS)/$(name)_graph.png
	@dot -Tpng $(BUILDDIR_DS)/*bbgraph.dot > $(BUILDDIR_DS)/$(name)_bbgraph.png
	@cp $(BUILDDIR_DS)/$(name)_graph.dot $(BUILDDIR_DS)/$(name).dot
	
ss: csim
	@mkdir -p $(BUILDDIR_FILE)
	@mkdir -p $(BUILDDIR_SS)
	@rm -rf $(BUILDDIR_SS)/*
	@echo "Generating Vivado HLS tcl file..."
	@python $(DSS)/src/preprocess_ss.py $(name) || (echo "Error: SS preprocessing failed."; exit 3)
	@echo "Running Vivado HLS tcl file..."
	@cd $(BUILDDIR_SS); $(VHLS) static.tcl || (echo "Error: SS in Vivado HLS failed."; exit 3)
	@mkdir -p $(VHDLDIR_FILE)
	@mkdir -p $(VHDLDIR_SS)
	@cp -r $(BUILDDIR_SS)/ss_$(name)/solution1/impl/ip $(VHDLDIR_SS) || (echo "Error: SS in Vivado HLS failed. Cannot find output IP."; exit 3)
	@echo "SS synthesis done."

qss: csim 	# quick ss - debug only
	@mkdir -p $(BUILDDIR_FILE)
	@mkdir -p $(BUILDDIR_DS)
	@rm -rf $(BUILDDIR_SS)/*
	@echo "Generating Vivado HLS tcl file..."
	@python ../src/preprocess_ss.py $(name) debug || (echo "Error: SS preprocessing failed."; exit 3)
	@echo "Running Vivado HLS tcl file..."
	@cd $(BUILDDIR_SS); $(VHLS) static.tcl || (echo "Error: SS in Vivado HLS failed."; exit 3)
	@mkdir -p $(VHDLDIR_FILE)
	@mkdir -p $(VHDLDIR_SS)
	@cp -r $(BUILDDIR_SS)/ss_$(name)/solution1/sim/vhdl $(VHDLDIR_SS) || (echo "Error: SS in Vivado HLS failed. Cannot find VHDL files."; exit 3)
	@cp -r $(BUILDDIR_SS)/ss_$(name)/solution1/sim/tv $(VHDLDIR_FILE)
	@echo "SS synthesis done."

csim: 
	@echo "Load environment..."
	@echo "Start C simulation..."
	@g++ $(name)/src/*.cpp || (echo "Error: C simulation failed."; exit 3)
	@./a.out || (echo "Error: C simulation failed. Return value has to be 0"; exit 3)
	@rm a.out
	@echo "C simulation done."

clean-all:
	for dir in $(BENCH); do \
		rm -r $$dir/_build;\
		rm -r $$dir/vhdl; \
	done

clean:
	rm -r $(BUILDDIR_FILE)
	rm -r $(VHDLDIR_FILE)
