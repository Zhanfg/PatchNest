# Component import provenance

- PatchNest CLI: canonical history, relocated under ; baseline merge commit .
- PatchNest-Module: non-squashed usage: git subtree add   --prefix=<prefix> [-S[=<key-id>]] <commit>
   or: git subtree add   --prefix=<prefix> [-S[=<key-id>]] <repository> <ref>
   or: git subtree merge --prefix=<prefix> [-S[=<key-id>]] <commit>
   or: git subtree split --prefix=<prefix> [-S[=<key-id>]] [<commit>]
   or: git subtree pull  --prefix=<prefix> [-S[=<key-id>]] <repository> <ref>
   or: git subtree push  --prefix=<prefix> [-S[=<key-id>]] <repository> <refspec>

    -h, --help            show the help
    -q, --quiet           quiet
    -d, --debug           show debug messages
    -P, --[no-]prefix ... the name of the subdir to split out

options for 'split' (also: 'push')
    --[no-]annotate ...   add a prefix to commit message of new commits
    -b, --branch ...      create a new branch from the split subtree
    --[no-]ignore-joins   ignore prior --rejoin commits
    --[no-]onto ...       try connecting new tree to an existing one
    --[no-]rejoin         merge the new branch back into HEAD

options for 'add' and 'merge' (also: 'pull', 'split --rejoin', and 'push --rejoin')
    --[no-]squash         merge subtree changes as a single commit
    -m, --message ...     use the given message as the commit message for the merge commit
    -S, --[no-]gpg-sign[=<key-id>]
                          GPG-sign commits. The keyid argument is optional and defaults to the committer identity import from  main at .
- PatchNest-Kpms: non-squashed usage: git subtree add   --prefix=<prefix> [-S[=<key-id>]] <commit>
   or: git subtree add   --prefix=<prefix> [-S[=<key-id>]] <repository> <ref>
   or: git subtree merge --prefix=<prefix> [-S[=<key-id>]] <commit>
   or: git subtree split --prefix=<prefix> [-S[=<key-id>]] [<commit>]
   or: git subtree pull  --prefix=<prefix> [-S[=<key-id>]] <repository> <ref>
   or: git subtree push  --prefix=<prefix> [-S[=<key-id>]] <repository> <refspec>

    -h, --help            show the help
    -q, --quiet           quiet
    -d, --debug           show debug messages
    -P, --[no-]prefix ... the name of the subdir to split out

options for 'split' (also: 'push')
    --[no-]annotate ...   add a prefix to commit message of new commits
    -b, --branch ...      create a new branch from the split subtree
    --[no-]ignore-joins   ignore prior --rejoin commits
    --[no-]onto ...       try connecting new tree to an existing one
    --[no-]rejoin         merge the new branch back into HEAD

options for 'add' and 'merge' (also: 'pull', 'split --rejoin', and 'push --rejoin')
    --[no-]squash         merge subtree changes as a single commit
    -m, --message ...     use the given message as the commit message for the merge commit
    -S, --[no-]gpg-sign[=<key-id>]
                          GPG-sign commits. The keyid argument is optional and defaults to the committer identity import from  main at .
- Source branches are mirrored under  and .
- Source tags are mirrored under  and .
- Historical GitHub Releases remain immutable in the source repositories and are not rewritten.
-  remains independent and is intentionally not imported.
