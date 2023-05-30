#!/bin/bash

if [ ! -d data ]; then
  echo "No ./data directory, make sure to run this script from the root of the repo."
  exit 1
fi

if [ ! -e data/fonts/heavy.ttf -o ! -e data/fonts/light.ttf -o ! -e data/music/retro.sf2 ]; then
  echo "Crystal Picnic requires three fonts and one MIDI soundfont which are not available in the git repo, and at least one of them seems missing."
  echo "Check data/Notice.txt for details. Aborting, as the resulting data.cpa would be incomplete."
  exit 2
fi

ROOT=$(pwd)
mkdir -p ${ROOT}/build

if [ ! -x "${ROOT}/tools/packtiles2" ]; then
  echo "The tools/packtiles2 script seems missing, trying to build it with GCC."
  g++ ${ROOT}/tools/utils/packtiles2.cpp -lallegro -lallegro_image -o tools/packtiles2
fi

cp -a data __data.tmp__
cd __data.tmp__/areas/tiles
${ROOT}/tools/packtiles2 .png *

for f in `find . -name "*_new.png"` ; do mv $f `echo $f | sed -e 's/_new//'` ; done

cd ${ROOT}/__data.tmp__

FILES=`find . -type f | sort`

echo "Writing header..."
# the big space is a tab
ls -l $FILES | awk '{sum += $5} END {print sum}' > ${ROOT}/build/data.cpa

echo "Writing data..."
cat $FILES >> ${ROOT}/build/data.cpa

echo "Writing info..."
# sed removed "./" from beginning of filenames
ls -l $FILES | sed -e 's/.\///' | awk '{size = $5; name = $9} {printf "%d\t%s\n", size, name}' >> ${ROOT}/build/data.cpa

cd ${ROOT}

echo "Saving uncompressed archive..."
cp build/data.cpa build/data.cpa.uncompressed

echo "Compressing..."
gzip build/data.cpa
mv build/data.cpa.gz build/data.cpa

rm -rf __data.tmp__
