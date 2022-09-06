#!/bin/sh
set -e
UPSTREAM_MILKYTRACKER_COMMIT=caa8169

error(){
  echo "\nERROR: $*" ; exit 1;
}

runverbose(){  set -x; "$@"; set +x; };
expecterror(){ set +e; "$@"; set +e; }

pull(){
  echo "\nPULL: pulling+merging branches from coderofsalvation-repo"
  cd milkytracker; 
  git remote | grep coderofsalvation || git remote add coderofsalvation https://github.com/coderofsalvation/milkytracker
  git fetch coderofsalvation
  test -f CMakeLists.txt && git reset $UPSTREAM_MILKYTRACKER_COMMIT --hard
  git config --global user.email "CI@appveyor.com"
  git config --global user.name "ci-appveyor"
  git checkout master
  git reset $UPSTREAM_MILKYTRACKER_COMMIT --hard
  # copy this for creating diffpatch later
  test -d ../milkytracker.mainline && rm -rf ../milkytracker.mainline
  cp -r ../milkytracker ../milkytracker.mainline
  cd ..
}

merge(){
  cd milkytracker
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
  # solve conflict (accept both changes)
 # cp src/tracker/Tracker.cpp /tmp/. && awk '!/^(<<<<|====|>>>>)/ {print $0}' /tmp/Tracker.cpp > src/tracker/Tracker.cpp
  cd ..
}

patch(){

  create(){
    pull && merge
    echo "\nPATCH: patching"
    cd patch
    find . -type f | while read file; do 
      destfile=../milkytracker/$file
      test -f $destfile && rm $destfile 
      runverbose cp $(pwd)/$file $destfile
    done
    cd ..
    cp -r patch/resources/music/* milkytracker/resources/music/.
    # set default milkytracker keyboard layout & default colors
    sed -i 's/EditModeFastTracker/EditModeMilkyTracker/g' milkytracker/src/tracker/TrackerInit.cpp
    sed -i 's/EditModeFastTracker/EditModeMilkyTracker/g' milkytracker/src/tracker/TrackerSettings.cpp
    # set default resolution for notebooks/netbooks etc
    sed -i 's/#define DISPLAYDEVICE_WIDTH 640/#define DISPLAYDEVICE_WIDTH 1024/g' milkytracker/src/ppui/DisplayDeviceBase.h
    sed -i 's/#define DISPLAYDEVICE_HEIGHT 480/#define DISPLAYDEVICE_HEIGHT 768/g' milkytracker/src/ppui/DisplayDeviceBase.h
    # set default interpolation to fast sinc
    sed -i 's|settingsDatabase->store("INTERPOLATION", 1);|settingsDatabase->store("INTERPOLATION", 4);|g' milkytracker/src/tracker/TrackerSettings.cpp
    sed -i 's|settingsDatabase->store("HDRECORDER_INTERPOLATION", 1);|settingsDatabase->store("HDRECORDER_INTERPOLATION", 4);|g' milkytracker/src/tracker/TrackerSettings.cpp
    # set secondary highlight to 2 so ASCIISTEP16 makes immediate sense
    sed -i 's|settingsDatabase->store("HIGHLIGHTMODULO2", 8);|settingsDatabase->store("HIGHLIGHTMODULO2", 2);|g' milkytracker/src/tracker/TrackerSettings.cpp
    # enable backtrace note by default
    sed -i 's|settingsDatabase->store("INSTRUMENTBACKTRACE", 0);|settingsDatabase->store("INSTRUMENTBACKTRACE", 1);|g' milkytracker/src/tracker/TrackerSettings.cpp
    # now create the diff file 
    $(which diff) -Naur milkytracker.mainline milkytracker/ > patch/all.patch
    #cat patch/all.patch |less
  }

  apply(){
    set -x
    $(which patch) -p0 < patch/all.patch
  }

  test -z $1 && applypatch 
  test -z $1 || "$@"
}

release(){
  set -e
  set -x
  commit=$(git rev-parse --short HEAD)
  cd milkytracker
  git add resources/music src appveyor.yml CMakeLists.txt && git commit -m "release $commit + 1"
  expecterror git branch -D release
  git branch release
  git checkout release
  git push coderofsalvation release -f
}

build(){
  #cp CMakeLists.txt milkytracker/.
  mkdir -p milkytracker/build
  cd milkytracker/build
  cmake .. && make clean && make && ls -la src/tracker/milkytracker
}

all(){
  pull && patch && build
}

test -z "$1" && all
test -z "$1" || "$@"
