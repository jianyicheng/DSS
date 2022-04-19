from optparse import OptionParser
import sys, os, time, datetime, glob
from pathlib import Path

def main():
    INFO  = "Vitis TCL Generator"
    USAGE = "Usage: python VitisSynthesisTCLGen.py -t $TOP ..."

    def showVersion():
        print(INFO)
        print(USAGE)
        sys.exit()

    optparser = OptionParser()
    optparser.add_option("-v", "--version", action="store_true", dest="showversion",
                         default=False, help="Show the version")
    optparser.add_option("-o", "--output", dest="outfile",
                         default="", help="Output file name")
    optparser.add_option("-t", "--top", dest="topmodule",
                         default="TOP", help="Top module, Default=TOP")
    optparser.add_option("-d", "--dass_dir", dest="dass_dir",
                         default="/workspace", help="DASS diretory, Default=/workspace")
    optparser.add_option("-j", "--threads", dest="threads",
                         default=1, help="threads, default = 1")
    optparser.add_option("--target", dest="target",
                         default="xc7z020clg484-1", help="target, default=xc7z020clg484-1, also support xcvu125-flva2104-1-i")
    (options, args) = optparser.parse_args()
    if options.target != 'xcvu125-flva2104-1-i' and options.target != 'xc7z020clg484-1':
        raise IOError('Found unsupported target')
    if options.target ==  'xcvu125-flva2104-1-i':
        target = 'xcvu'
    elif options.target == 'xc7z020clg484-1':
        target = 'zynq'

    if options.showversion:
        showVersion()
    top = options.topmodule
    pwd = os.getcwd()
    dass_dir = options.dass_dir
    cores = int(options.threads)

    tcl = open("syn.tcl", "w")
    tcl.write("create_project -force syn_project "+pwd+"/syn_project -part {}\n".format(options.target))

    os.chdir(".")
    for file in glob.glob(pwd+"/rtl/*.v*"):
        tcl.write("add_files -norecurse {"+file+"}\n")

    for file in Path('{}/dass/xlibs/syn_{}/fop'.format(options.dass_dir, target)).rglob('*.v*'):
        tcl.write("add_files -norecurse {"+str(file)+"}\n")

    # ss libraries
    if os.path.isdir("./vhls"):
        path, dirs, files = next(os.walk("./vhls"))
        for p in dirs:
            if p.startswith("ss") and p.endswith("ss"):
                ipDir = "vhls/"+p+"/solution1/impl/ip/tmp.srcs/sources_1/ip"
                if os.path.isdir(ipDir):
                    p, d, f = next(os.walk(ipDir))
                    for ip in d:
                        ipFile = ipDir+"/"+ip+"/"+ip+".xci"
                        if os.path.exists(ipFile):
                            tcl.write("import_ip "+pwd+"/"+ipFile+"\n")
    if target == 'zynq':
        # float libraries
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_zynq/fop/tmp.srcs/sources_1/ip/fop_ap_fdiv_14_no_dsp_32/fop_ap_fdiv_14_no_dsp_32.xci\n")
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_zynq/fop/tmp.srcs/sources_1/ip/fop_ap_fadd_3_full_dsp_32/fop_ap_fadd_3_full_dsp_32.xci\n")
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_zynq/fop/tmp.srcs/sources_1/ip/fop_ap_fmul_2_max_dsp_32/fop_ap_fmul_2_max_dsp_32.xci\n")
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_zynq/fop/tmp.srcs/sources_1/ip/fop_ap_fcmp_0_no_dsp_32/fop_ap_fcmp_0_no_dsp_32.xci\n")
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_zynq/fop/tmp.srcs/sources_1/ip/fop_ap_fsub_3_full_dsp_32/fop_ap_fsub_3_full_dsp_32.xci\n")

        # double libraries
        # TODO
    elif target == 'xcvu':
        # float libraries
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_xcvu/fop/tmp.srcs/sources_1/ip/fop_ap_fdiv_8_no_dsp_32/fop_ap_fdiv_8_no_dsp_32.xci\n")
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_xcvu/fop/tmp.srcs/sources_1/ip/fop_ap_fadd_3_full_dsp_32/fop_ap_fadd_3_full_dsp_32.xci\n")
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_xcvu/fop/tmp.srcs/sources_1/ip/fop_ap_fmul_2_max_dsp_32/fop_ap_fmul_2_max_dsp_32.xci\n")
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_xcvu/fop/tmp.srcs/sources_1/ip/fop_ap_fcmp_0_no_dsp_32/fop_ap_fcmp_0_no_dsp_32.xci\n")
        tcl.write("import_ip "+dass_dir+"/dass/xlibs/syn_xcvu/fop/tmp.srcs/sources_1/ip/fop_ap_fsub_3_full_dsp_32/fop_ap_fsub_3_full_dsp_32.xci\n")

        # double libraries
        # TODO
 
    # Floating point ip
    tcl.write("add_files -fileset constrs_1 -norecurse "+dass_dir+"/dass/components/clock.xdc\n")

    tcl.write("set_property top "+top+" [current_fileset]\n")
    tcl.write("update_compile_order -fileset sources_1\n")

    tcl.write("launch_runs synth_1 -jobs "+str(cores)+"\n")
    tcl.write("wait_on_run synth_1\n")
    tcl.write("open_run synth_1 -name synth_1\n")
    tcl.write("report_utilization -hierarchical -file "+pwd+"/syn_project/util.rpt\n")
    tcl.write("report_timing_summary -delay_type min_max -report_unconstrained -check_timing_verbose -max_paths 10 -input_pins -routable_nets -name timing_1 -file "+pwd+"/syn_project/timing.rpt\n")

if __name__ == '__main__':
    main()
