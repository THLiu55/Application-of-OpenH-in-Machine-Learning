---------------------------------------------------------------------
# Final Year Project
Training machine learning model (Random Forest) hybridly using `OpenH`

`OpenH`: Programming and Execution Model for Applications on Heterogeneous Hybrid Platforms

---------------------------------------------------------------------

Hybrid Application Creator: Tianhao Liu

OpenH Creators: Simon Farrelly and Ravi Reddy Manumachu

---------------------------------------------------------------------

To build and install:

1). cd <hclaffinity root directory>

Replace '<hclaffinity root directory>' by the directory 
where hclaffinity is downloaded.

2). Set the OpenACC environment.

$ source setacc.sh

3). make

4). set the env:
    ~~~sh
    export LD_LIBRARY_PATH=<dir to project root>/openhinstall/lib64:$LD_LIBRARY_PATH
    ~~~

5). train the machine learning model:

    ~~~sh
    ./tree_hybrid -r ../Data/trainingdata.txt -t ../Data/testingdata.txt -d 784 -c 49 -s 10000 -m 10000 -p 0
    ~~~

    ~~~sh
    ./tree_cpu -r ../Data/trainingdata.txt -t ../Data/testingdata.txt -d 784 -c 49 -s 10000 -m 10000 -p 0 -n 10
    ~~~

    ~~~sh
    ./tree_gpu -r ../Data/trainingdata.txt -t ../Data/testingdata.txt -d 784 -c 49 -s 10000 -m 10000 -p 0 -n 10
    ~~~

The tests and applications are installed in the openhinstall directory.

---------------------------------------------------------------------
