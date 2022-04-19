#include <fstream>
#include <iostream>

#include "Help.h"
#include "HlsLogging.h"
#include "HlsVhdlTb.h"
#include "HlsVhdlVerification.h"
#include "Utilities.h"

#define XSIM

namespace hls_verify {
const string LOG_TAG = "VVER";

bool run_vhdl_verification(vector<string> args) {
  if (args.size() < 2) {
    log_err(LOG_TAG, "Not enough arguments.");
    cout << get_vhdl_verification_help_message() << endl;
    return true;
  }

  bool use_addr_width_32 = false;

  vector<string> temp;

  for (auto &arg : args) {
    if (arg.size() > 0 && arg[0] == '-') {
      if (arg == "-aw32") {
        use_addr_width_32 = true;
      }
    } else {
      temp.push_back(arg);
    }
  }

  args = temp;

  string c_tb_path = args[0];
  string vhdl_duv_entity_name = args[1];
  string c_fuv_function_name = vhdl_duv_entity_name;
  if (args.size() > 2) {
    c_fuv_function_name = args[2];
  }
  vector<string> other_c_paths;
  try {
    VerificationContext ctx(c_tb_path, "", c_fuv_function_name,
                            vhdl_duv_entity_name, other_c_paths);
    ctx.use_addr_width_32 = use_addr_width_32;
    execute_vhdl_testbench(ctx);
    check_vhdl_testbench_outputs(ctx);
    return true;
  } catch (string error) {
    log_err(LOG_TAG, error);
    return false;
  } catch (...) {
    log_err(LOG_TAG, "Unknown error");
    return false;
  }
}

void generate_vhdl_testbench(const VerificationContext &ctx) {
  HlsVhdlTb vhdlTb(ctx);
  ofstream fout(ctx.get_vhdl_testbench_path());
  fout << vhdlTb.generate_vhdl_testbench();
  fout.close();
}

void generate_modelsim_scripts(const VerificationContext &ctx) {
  vector<string> filelist_vhdl =
      get_list_of_files_in_directory(ctx.get_vhdl_src_dir(), ".vhd");
  vector<string> filelist_verilog =
      get_list_of_files_in_directory(ctx.get_vhdl_src_dir(), ".v");

  ofstream sim(ctx.get_modelsim_do_file_name());
  // sim << "vdel -all" << endl;
  sim << "vlib work" << endl;
  sim << "vmap work work" << endl;
  sim << "project new . simulation work modelsim.ini 0" << endl;
  sim << "project open simulation" << endl;
  for (auto it = filelist_vhdl.begin(); it != filelist_vhdl.end(); it++) {
    sim << "project addfile " << ctx.get_vhdl_src_dir() << "/" << *it << endl;
  }

  for (auto it = filelist_verilog.begin(); it != filelist_verilog.end(); it++) {
    sim << "project addfile " << ctx.get_vhdl_src_dir() << "/" << *it << endl;
  }
  sim << "project calculateorder" << endl;
  sim << "project compileall" << endl;
  sim << "eval vsim " << ctx.get_vhdl_duv_entity_name() << "_tb" << endl;
  sim << "run -all" << endl;
  sim << "exit" << endl;
  sim.close();
}

void generate_xsim_scripts(const VerificationContext &ctx) {
  vector<string> filelist_vhdl =
      get_list_of_files_in_directory(ctx.get_vhdl_src_dir(), ".vhd");
  vector<string> filelist_verilog =
      get_list_of_files_in_directory(ctx.get_vhdl_src_dir(), ".v");

  ofstream proj("proj.prj");
  for (auto it = filelist_vhdl.begin(); it != filelist_vhdl.end(); it++)
    proj << "vhdl work \"" << ctx.get_vhdl_src_dir() << "/" << *it << "\""
         << endl;
  for (auto it = filelist_verilog.begin(); it != filelist_verilog.end(); it++)
    proj << "sv work \"" << ctx.get_vhdl_src_dir() << "/" << *it << "\""
         << endl;
  proj.close();

  ofstream tcl("proj.tcl");
  tcl << "\n#log_wave -r /\nrun 300000ns\nquit\n";
  // tcl << "\n#log_wave -r /\nrun 100000ns\nquit\n";
  tcl.close();

  ofstream sh("run_xsim.sh");
  sh << "xelab " << ctx.get_vhdl_duv_entity_name() << "_tb "
     << "-prj proj.prj -L smartconnect_v1_0 "
     << "-L axi_protocol_checker_v1_1_12 "
     << "-L axi_protocol_checker_v1_1_13 "
     << "-L axis_protocol_checker_v1_1_11 "
     << "-L axis_protocol_checker_v1_1_12 "
     << "-L xil_defaultlib "
     << "-L unisims "
     << "-L unisims_ver "
     << "-L xpm  -L floating_point_v7_0_18 "
     << "-L floating_point_v7_1_11 "
     << "--lib \"ieee_proposed=./ieee_proposed\" "
     << "-s proj\n"
     << "xsim "
     << "--noieeewarnings proj -tclbatch proj.tcl";
  sh.close();
}

void check_vhdl_testbench_outputs(const VerificationContext &ctx) {
  const vector<CFunctionParameter> &output_params = ctx.get_fuv_output_params();
  cout << "\n--- Comparison Results ---\n" << endl;
  for (auto it = output_params.begin(); it != output_params.end(); it++) {
    bool result =
        compare_files(ctx.get_ref_out_path(*it), ctx.get_vhdl_out_path(*it),
                      ctx.get_token_comparator(*it));
    cout << "Comparison of [" + it->parameter_name + "] : "
         << (result ? "Pass" : "Fail") << endl;
  }
  cout << "\n--------------------------\n" << endl;
}

void execute_vhdl_testbench(const VerificationContext &ctx) {
  string command;

  // Generating VHDL testbench

  log_inf(LOG_TAG, "Generating VHDL testbench for entity " +
                       ctx.get_vhdl_duv_entity_name());
  generate_vhdl_testbench(ctx);

  // Copying supplementary files

  command = "cp " + extract_parent_directory_path(get_application_directory()) +
            "/resources/template_two_port_RAM.vhd " + ctx.get_vhdl_src_dir() +
            "/two_port_RAM.vhd";
  log_inf(LOG_TAG, "Copying supplementary files: [" + command + "]");
  execute_command(command);

  command = "cp " + extract_parent_directory_path(get_application_directory()) +
            "/resources/template_single_argument.vhd " +
            ctx.get_vhdl_src_dir() + "/single_argument.vhd";
  log_inf(LOG_TAG, "Copying supplementary files: [" + command + "]");
  execute_command(command);

  command = "cp " + extract_parent_directory_path(get_application_directory()) +
            "/resources/template_simpackage.vhd " + ctx.get_vhdl_src_dir() +
            "/simpackage.vhd";
  log_inf(LOG_TAG, "Copying supplementary files: [" + command + "]");
  execute_command(command);

#ifdef XSIM
  // Generating xsim script for the simulation
  generate_xsim_scripts(ctx);
#else
  // Generating modelsim script for the simulation
  command = "cp " + extract_parent_directory_path(get_application_directory()) +
            "/resources/modelsim.ini " + ctx.get_hls_verify_dir() +
            "/modelsim.ini";
  log_inf(LOG_TAG, "Copying supplementary files: [" + command + "]");
  execute_command(command);
  generate_modelsim_scripts(ctx);
#endif

  // Cleaning-up exisiting outputs

  command = "rm -rf " + ctx.get_vhdl_out_dir();
  log_inf(LOG_TAG, "Cleaning VHDL output files [" + command + "]");
  execute_command(command);

  command = "mkdir -p " + ctx.get_vhdl_out_dir();
  log_inf(LOG_TAG, "Creating VHDL output files directory [" + command + "]");
  execute_command(command);

#ifdef XSIM
  // Executing xsim
  system("bash run_xsim.sh");
#else
  // Executing modelsim
  command = "vsim -c -do " + ctx.get_modelsim_do_file_name();
  log_inf(LOG_TAG, "Executing modelsim: [" + command + "]");
  system(("vsim -c -do " + ctx.get_modelsim_do_file_name()).c_str());
#endif
}
} // namespace hls_verify
