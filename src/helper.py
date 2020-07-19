# ====================================================
# Dynamic & Static Scheduling v1.0
#
# Some helper functions for the parsers
#
# Written by Jianyi Cheng
# ====================================================

from __future__ import print_function
import os, fnmatch, datetime, sys, re, subprocess

def removeComment(text):
    def replacer(match):
        s = match.group(0)
        if s.startswith('/'):
            return " " # note: a space and not an empty string
        else:
            return s
    pattern = re.compile(
    r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
    re.DOTALL | re.MULTILINE
    )
    return re.sub(pattern, replacer, text)

def fileOpen(file):
    if os.path.isfile(file):
        return open(file)
    else:
        error("Error: File not found - "+file)
        assert 0

def astGetCol(text, index):
    if "col:" in text[index]:
        return strFindNumber(text[index], "col:")
    elif "line:" in text[index]:
        return strFindNumber(text[index], "line:"+strFindNumber(text[index], "line:")+":")
    
def astGetLine(text, index):
    while "line:" not in text[index]:
        index = index -1
    return strFindNumber(text[index], "line:")
    
def strFindNumber(text, pattern):
    i = text.find(pattern)+len(pattern);
    resStr = ""
    while(text[i].isdigit()):
        resStr = resStr + text[i]
        i = i+1
    return resStr

class funcTree:
    def __init__(self):
        self.name = []
        self.sch = []
        self.ii = []
        self.compName = []
        self.latency = []
        self.arrays = []
        self.hwPort = []
        self.hwCons = []

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def error(text):
    print(bcolors.FAIL +text+ bcolors.ENDC)
    
def warning(text):
    print(bcolors.WARNING +text+ bcolors.ENDC)

def vhdlSigName(text):
    return text[0:text.find(":")].replace(" ", "").replace("\t", "")

def vhdlVarName(text):
    if text.find("_") > text.find(":"):
        return text[0:text.find(":")].replace(" ", "").replace("\t", "")
    else:
        return text[0:text.find("_")].replace(" ", "").replace("\t", "")

def findPath(text, textList):
    temp = text[text.find("=")+1:]
    i = 0
    while "$" in temp:
        if "$"+textList[i][0:textList[i].find("=")] in temp:
            temp = temp.replace("$"+textList[i][0:textList[i].find("=")],textList[i][textList[i].find("=")+1:]).replace("\n", "")
        elif "${"+textList[i][0:textList[i].find("=")]+"}" in temp:
            temp = temp.replace("${"+textList[i][0:textList[i].find("=")]+"}",textList[i][textList[i].find("=")+1:]).replace("\n", "")
        if i == textList.index(text)-1:
            i = 0
        else:
            i = i+1
        
    if "$" in temp:
        error("Error: generating path failed")
        assert 0
    else:
        return temp