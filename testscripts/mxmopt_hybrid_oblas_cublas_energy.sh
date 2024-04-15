#!/bin/bash

#################################################################

export LD_LIBRARY_PATH=/opt/openblas/lib:/home/ravi/Research/openh/openhinstall/lib64:$LD_LIBRARY_PATH

PROGRAM=$(basename $0)

#################################################################

WATTSUP1=/opt/powertools/bin/wattsup1
ENERGYTOOL=/home/ravi/Research/hclpowermeterscalibration/wattsupenergy

#################################################################

NRUNS=10
SRATIO=(11 7 6 4 4)

#################################################################

COUNT=0
for (( N=10240; N <= 30720; N+=5120 ));
do
    rm -f /var/lock/LCK..ttyUSB0
    $WATTSUP1 --interval=1 > wattsup1.data 2>&1 &
    wattsupid1=$!

    for (( COUNT=1; COUNT<=${NRUNS}; COUNT++ ));
    do
        ../openhinstall/Apps/mxmopt_hybrid_oblas_cublas ${N} ${SRATIO} 0
    done

    kill -9 $wattsupid1
    killall $WATTSUP1
    rm -rf HCLWattsUpData

    $ENERGYTOOL wattsup1.data 3 > N${N}.energy.res
    rm -f wattsup1.data

    TENERGYALL=`cat N${N}.energy.res | awk '{ print sprintf("%.9f", $4); }'`
    TENERGY=`echo "$TENERGYALL / $NRUNS" | bc -l`

    rm -f N${N}.energy.res

    echo "N=$N, OPENH HYBRID TotalEnergy(J)=$TENERGY" >> energy_openh.res

    let "COUNT = $COUNT + 1"
    
    sleep 120
done

#################################################################

exit 0

################################################################
