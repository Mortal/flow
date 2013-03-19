#!/bin/sh
seed=`date +%N%s`
echo Producing bg.$seed.pgm
./height --size 1920x1080 --seed $seed --roughness 3 --pgm > bg.$seed.pgm
echo Producing bglight.$seed.pgm
./height --size 1920x1080 --seed $seed --roughness 3 --pgm --light 5.0 > bglight.$seed.pgm
echo Producing composite.$seed.png
convert \( bg.$seed.pgm \( xc:white xc:green xc:green xc:PaleGreen3 xc:DodgerBlue \
                       xc:DodgerBlue xc:DodgerBlue xc:DodgerBlue +append \) \
             -clut \) \
          \( bg.$seed.pgm bglight.$seed.pgm -compose screen -composite \) \
          -compose multiply -composite composite.$seed.png
echo Remove temporaries
rm -f bg.$seed.pgm bglight.$seed.pgm
