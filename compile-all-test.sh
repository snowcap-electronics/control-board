#!/bin/sh

set -e

CONFIGS="projects/*.mk"

for cfg in projects/*.mk
do

  make SC_PROJECT_CONFIG=$cfg clean
  echo "BUILDING PROJECT CONFIG $cfg"
  make SC_PEDANTIC_COMPILER=1 SC_PROJECT_CONFIG=$cfg -j4
done
