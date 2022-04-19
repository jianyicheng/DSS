#!/usr/bin/python3

import argparse
import os


SPLIT_FLAG = "open_project"
WORK_DIR = "./"
PARALLEL_TXT = "parallel.txt"
VHLS = "vitis_hls"

def split_vhls_script(vhls_script_path: str) -> None:
  """
  split vitis hls tcl into separate files
  """
  orig_tcl = open(vhls_script_path, 'r').readlines()

  # split based on the flag "open_project"
  split_pos = [i for i, line in enumerate(orig_tcl) if SPLIT_FLAG in line]
  split_result = [orig_tcl[i:j] for i, j in zip(split_pos, split_pos[1:] + [None])]

  for idx, sub_tcl in enumerate(split_result):
    open(f"{WORK_DIR}/vhls_{idx}.tcl", "w").write("".join(sub_tcl))
  
  # generate the file including all parallel tasks
  all_tasks = [f"{VHLS} vhls_{idx}.tcl" for idx in range(len(split_result))]
  open(f"{WORK_DIR}/{PARALLEL_TXT}", "w").write("\n".join(all_tasks))


def launch_parallel_vhls() -> None:
  """
  launch all vitis_hls instances in parallel
  """
  os.system(f"cd {WORK_DIR}; parallel < {PARALLEL_TXT}")


if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("--vhls_script", type=str, required=True)
  args = parser.parse_args()

  split_vhls_script(args.vhls_script)
  launch_parallel_vhls()
