#!/bin/bash

echo ==========================================================
echo Downloading libwebrtc
echo ==========================================================

set -e
set -v

export PATH=$DEPOT_TOOLS:$PATH
gclient config --unmanaged --spec 'solutions=[{"name":"src","url":"https://webrtc.googlesource.com/src.git"}]'
gclient sync --shallow --no-history --nohooks --with_branch_heads -r ${WEBRTC_REVISION} -R

# Restore the modification timestamps of all the files. This allows timestamps to be valid for rebuilds in CI
# if the developer doesn't have git-restore-mtime package installed, we'll just move along (this is not an error)
find build/external/libwebrtc/download/src -type d -name ".git" -execdir git restore-mtime \ || true;

python src/tools/clang/scripts/update.py
rm -f webrtc
ln -s src webrtc
