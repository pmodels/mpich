# KLEO, 7/9/2026
# Takes the src code and generates descriptions of MPIX functions. 

import re
import subprocess
from os import path
from os import scandir
from os import getcwd

def main():
    src = path.join(getcwd(), "src/binding/c")
    for entry in scandir(src):
        if re.compile("(.*)_api.txt").match(entry.name): 
            with open(path.join(src, entry)) as sect:
                for line in sect:
                    func = re.compile("(MPIX_.*):").match(line)
                    if func: 
                        mpixPrompt = path.join(getcwd(), "doc/mansrc/maint/ai_prompt/mpixPrompt.txt")
                        subprocess.call("opencode --model 'argo/claudeopus45' run $(sed 's/FUNCTION/"+func[1]+"/g' "+mpixPrompt+")", shell=True)
                
if __name__ == "__main__":
    main()