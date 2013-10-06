#!/bin/sh

set -e

PROJECTS="fps pleco test"
BOARDS="SC_SNOWCAP_V1 SC_F4_DISCOVERY SC_F1_DISCOVERY"

for project in $PROJECTS
do
  for board in $BOARDS
  do
    SC_PROJECT=$project SC_BOARD=$board make clean
	echo "BUILDING PROJECT $project ON BOARD $board"
    SC_PROJECT=$project SC_BOARD=$board make -j4
  done
done
