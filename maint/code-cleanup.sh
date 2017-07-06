#! /bin/bash

if hash gindent 2>/dev/null; then
	indent=gindent
else
	indent=indent
fi

indent_code()
{
    file=$1

    $indent \
        `# Expansion of Kernighan & Ritchie style` \
        --no-blank-lines-after-declarations \
        `# --no-blank-lines-after-procedures` `# Overwritten below` \
        `# --break-before-boolean-operator` `# Overwritten below` \
        --no-blank-lines-after-commas \
        --braces-on-if-line \
        --braces-on-struct-decl-line \
        `# --comment-indentation33` `# Overwritten below` \
        --declaration-comment-column33 \
        --no-comment-delimiters-on-blank-lines \
        `# --cuddle-else` `# Overwritten below` \
        --continuation-indentation4 \
        --case-indentation0 \
        `# --else-endif-column33` `# Overwritten below` \
        --space-after-cast \
        --line-comments-indentation0 \
        --declaration-indentation1 \
        --dont-format-first-column-comments \
        --dont-format-comments \
        --honour-newlines \
        --indent-level4 \
        --parameter-indentation0 \
        `# --line-length75` `# Overwritten below` \
        --continue-at-parentheses \
        --no-space-after-function-call-names \
        --no-space-after-parentheses \
        --dont-break-procedure-type \
        --space-after-for \
        --space-after-if \
        --space-after-while \
        `# --dont-star-comments` `# Overwritten below` \
        --leave-optional-blank-lines \
        --dont-space-special-semicolon \
        \
        --line-length100 \
        --else-endif-column1 \
        --start-left-side-of-comments \
        --break-after-boolean-operator \
        --comment-indentation1 \
        --no-tabs \
        --blank-lines-after-procedures \
        --leave-optional-blank-lines \
        --braces-after-func-def-line \
        --brace-indent0 \
        --cuddle-do-while \
        --no-space-after-function-call-names \
        --dont-break-procedure-type \
        ${file}

    rm -f ${file}~
    cp ${file} /tmp/${USER}.__tmp__ && \
	cat ${file} | sed -e 's/ *$//g' -e 's/( */(/g' -e 's/ *)/)/g' \
	-e 's/if(/if (/g' -e 's/while(/while (/g' -e 's/do{/do {/g' -e 's/}while/} while/g' > \
	/tmp/${USER}.__tmp__ && mv /tmp/${USER}.__tmp__ ${file}
}

usage()
{
    echo "Usage: $1 [filename | --all] {--recursive} {--debug}"
}

# Check usage
if [ -z "$1" ]; then
    usage $0
    exit
fi

# Make sure the parameters make sense
all=0
recursive=0
got_file=0
debug=
ignore=0
ignore_list="__I_WILL_NEVER_FIND_YOU__"
for arg in $@; do
    if [ "$ignore" = "1" ] ; then
	ignore_list="$ignore_list|$arg"
	ignore=0
	continue;
    fi

    if [ "$arg" = "--all" ]; then
	all=1
    elif [ "$arg" = "--recursive" ]; then
	recursive=1
    elif [ "$arg" = "--debug" ]; then
	debug="echo"
    elif [ "$arg" = "--ignore" ] ; then
	ignore=1
    else
	got_file=1
    fi
done
if [ "$recursive" = "1" -a "$all" = "0" ]; then
    echo "--recursive cannot be used without --all"
    usage $0
    exit
fi
if [ "$got_file" = "1" -a "$all" = "1" ]; then
    echo "--all cannot be used in conjunction with a specific file"
    usage $0
    exit
fi

if [ "$recursive" = "1" ]; then
    for i in `find . \! -type d | egrep '(\.c$|\.h$|\.c\.in$|\.h\.in$|\.cpp$|\.cpp.in$)' | \
	egrep -v "($ignore_list)"` ; do
	${debug} indent_code $i
	${debug} indent_code $i
    done
elif [ "$all" = "1" ]; then
    for i in `find . -maxdepth 1 \! -type d | egrep '(\.c$|\.h$|\.c\.in$|\.h\.in$|\.cpp$|\.cpp.in$)' | \
	egrep -v "($ignore_list)"` ; do
	${debug} indent_code $i
	${debug} indent_code $i
    done
else
    ${debug} indent_code $@
    ${debug} indent_code $@
fi
