#!/bin/dash

set -e

CONFIGS="projects/*.mk"
CORECOUNT=$(nproc)

# Spirit1 apps need encryption key and device address
if [ ! -f spirit1_key.h ]
then
	cat <<EOF > spirit1_key.h
#define TOP_SECRET_KEY          ((uint8_t*)"_TOP_SECRET_KEY_")
#define MY_ADDRESS              0x01
EOF
fi

for cfg in projects/*.mk
do

  echo "BUILDING PROJECT CONFIG $cfg"
  make SC_PROJECT_CONFIG=$cfg clean
  make SC_PEDANTIC_COMPILER=1 SC_PROJECT_CONFIG=$cfg -j$CORECOUNT

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
