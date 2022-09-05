#!/bin/sh
set -e
UPSTREAM_MILKYTRACKER_COMMIT=caa8169

error(){
  echo "\nERROR: $*" ; exit 1;
}

runverbose(){  set -x; "$@"; set +x; };
expecterror(){ set +e; "$@"; set +e; }

merge(){
  echo "\nMERGE: merging branches from coderofsalvation-repo"
  cd milkytracker; 
  git remote | grep coderofsalvation || git remote add coderofsalvation https://github.com/coderofsalvation/milkytracker
  git fetch coderofsalvation
  git reset $UPSTREAM_MILKYTRACKER_COMMIT --hard
  runverbose git merge --no-edit coderofsalvation/bugfix/sustain-keyjazz-note-instead-of-retriggering
  runverbose git merge --no-edit coderofsalvation/chore/copy-paste-sample-respect-relative-notenumber
  runverbose git merge --no-edit coderofsalvation/chore/remove-hardcoded-envelope-color
  runverbose git merge --no-edit coderofsalvation/claytone/adding-shortcuts-and-highlight-scope
  runverbose git merge --no-edit coderofsalvation/feat/channelmeters-for-mixing
  runverbose git merge --no-edit coderofsalvation/feat/linux-midi-specify-port
  runverbose git merge --no-edit coderofsalvation/feat/masteringlimiter
  runverbose git merge --no-edit coderofsalvation/feat/milky-navigation-shortcuts
  runverbose git merge --no-edit coderofsalvation/feat/nonaccelerated-compatibility-mode
  runverbose git merge --no-edit coderofsalvation/feat/responsive-filedialog
  runverbose git merge --no-edit coderofsalvation/feat/sampleditor-scripting
  runverbose git merge --no-edit coderofsalvation/feat/sampleeditor-improved-pasting
  runverbose git merge --no-edit coderofsalvation/feat/ASCIISTEP16-compatibility-stepsequencer-mode
  expecterror runverbose git merge --no-edit coderofsalvation/feat/show-note-backtrace-in-instrumentlistbox
  # solve conflict
  cat src/tracker/Tracker.cpp          | awk '!/^(<<<<|====|>>>>)/ {print $0}' > src/tracker/Tracker.cpp
  cd ..
}

patch(){
  echo "\nPATCH: patching"
  cp CMakeLists.txt milkytracker/.
  cd patch
  find src -type f | while read file; do 
    destfile=../milkytracker/$file
    test -f $destfile && rm $destfile 
    runverbose ln -s $(pwd)/$file $destfile
  done
  cd ..
  # set default milkytracker keyboard layout
  sed -i 's/EditModeFastTracker/EditModeMilkyTracker/g' milkytracker/src/tracker/TrackerInit.cpp
  sed -i 's/EditModeFastTracker/EditModeMilkyTracker/g' milkytracker/src/tracker/TrackerSettings.cpp
}

build(){
  mkdir -p milkytracker/build
  cd milkytracker/build
  cmake .. && make clean && make && ls -la src/tracker/milkytracker
}

all(){
  merge && patch && build
}

test -z "$1" && all
test -z "$1" || "$@"
