# KLEO, 6/18/26
# Takes the Latex format of the MPI standard and sorts into all references to each function, printed to text files.
# NOTE: Run from outside the mpi-standard latex folder.
#       This works with the current (MPI-5.0) formating of the MPI Standard as it relies on the Latex tags and chapter 
#       naming conventions. If naming formatting is changed this file will need to be changed. 
import re
import subprocess
from os import path
from os import scandir
from os import getcwd
from os import remove

chapterPattern = re.compile("chap-")
subsectionPattern = re.compile("(\\\label\{subsec:)|(\\\label\{sec:)")
functionDeclPattern = re.compile('(\s)*function_name\(')
functionPattern = re.compile('[\"\']\s*(MPI_[aA-zZ_]*)[\"\']')
# Removes: advice for implimentors, tables, examples, comments, and non-section lables 
excludeStart = re.compile("(begin\{implementors\})|(begin\{table\})|(begin\{example\})")
excludeEnd = re.compile("(end\{implementors\})|(end\{table\})|(end\{example\})")
oneLineExclude = re.compile("(\\\mpitermtitleindex\{)|(\%\%)|(\\\label\{)|(\\\subsection\{)|(\\\section\{)")

def scanChapter(chapPath) -> None:
    with open(chapPath, "r") as chap: 
        subsectionHeader = ""
        subMap = {} # map name of sub/sections onto their content
        funcMap ={} # map name of functions onto their sub/sections
        collecting = False
        omitting = False
        nextLineFunc = False
        for line in chap:
            decl = functionDeclPattern.match(line)
            sub = subsectionPattern.match(line)
            if decl != None or nextLineFunc: 
                function = functionPattern.search(line)
                if function == None:
                    nextLineFunc = True
                else:
                    nextLineFunc = False
                    funcMap[function[1]] = []
            elif excludeStart.search(line) != None: omitting = True
            elif excludeEnd.search(line) != None: omitting = False
            elif sub != None:
                subsectionHeader = line[sub.end():-2]
                subMap[subsectionHeader] = ""
                collecting = True
            else:
                if collecting and (omitting == False) and oneLineExclude.match(line) == None: subMap[subsectionHeader] += line
        # Associate subsections with functions in chapter
        for func in funcMap:
            temp = re.sub("_", "\_", func)
            funcName = re.compile("[("+temp+")|(\{"+temp+"\})]", re.IGNORECASE)
            for subsection in subMap:
                if funcName.search(subMap[subsection]) != None:
                    funcMap[func].append(subsection)
        generateDescription(funcMap, subMap)
        
def generateDescription(funcMap, subMap) -> None:
    outPath = path.join(getcwd(), "doc/mansrc/maint/ai_prompt/workingPrompt.txt")
    for func in funcMap:
        with open("doc/mansrc/maint/"+func+"_Extracted.txt", "w+") as out:
            for sec in funcMap[func]:
                out.write(subMap[sec])
        subprocess.call("opencode --model 'argo/claudeopus45' run $(sed 's/FUNCTION/"+func+"/g' "+outPath+")", shell=True)
        remove("doc/mansrc/maint/"+func+"_Extracted.txt")

def main():
    rootPath = path.join(getcwd(), "doc/mansrc/maint/mpi-standard")
    # Scan whole latex folder for chapters, get un-rendered version and scan
    for entry in scandir(rootPath):
        if chapterPattern.match(entry.name) != None:
            for file in scandir(rootPath+"/"+entry.name):
                namePattern = re.compile(entry.name[5:]+"(-2)*.tex")
                if namePattern.match(file.name) != None or file.name == "prof.tex" or file.name == "mpit.tex":
                    chapterPath = path.join(rootPath, entry.name+"/"+file.name)
                    scanChapter(chapterPath)
                
if __name__ == "__main__":
    main()