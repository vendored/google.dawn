# How to contribute to Dawn

First off, we'd love to get your contributions to Dawn!

Everything helps other folks using Dawn and WebGPU: from small fixes and documentation
improvements to larger features and optimizations.
Please read on to learn about the contribution process.

## One time setup

### Contributor License Agreement

Contributions to this project must be accompanied by a Contributor License
Agreement. You (or your employer) retain the copyright to your contribution.
This simply gives us permission to use and redistribute your contributions as
part of the project. Head over to <https://cla.developers.google.com/> to see
your current agreements on file or to sign a new one.

You generally only need to submit a CLA once, so if you've already submitted one
(even if it was for a different Google project), you probably don't need to do
it again.

### Gerrit setup

Dawn's contributions are submitted and reviewed on [Dawn's Gerrit](https://dawn-review.googlesource.com).

Gerrit works a bit differently than Github (if that's what you're used to):
there are no forks. Instead everyone works on the same repository. Gerrit has
magic branches for various purpose:

 - `refs/for/<branch>` (most commonly `refs/for/main`) is a branch that anyone
can push to that will create or update code reviews (called CLs for ChangeList)
for the commits pushed.
 - `refs/changes/00/<change number>/<patchset>` is a branch that corresponds to
the commits that were pushed for codereview for "change number" at a certain
"patchset" (a new patchset is created each time you push to a CL).

#### Gerrit's .gitcookies

To push commits to Gerrit your `git` command needs to be authenticated. This is
done with `.gitcookies` that will make `git` send authentication information
when connecting to the remote. To get the `.gitcookies`, log-in to [Dawn's Gerrit](https://dawn-review.googlesource.com)
and browse to the [new-password](https://dawn.googlesource.com/new-password)
page that will give you shell/cmd commands to run to update `.gitcookie`.

#### Set up the commit-msg hook

Gerrit associates commits to CLs based on a `Change-Id:` tag in the commit
message. Each push with commits with a `Change-Id:` will update the
corresponding CL.

To add the `commit-msg` hook that will automatically add a `Change-Id:` to your
commit messages, run the following command:

```
f=`git rev-parse --git-dir`/hooks/commit-msg ; mkdir -p $(dirname $f) ; curl -Lo $f https://gerrit-review.googlesource.com/tools/hooks/commit-msg ; chmod +x $f
```

Gerrit helpfully reminds you of that command if you forgot to set up the hook
before pushing commits.

## The code review process

All submissions, including submissions by project members, require review.

### Discuss the change if needed

Some changes are inherently risky, because they have long-term or architectural
consequences, contain a lot of unknowns or other reasons. When that's the case
it is better to discuss it on the [Dawn Matrix Channel](https://matrix.to/#/#webgpu-dawn:matrix.org)
or the [Dawn mailing-list](https://groups.google.com/g/dawn-graphics/members).

### Pushing changes to code review

Before pushing changes to code review, it is better to run `git cl presubmit`
that will check the formatting of files and other small things.

Pushing commits is done with `git push origin HEAD:refs/for/main`. Which means
push to `origin` (i.e. Gerrit) the currently checkout out commit to the
`refs/for/main` magic branch that creates or updates CLs.

When code review asks for changes in the commits, you can amend them any way
you want (small fixup commit and `git rebase -i` are crowd favorites) and run
the same `git push origin HEAD:refs/for/main` command.

### Tracking issues

We usually like to have commits associated with issues in [Dawn's issue tracker](https://bugs.chromium.org/p/dawn/issues/list)
so that commits for the issue can all be found on the same page. This is done
by adding a `Bug: dawn:<issue number>` tag in the commit message. It is also
possible to reference Chromium or Tint issues with `Bug: tint:<issue number>` or
`Bug: chromium:<issue number>`.

Some small fixes (like typo fixes, or some one-off maintenance) don't need a
tracking issue. When that's the case, it's good practice to call it out by
adding a `Bug: None` tag.

It is possible to make issues fixed automatically when the CL is merged by
adding a `Fixed: <project>:<issue number>` tag in the commit message.

### Iterating on code review

Dawn follows the general [Google code review guidelines](https://google.github.io/eng-practices/review/).
Most Dawn changes need reviews from two Dawn committers. Reviewers will set the
"Code Review" CR+1 or CR+2 label once the change looks good to them (although
it could still have comments that need to be addressed first). When addressing
comments, please mark them as "Done" if you just address them, or start a
discussion until they are resolved.

Once you are granted rights (you can ask on your first contribution), you can
add the "Commit Queue" CQ+1 label to run the automated tests for Dawn. Once the
CL has CR+2 you can then add the CQ+2 label to run the automated tests and
submit the commit if they pass.

The "Auto Submit" AS+1 label can be used to make Gerrit automatically set the
CQ+2 label once the CR+2 label is added.
