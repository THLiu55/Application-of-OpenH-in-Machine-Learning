#!/bin/sh

HOSTNAME=$(hostname)
if [ "${HOSTNAME}" = "hclserver02.ucd.ie" ]; then
   export CUDA_HOME=/home/atefeh/nvc/nvc_install/Linux_x86_64/22.3/cuda/11.6/
   export CUDAHOME=${CUDA_HOME}
   export NVHPC_CUDA_HOME=${CUDA_HOME}
   export PATH=/home/atefeh/nvc/nvc_install/Linux_x86_64/22.3/compilers/bin:$PATH
   export LD_LIBRARY_PATH=/home/atefeh/nvc/nvc_install/Linux_x86_64/22.3/compilers/lib:$LD_LIBRARY_PATH
elif [ "${HOSTNAME}" = "hclserver2a40" ]; then
   export CUDA_HOME=/opt/nvidia/hpc_sdk/Linux_x86_64/2022/cuda/11.8/
   export CUDAHOME=${CUDA_HOME}
   export NVHPC_CUDA_HOME=${CUDA_HOME}
   export PATH=${CUDAHOME}/bin:$PATH
   export LD_LIBRARY_PATH=${CUDAHOME}/lib64:$LD_LIBRARY_PATH
else
   echo "No nvc for ${HOSTNAME$}"
fi
