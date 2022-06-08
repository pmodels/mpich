# Code formatting and code-cleanup.sh

To keep our code base with a consistent style, we recommend all commits
filter through a common code formatter -- \`maint/code-cleanup.sh\`.
This script uses "GNU Indent" to format the source code with a
common/enforced set of rules.

## Rules

The current set of enforced "GNU Indent" options are (copied from the
code here for reference):

```
--no-blank-lines-after-declarations \
       `# --no-blank-lines-after-procedures` `# Overwritten below` \
       `# --break-before-boolean-operator` `# Overwritten below` \
--no-blank-lines-after-commas \
--braces-on-if-line \
--braces-on-struct-decl-line \
       `# --comment-indentation33` `# Overwritten below` \
--declaration-comment-column33 \
--no-comment-delimiters-on-blank-lines \
--cuddle-else \
--continuation-indentation4 \
       `# --case-indentation0` `# Overwritten below` \
       `# --else-endif-column33` `# Overwritten below` \
--space-after-cast \
--line-comments-indentation0 \
--declaration-indentation1 \
--dont-format-first-column-comments \
--dont-format-comments \
--honour-newlines \
--indent-level4 \
--parameter-indentation0 \
       `# --line-length75` `# Overwritten below` \
--continue-at-parentheses \
--no-space-after-function-call-names \
--no-space-after-parentheses \
--dont-break-procedure-type \
--space-after-for \
--space-after-if \
--space-after-while \
       `# --dont-star-comments` `# Overwritten below` \
--leave-optional-blank-lines \
--dont-space-special-semicolon \
       `# End of K&R expansion` \
--line-length100 \
--else-endif-column1 \
--start-left-side-of-comments \
--break-after-boolean-operator \
--comment-indentation1 \
--no-tabs \
--blank-lines-after-procedures \
--leave-optional-blank-lines \
--braces-after-func-def-line \
--brace-indent0 \
--cuddle-do-while \
--no-space-after-function-call-names \
--case-indentation4 \
```

## GNU Indent versions

For a style formatter to work, it is more important to keep it
consistent rather than "best". The latest version of GNU Indent is at
version 2.2.12 (2018), while the dominant version available through
distributions is version 2.2.11. The latest version had a few
improvements, in particular, it is better at detecting pointer type
declarations and pointer based functions. Because most stable
distribution is still using version 2.2.11 we require this version in
\`code-cleanup.sh\`.

For certain distribution, in particular, Mac using Homebrew, a custom
version need be installed. One can either install from source tar ball,
or using a custom brew recipe.

To install from \`indent-2.2.12\` tarball, simply do \`./configure &&
make && make install\`. On Mac, it will complain at "missing texi2html".
One can bypass that by manually edit \`Makefile\` by search \`^SUBDIRS\`
and delete the \`doc\` target.

Alternatively, there is patch to homebrew that one can try:

<https://gist.github.com/wesbland/501063f151c1eb815d8001abf2285cbe>
