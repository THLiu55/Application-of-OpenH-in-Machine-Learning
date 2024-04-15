#!/bin/bash

#################################################################

export LD_LIBRARY_PATH=/opt/openblas/lib:/home/ravi/Research/openh/openhinstall/lib64:$LD_LIBRARY_PATH

#################################################################

WATTSUP1=/opt/powertools/bin/wattsup1
ENERGYTOOL=/home/ravi/Research/hclpowermeterscalibration/wattsupenergy

#################################################################

#NARRAY=(10240 15360 20480 25600 30720)
NARRAY=(30720)
NSTART=512
NSTEP=512
BASEPOWER=146
NRUNSCPU=10
NRUNSGPU=10

#################################################################

for N in ${NARRAY[@]}
do
    for (( N1=${NSTART}; N1 <= ${N}; N1+=NSTEP ));
    do
        #######################################################
        #              CPU
        #######################################################
        rm -f /var/lock/LCK..ttyUSB0
        $WATTSUP1 --interval=1 > wattsup1.data 2>&1 &
        wattsupid1=$!

        for (( COUNT=1; COUNT<=${NRUNSCPU}; COUNT++ ));
        do
            echo ../openhinstall/Apps/mxmopt_oblas_cpu2 ${N1} ${N} ${N} 0
            echo ../openhinstall/Apps/mxmopt_oblas_cpu2 ${N1} ${N} ${N} 0 >> energies_${N}_${N1}.out
            ../openhinstall/Apps/mxmopt_oblas_cpu2 ${N1} ${N} ${N} 0 >> energies_${N}_${N1}.out
        done

        kill -9 $wattsupid1
        killall $WATTSUP1
        rm -rf HCLWattsUpData

        $ENERGYTOOL wattsup1.data 3 > N${N}.energy.res
        rm -f wattsup1.data

        ETIME=`cat energies_${N}_${N1}.out | grep "MXM OBLAS" | awk -F',' '{print $5}' | awk '{ sum += $3 } END { if (NR > 0) print sprintf("%.9f", sum / NR); }'`
        TENERGYALL=`cat N${N}.energy.res | awk '{ print sprintf("%.9f", $4); }'`
        TENERGY=`echo "$TENERGYALL / $NRUNSCPU" | bc -l`
        DENERGY=`echo "$TENERGY - $BASEPOWER * $ETIME" | bc -l`

        rm -f energies_${N}_${N1}.out
        rm -f N${N}.energy.res

        echo ${N1} ${ETIME} ${TENERGY} ${DENERGY} >> CPU_${N}.energies

        sleep 180

        #######################################################
        #              GPU0
        #######################################################
        rm -f /var/lock/LCK..ttyUSB0
        $WATTSUP1 --interval=1 > wattsup1.data 2>&1 &
        wattsupid1=$!

        for (( COUNT=1; COUNT<=${NRUNSGPU}; COUNT++ ));
        do
            echo ../openhinstall/Apps/mxmopt_cublas_1gpu ${N1} ${N} ${N} 0 0
            echo ../openhinstall/Apps/mxmopt_cublas_1gpu ${N1} ${N} ${N} 0 0 >> energies_${N}_${N1}.out
            ../openhinstall/Apps/mxmopt_cublas_1gpu ${N1} ${N} ${N} 0 0 >> energies_${N}_${N1}.out
        done

        kill -9 $wattsupid1
        killall $WATTSUP1
        rm -rf HCLWattsUpData

        $ENERGYTOOL wattsup1.data 3 > N${N}.energy.res
        rm -f wattsup1.data

        ETIME=`cat energies_${N}_${N1}.out | grep "MXM CUBLAS" | awk -F',' '{print $5}' | awk '{ sum += $3 } END { if (NR > 0) print sprintf("%.9f", sum / NR); }'`
        TENERGYALL=`cat N${N}.energy.res | awk '{ print sprintf("%.9f", $4); }'`
        TENERGY=`echo "$TENERGYALL / $NRUNSGPU" | bc -l`
        DENERGY=`echo "$TENERGY - $BASEPOWER * $ETIME" | bc -l`

        rm -f energies_${N}_${N1}.out
        rm -f N${N}.energy.res

        echo ${N1} ${ETIME} ${TENERGY} ${DENERGY} >> GPU0_${N}.energies

        sleep 180

        #######################################################
        #              GPU1
        #######################################################
        rm -f /var/lock/LCK..ttyUSB0
        $WATTSUP1 --interval=1 > wattsup1.data 2>&1 &
        wattsupid1=$!

        for (( COUNT=1; COUNT<=${NRUNSGPU}; COUNT++ ));
        do
            echo ../openhinstall/Apps/mxmopt_cublas_1gpu ${N1} ${N} ${N} 1 0
            echo ../openhinstall/Apps/mxmopt_cublas_1gpu ${N1} ${N} ${N} 1 0 >> energies_${N}_${N1}.out
            ../openhinstall/Apps/mxmopt_cublas_1gpu ${N1} ${N} ${N} 1 0 >> energies_${N}_${N1}.out
        done

        kill -9 $wattsupid1
        killall $WATTSUP1
        rm -rf HCLWattsUpData

        $ENERGYTOOL wattsup1.data 3 > N${N}.energy.res
        rm -f wattsup1.data

        ETIME=`cat energies_${N}_${N1}.out | grep "MXM CUBLAS" | awk -F',' '{print $5}' | awk '{ sum += $3 } END { if (NR > 0) print sprintf("%.9f", sum / NR); }'`
        TENERGYALL=`cat N${N}.energy.res | awk '{ print sprintf("%.9f", $4); }'`
        TENERGY=`echo "$TENERGYALL / $NRUNSGPU" | bc -l`
        DENERGY=`echo "$TENERGY - $BASEPOWER * $ETIME" | bc -l`

        rm -f energies_${N}_${N1}.out
        rm -f N${N}.energy.res

        echo ${N1} ${ETIME} ${TENERGY} ${DENERGY} >> GPU1_${N}.energies

        sleep 180

    done
done

#################################################################

exit 0

################################################################
