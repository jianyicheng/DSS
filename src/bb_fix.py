from __future__ import print_function
import os, fnmatch, datetime, sys, re, math
import helper as helper

top = sys.argv[1]
sch = sys.argv[2]
st = sys.argv[3] if len(sys.argv) > 3 else "unbuf"


buildDir=top+"/_build/"+sch
if st == "buf":
	ftemp = helper.fileOpen(buildDir+"/"+top+"_graph_buf.dot")
elif sch == "dss":
	ftemp = helper.fileOpen(buildDir+"/ds_"+top+"/"+top+".dot")
else:
	ftemp = helper.fileOpen(buildDir+"/"+top+".dot")

buf=[]
for line in ftemp:
	buf.append(line)
ftemp.close()

if st == "buf":
	ftemp = helper.fileOpen(buildDir+"/"+top+"_bbgraph_buf.dot")
elif sch == "dss":
	ftemp = helper.fileOpen(buildDir+"/ds_"+top+"/"+top+"_bbgraph.dot")
else:
	ftemp = helper.fileOpen(buildDir+"/"+top+"_bbgraph.dot")

bufBB = []
for line in ftemp:
	bufBB.append(line)
ftemp.close()

check = True
for line in buf:
	if "bbID= 1," in line:
		check = False

if check:
	helper.warning("Warning: bb offset found for benchmark "+top+". Fixing...")
	bbMin = 0
	bbMax = 0
	for line in buf:
		if "bbID= " in line:
			bb = helper.strFindNumber(line, "bbID= ")
			bb = int(bb)
			if bbMax < bb and bbMax == 0: bbMin = bb
			if bbMax < bb: bbMax = bb
	print("BB = 0+"+str(bbMin) + " - "+ str(bbMax))
	bb = bbMin
	i = 1
	while (bb <= bbMax):
		bufBB = [w.replace("block"+str(bb), "block"+str(i)) for w in bufBB]
		buf = [w.replace("bbID= "+str(bb), "bbID= "+str(i)) for w in buf]
		i = i + 1
		bb = bb + 1
	print("Now BB = 0 - "+ str(i-1))
	if st == "buf":
		os.system("mv "+buildDir+"/"+top+"_graph_buf.dot "+buildDir+"/"+top+"_graph_buf.dot_")
		os.system("mv "+buildDir+"/"+top+"_bbgraph_buf.dot "+buildDir+"/"+top+"_bbgraph_buf.dot_")
	else:
		os.system("mv "+buildDir+"/"+top+".dot "+buildDir+"/"+top+".dot_")
		os.system("mv "+buildDir+"/"+top+"_bbgraph.dot "+buildDir+"/"+top+"_bbgraph.dot_")
	if st == "buf":
		ftemp = open(buildDir+"/"+top+"_graph_buf.dot", "w")
	else:
		ftemp=open(buildDir+"/"+top+".dot", "w")
	for line in buf:
		ftemp.write(line)
	ftemp.close()
	if st == "buf":
		ftemp = open(buildDir+"/"+top+"_bbgraph_buf.dot", "w")
	else:
		ftemp=open(buildDir+"/"+top+"_bbgraph.dot", "w")
	first = True
	for line in bufBB:
		if first and "block" in line:
			bb = helper.strFindNumber(line, "block")
			bb = int(bb)
			i = 1
			while (i < bb):
				ftemp.write("\"block"+str(i)+"\";\n")
				i = i + 1
			first = False
		ftemp.write(line)
	ftemp.close()

