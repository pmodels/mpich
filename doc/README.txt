There are 3 kind of documents: User documentations, Developer documentations, and Developer notes. 

User documentations include man pages, info docs, and all documentations meant to be consumed by MPICH users or developers using MPICH rather than developing MPICH. These documents are often formatted, e.g. in LaTeX. They are located in doc/ folder, grouped into various subfolders.

Developer documentations are detailed and potentially lengthy documents that meant for MPICH developers. They are hosted on wiki.mpich.org website. These documents provide team-wide guidelines.

Developer notes are less formal and in concise code style that let developers describe or explain code that does not fit as comments in individual code, e.g. doc/notes/coll.txt provides an overview of collective functions, general parameters naming conventions and orderings etc. Because its overview nature, it does not fit in individual code, but it is important for providing context for them.  

Developer notes are prone to be out-of-sync with actual code. To prevent that, following guidelines are proposed:

    * Placed in doc/notes/, try to avoid sub-folders to help discovery and access -- use namespace-like prefix/suffix instead. Enables and encourages changes and reviews along with commits and PRs. File names use the similar styles as we name functions.

    * Use the style of code comments, ie short and concise, uses function/variable naming conventions.

    * Do not emphasize comprehensiveness. Encourage coder to put down notes that reflect their best understanding at the point to of writing code. 

    * Always add date signature as we add or make changes to them. If we add the date signature to the top of the text, it means the entire note has been reviewed and up to date to that signature. Otherwise, add date signature just above the blocks that is new or changed.

