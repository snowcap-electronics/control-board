#!/bin/dash

set -e

CONFIGS="projects/*.mk"

for cfg in projects/*.mk
do

  make SC_PROJECT_CONFIG=$cfg clean
  echo "BUILDING PROJECT CONFIG $cfg"
  make SC_PEDANTIC_COMPILER=1 SC_PROJECT_CONFIG=$cfg -j4

  if [ "$1" = "store" ]
  then
    config_name=`basename $cfg .mk`
    rm -rf out/$config_name
    mkdir -p out/$config_name
    cp build/*bin out/$config_name/
    cp build/*dmp out/$config_name/
    cp build/*elf out/$config_name/
    cp build/*hex out/$config_name/
    cp build/*map out/$config_name/
  fi
done
