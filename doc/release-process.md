# SilentDragon Release Process

## High-Level Philosophy

Beware of making high-risk changes too close to a new release, because they will not get as much testing as they should. Don't merge large branches which haven't undergone lots of testing just before a release.

It is best to keep doc/relnotes/README.md up to date as changes and bug fixes are made. It's more work to summarize all changes and bugfixes just before the release.

## Check for changes on master that should be on dev

See https://git.hush.is/hush/hush3/src/branch/master/doc/release-process.md#check-for-changes-on-master-that-should-be-on-dev , there is no sense repeating the exact same thing here.


## Dealing with merge conflicts

SD very often has merge conflicts in generated translation files, because QT embeds line numbers in XML. So even if you don't change translations, but add or remove even a single line from SD source code, it can change the XML. If there are actual changes to translations on both master and dev, you are out of luck and they need to be manually dealt with. But if you want to just use whatever is on master, you can do this:

```
git checkout dev
# this assumes you are using the remote called "origin"
git pull origin dev # make sure it is up to date
git merge --no-ff -X theirs master
```

The last command uses the "theirs" merge strategy option to the "recursive" merge strategy, which is default. See "git help merge" for more details.

If you have a messed up merge or you don't want to deal with conflicts right now, you can do `git merge --abort` to cancel a merge.

## Git Issues

Look for Git issues that should be fixed in the next release: https://git.hush.is/hush/SilentDragon/issues Especially low-risk and simple things and like documentation changes, improvements to error messages. Take note that changing strings in the source code, such as adding a new string or changing an existing one, will affect translations.

## Translations

...

```
# update generated translation data
./build.sh linguist
git commit -am "update translations"
git push
```

## Release process

* Update version in src/version.h
  * Sometimes the `dev` branch already has the new version and this is done already
* Verify a full build works correctly: `./build.sh clean; ./build.sh`
* Merge dev branch into master
* Make a new Gitea release from master branch
  * There is an option for Gitea to automate making a new Git tag as well
* SD requires a staticly compiled version of QT for release binaries
  * If it isn't, the binary will resort to using the system QT which is usually a different version and the binary will not work
* Make linux binaries:
```
# QT_STATIC is a directory with a staticly compiled qt5.x
# HUSH_DIR is a directory containing hushd/hush-cli binaries
# APP_VERSION is the version being released in git tag format (prepended v), such as v1.3.1
QT_STATIC=$HOME/src/qt5 APP_VERSION=vX.Y.Z HUSH_DIR=$HOME/git/hush3 src/scripts/mkrelease.sh
```
* Make linux binary tarball: `./src/scripts/make-binary-tarball.sh`
  * Upload to gitea release
* Make debian package with `./src/scripts/make-deb.sh`
  * Upload to gitea release
