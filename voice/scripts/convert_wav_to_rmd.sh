#! /bin/sh
# Convert from WAV to RMD format, including stereo.
# $Id: convert_wav_to_rmd.sh,v 1.1 1999/01/23 15:17:10 marcs Exp $

if [ $# != 1 ]; then
   echo "$0: bad args."
   exit 2
fi

SOX=~/ported/sox10p11/sox
PVFTOOLS=/usr/lib/mgetty/pvftools/
SPEED=11025
MODEM_BRAND=ZyXEL_2864
MODEM_BRAND_RESOLUTION=4

FNAME=$1
TMP_FNAME=/tmp/convert_$$

# Merge into one channel (maybe that could be done merged)
$SOX -t wav $FNAME -t wav -c 1 $TMP_FNAME

$PVFTOOLS/wavtopvf < $TMP_FNAME | $PVFTOOLS/pvfspeed -s $SPEED | $PVFTOOLS/pvftormd $MODEM_BRAND $MODEM_BRAND_RESOLUTION > $FNAME.rmd

rm -f $TMP_FNAME
