from __future__ import print_function
import os, fnmatch, datetime, sys, re, glob, cxxfilt
import helper as helper

top = sys.argv[1]

fT = glob.glob(top+'/_build/ds/*_graph.dot')
for n in fT:
	line = n[n.rfind("/")+1:n.find("_graph.dot")]
	print(line)
	if (line.startswith('_Z')):
		print("Fixing naming issue: " + line + " >> " + cxxfilt.demangle(line)[0:cxxfilt.demangle(line).find("(")])
		print("mv "+top+"/_build/ds/"+line+"_graph.dot "+top+"/_build/ds/"+cxxfilt.demangle(line)[0:cxxfilt.demangle(line).find("(")]+"_graph.dot")
		os.system("mv "+top+"/_build/ds/"+line+"_graph.dot "+top+"/_build/ds/"+cxxfilt.demangle(line)[0:cxxfilt.demangle(line).find("(")]+"_graph.dot")

fT = glob.glob(top+'/_build/ds/*_bbgraph.dot')
for n in fT:
	line = n[n.rfind("/")+1:n.find("_bbgraph.dot")]
	print(line)
	if (line.startswith('_Z')):
		print("Fixing naming issue: " + line + " >> " + cxxfilt.demangle(line)[0:cxxfilt.demangle(line).find("(")])	
		print("mv "+top+"/_build/ds/"+line+"_bbgraph.dot "+top+"/_build/ds/"+cxxfilt.demangle(line)[0:cxxfilt.demangle(line).find("(")]+"_bbgraph.dot")
		os.system("mv "+top+"/_build/ds/"+line+"_bbgraph.dot "+top+"/_build/ds/"+cxxfilt.demangle(line)[0:cxxfilt.demangle(line).find("(")]+"_bbgraph.dot")

