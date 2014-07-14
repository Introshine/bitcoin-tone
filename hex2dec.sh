#!/bin/bash

#Warning, after 10000 lines a \ will appear. this is buggy, workaround:
hex2dec() {
    BC_LINE_LENGTH=10000 bc <<< "ibase=16;$1"
}

dec2hex() {
    BC_LINE_LENGTH=10000 bc <<< "obase=16;$1"
}


#Convert to uppercase
hex=$1
hex="${hex^^}"


#Show decimal version of hex 
echo "###################### DECIMAL VERSION OF HEX #######################"
dec=$(hex2dec "$hex")
echo "$dec"

#Generate pseudo-checksum by doing a sum of all digits
echo "######################      CHECKSUM          #######################"
checksum=$(expr `echo $dec | sed 's/[0-9]/ + &/g' | sed 's/^ +//g'`)
echo "$checksum"

bittone="###$dec###$checksum***"

#
#Syntax of the bittone is  ###          digits       ###          checksum    ***
#                          ^three tones ^dtmf tones  ^three tones ^checksum   ^footer

#Generate wav file - using Phrack code
#./dial/dial --tone-time 150 --silent-time 100 --output-dev dtmf.u8 --use-audio 0 $bittone
./gen "$bittone"

#Convert old Unsigned 8bit to normal PCM Wave audio by using sox - swiss army knife
#8000Hz, 1 Channel, 8 Bit, Unsigned.

#works, but wav is ugly to hear......... oh well
sox -t raw -r 8000 -c 1 -b 8 -U devdsp.raw dtmf.wav

#does not work
#sox -t raw -r 8000 -c 1 -b 8 devdsp.raw dtmf.wav

#Sleep 1 second and take a drink
sleep 1

echo "* Now detecting..."

#detect
detect=$(./detect dtmf.wav)

#Header is first 3 chars (###)
HEADER=${detect:0:3}
#Footer is last 3 chars (***)
FOOTER=${detect: -3}
#Transaction
TRANS=${detect:3:-10}
#Checksum
CHECKSUM=${detect: -7}
#Buggy :( - hack
CHECKSUM=${CHECKSUM:0:-3}

if [ "$HEADER" = "###" ];
then
    echo "* Header [OK]: $HEADER"
else
    echo "* Header [CORRUPT]: $HEADER - Should be ###"
fi
if [ "$FOOTER" = "***" ];
then
    echo "* Footer [OK]: $FOOTER"
else
    echo "* Header [CORRUPT]: $FOOTER - Should be ***"
fi

#Uncomment to show transaction as debug
#echo "!!! Transaction: $TRANS !!!"

#Checksum 
SANE=$(expr `echo $TRANS | sed 's/[0-9]/ + &/g' | sed 's/^ +//g'`)

echo "* Checksum in DTMF is: $CHECKSUM"
echo "* Checksum of transaction is $SANE"

if [ "$CHECKSUM" = "$SANE" ];
then
    echo "* Checksum [PASSED]: $SANE"
    txid=$(dec2hex "$TRANS")
    echo "* TXID: "
    #include a trailing 0 - stripped off by bc but needed for hex/pushing it onto the network
    echo "0$txid"

else
    echo "* Checksum [FAILED]: should be $SANE but is $CHECKSUM. Check Wav/Sox/Sound quality/etc. Don't forget 10 to 50ms of silence between tones (or else 1223 wil be read as 123)!"
fi
