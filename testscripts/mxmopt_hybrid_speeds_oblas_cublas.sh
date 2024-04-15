#!/bin/bash

#################################################################

export LD_LIBRARY_PATH=/opt/openblas/lib:/home/ravi/Research/openh/openhinstall/lib64:$LD_LIBRARY_PATH

#################################################################

NARRAY=(10240 15360 20480 25600 30720)
NSTART=512
NSTEP=512
NRUNS=5

#################################################################

for N in ${NARRAY[@]}
do
    for (( N1=${NSTART}; N1 <= ${N}; N1+=NSTEP ));
    do
        for (( COUNT=1; COUNT<=${NRUNS}; COUNT++ ));
        do
            echo ../openhinstall/Apps/mxmopt_speeds_oblas_cublas ${N1} ${N} ${N} 0
            echo ../openhinstall/Apps/mxmopt_speeds_oblas_cublas ${N1} ${N} ${N} 0 >> speeds_${N}_${N1}.out
            ../openhinstall/Apps/mxmopt_speeds_oblas_cublas ${N1} ${N} ${N} 0 >> speeds_${N}_${N1}.out
        done

        GPU0SPEED=`cat speeds_${N}_${N1}.out | grep "GPU ID 0" | awk -F',' '{print $6}' | sed 's/ Speed(GFLOPs) //' | awk '{ sum += $1 } END { if (NR > 0) print sprintf("%.9f", sum / NR); }'`
        GPU1SPEED=`cat speeds_${N}_${N1}.out | grep "GPU ID 1" | awk -F',' '{print $6}' | sed 's/ Speed(GFLOPs) //' | awk '{ sum += $1 } END { if (NR > 0) print sprintf("%.9f", sum / NR); }'`
        CPU0SPEED=`cat speeds_${N}_${N1}.out | grep "CPU ID 0" | awk -F',' '{print $6}' | sed 's/ Speed(GFLOPs) //' | awk '{ sum += $1 } END { if (NR > 0) print sprintf("%.9f", sum / NR); }'`

        rm -f speeds_${N}_${N1}.out

        echo ${N1} ${GPU0SPEED} >> GPU0_${N}.speeds
        echo ${N1} ${GPU1SPEED} >> GPU1_${N}.speeds
        echo ${N1} ${CPU0SPEED} >> CPU0_${N}.speeds
    done
done

#################################################################

exit 0

################################################################
