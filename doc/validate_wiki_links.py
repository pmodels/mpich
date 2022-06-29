import sys
import os
import time
import getopt
import re
import requests

# Globals
_WEB_LINKS = 0
_HELP_MSG=\
"""
Usage: python3 validate_wiki_links.py [OPTIONS]
This script needs to be run from the mpich/doc/ directory. It will crawl
through the mpich/doc/wiki directory and sub-directories, processing all
markdown files. Any link within the file is validated to ensure it still works.
If the link is a web link, it will check for a 200 HTTP response code. If the
link is to another file, it will validate if that file exists.

Short and Long OPTIONS:
    -h, --help          Display this help message
    --web-links         Enable validating of web links. Disabled by default
"""
_ERROR_MSG=\
"Error parsing option. Please use '--help' for more a list of valid options."
_ALL_FILES = {}

# Functions

'''
crawl_dir: Crawls through the starting directory and recurses for any
sub-directory. All files are added to an array and returned. Additionally all
files are added to the global _All_FILES dictionary for later use.
'''
def crawl_dir(dir_start):
    global _ALL_FILES
    files = []

    directory = os.fsencode(dir_start)

    for file in os.listdir(directory):
        filename = os.fsdecode(file)
        full_path = os.path.join(dir_start, filename)

        if(os.path.isdir(full_path)):
            # Recursive call for a sub-directory
            files += crawl_dir(full_path)
        else:
            # We only want to process markdown files
            if(os.path.splitext(filename)[1] == ".md"):
                files.append(full_path)

                # We only want to add the actual file name and extension
                _ALL_FILES[os.path.basename(filename)] = 0

    return files

'''
find_links: We look for any links of the pattern [<text>](<link>) within the
document. We return all matches that are found.
'''
def find_links(file_name):
    pattern = re.compile(r'\[.*\]\(.*\)')
    content = open(file_name).read()
    return pattern.findall(content, re.M)

'''
parse_links: We process the found links and split them into their respective
<text> and <link> sections. The <link> content is aggregated and returned.
'''
def parse_links(links):
    l = []

    for link in links:
        # <Text>
        page = link[link.find("[")+1:link.find("]")]
        #<Link>
        link = link[link.find("(")+1:link.find(")")]
        
        # In Markdown you can have a link such as
        # ["my link"](<link> "wikilink")
        # We still want to process those links, and just skip the remaining
        # text
        if len(link.split()) > 1:
            link = link.split()[0]

        l.append(link)

    return l

'''
validate_links: Here we call the previous two functions to get all of our links
properly parsed and collected. Once we have the list we then check whether the
link is a web link, mailto link, or a file link.
- Web links are checked for a 200 HTTP response code to be considered valid.
- Mailto links are skipped. 
- Files are checked to see if they exist in their relative locations.

We return a dictionary of each file and any broken links attached to it.

Additionally we count each time a file is linked to and store that information
in the global _ALL_FILES dictionary.
'''
def validate_links(file_name):
    global _ALL_FILES
    global _WEB_LINKS

    web_link_pattern = re.compile(r'http(|s):\/\/.*\.*')
    mailto_pattern = re.compile(r'mailto:.*@.*\.[a-zA-Z0-9][a-zA-Z0-9]*')
    ret = {"File": file_name, "Broken": []}

    links = parse_links(find_links(file_name))

    for link in links:
        # Web links
        if web_link_pattern.match(link):
            # If we dont have web link checking enabled, skip
            if not _WEB_LINKS:
                continue
            try:
                response = requests.head(link)

                if not response.status_code == 200:
                    ret["Broken"].append(link)
            except requests.exceptions.RequestException as e:
                ret["Broken"].append(link)

        # Mailto links - skip these by default, no easy way to validate
        elif mailto_pattern.match(link):
            continue
        # File links
        else:
            # If this is a markdown relative link, skip it
            if link.startswith('#'):
                continue

            # If file exists in our _ALL_FILES dictionary, increase its link 
            # count by 1.
            if os.path.basename(link) in _ALL_FILES:
                _ALL_FILES[os.path.basename(link)] += 1

            directory = os.path.dirname(file_name)
            file_path = os.path.join(directory, link)

            if not os.path.exists(file_path):
                ret["Broken"].append(file_path)

    return ret

'''
parse_arguments: Parse command lind arguments
'''
def parse_arguments(argv):
    global _HELP_MSG
    global _ERROR_MSG
    global _WEB_LINKS

    try:
        opts, args = getopt.getopt(argv, "h", ["help", "web-links"])
    except getopt.GetoptError:
        print(_ERROR_MSG)
        sys.exit()
    if args:
        print(_ERROR_MSG)
        sys.exit()
    for opt, arg in opts:
        # Output help message
        if opt in ("-h", "--help"):
            print(_HELP_MSG)
            sys.exit()
        # Used to enable checking web links
        elif opt in ("--web-links"):
            _WEB_LINKS = 1
# Main

def main(argv):
    global _VERBOSE
    global _ALL_FILES

    links = []

    parse_arguments(argv)
    all_files = crawl_dir("wiki")

    for file in all_files:
        links.append(validate_links(file))

    print("==== Broken Links ====")

    for link in links:
        if link["Broken"]:
            print("File: ", link["File"])
            for b in link["Broken"]:
                print("    Link: ", b)
            print()
    print()
    
    print("==== Unlinked Files ====")

    for f in _ALL_FILES:
        if _ALL_FILES[f] == 0:
            print(f)

if __name__ == "__main__":
    main(sys.argv[1:])
