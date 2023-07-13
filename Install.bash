#!bin/bash

clear

if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root. Terminated."
   exit 1
fi

if [ ! -d "DRIVERS/PLX" ]; then
  echo "There is no DRIVERS in $(pwd)!";
  exit -1
fi


#at first let's update all involved repositories
printf " == This script is intended for installing JSC \"Rudshel\" drivers == \n\n"

echo "Do you want to install drivers?"
read -p "Type 'Y' or 'y' if yes or any other button otherwise. " -n 1 -r
printf "\n\n"  # move to a new line
if [[ $REPLY =~ ^[Yy]$ ]]; then
  printf "=================== The job has begun! ===================\n\n"
else
  printf "No drivers have been installed!\n\n"
  exit -1
fi



BASE_DIR=$(pwd)
_PLX_SDK_DIR_="$BASE_DIR/DRIVERS/PLX"
USB_DRIVER_DIR="$BASE_DIR/DRIVERS/USB"

#echo "PLX driver directory = $_PLX_SDK_DIR_"

export PLX_SDK_DIR=$_PLX_SDK_DIR_

cd $_PLX_SDK_DIR_/Driver



if [ -f "$_PLX_SDK_DIR_/Bin/Plx9054.ko" ]; then
  rm $_PLX_SDK_DIR_/Bin/Plx9054.ko 
fi

if [ -f "$_PLX_SDK_DIR_/Bin/Plx9050.ko" ]; then
  rm $_PLX_SDK_DIR_/Bin/Plx9050.ko 
fi

if [ -f "$_PLX_SDK_DIR_/Bin/Plx8311.ko" ]; then
  rm $_PLX_SDK_DIR_/Bin/Plx8311.ko 
fi



printf "\n%s" "compiling PLX9054 . . . "
bash builddriver 9054 &> /dev/null
cp $_PLX_SDK_DIR_/Driver/Plx9054/Plx9054.ko $_PLX_SDK_DIR_/Bin/Plx9054.ko
if [ -f "$_PLX_SDK_DIR_/Bin/Plx9054.ko" ]; then
  printf "success"
else
  printf "failed"
fi

printf "\n%s" "compiling PLX9050 . . . "
bash builddriver 9050 &> /dev/null
cp $_PLX_SDK_DIR_/Driver/Plx9050/Plx9050.ko $_PLX_SDK_DIR_/Bin/Plx9050.ko
if [ -f "$_PLX_SDK_DIR_/Bin/Plx9050.ko" ]; then
  printf "success"
else
  printf "failed"
fi

printf "\n%s" "compiling PLX8311 . . . "
bash builddriver 8311 &> /dev/null
cp $_PLX_SDK_DIR_/Driver/Plx8311/Plx8311.ko $_PLX_SDK_DIR_/Bin/Plx8311.ko
if [ -f "$_PLX_SDK_DIR_/Bin/Plx8311.ko" ]; then
  printf "success"
else
  printf "failed"
fi

cd $_PLX_SDK_DIR_/PlxApi 
make &> /dev/null 
cp $_PLX_SDK_DIR_/PlxApi/Library/PlxApi.a $_PLX_SDK_DIR_/Bin/libPlxApi.a && rm -r $_PLX_SDK_DIR_/PlxApi/Library


if [ -d "/var/local/plx" ]; then
  rm -r /var/local/plx
fi

printf "\n%s\n" "Installing Plx driver . . . "
cd $_PLX_SDK_DIR_/InstallDriver && bash install
cd $_PLX_SDK_DIR_/Driver && rm -r Plx*


printf "\n\n%s" "Compiling USB driver . . . "
cd $USB_DRIVER_DIR/LAUSB
make &> /dev/null
if [ -f "$USB_DRIVER_DIR/LAUSB/lausb.ko" ]; then
  printf "success"
  cd $USB_DRIVER_DIR/USBInstallDriver
  bash install
else
  printf "failed"
fi

cd $BASE_DIR


if [ -d "/usr/lib/RSH" ]; then
  rm -r /usr/lib/RSH
fi

printf "\n\nCopying libraries . . .\n"
mkdir /usr/lib/RSH
cp -avr $BASE_DIR/boards /usr/lib/RSH
cp -avr $BASE_DIR/libraries /usr/lib/RSH
cp -avr $BASE_DIR/DPA /usr/lib/RSH

printf "\nInstall dependencies . . .\n"

while read line; do
  FILE_NAME=$(echo "$line" | cut -d " " -f1)
  FILE_PATH=$(echo "$line" | cut -d " " -f2)
  if [ -f "$FILE_PATH" ]; then
    printf "$FILE_NAME exists\n"
  else
    cp "$BASE_DIR/dependencies/$FILE_NAME" "$FILE_PATH"
    printf "installed $FILE_PATH\n"
  fi
done < "$BASE_DIR/dependencies/dep.txt"



printf "\n\nThe job has been finished! ---[ Please restart your computer! ]--- \n\n"