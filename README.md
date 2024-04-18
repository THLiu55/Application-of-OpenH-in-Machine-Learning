---------------------------------------------------------------------
# Application of OpenH in training machine learning model in hybrid
**Training machine learning model (Random Forest) hybridly using `OpenH`**

`OpenH`: Programming and Execution Model for Applications on Heterogeneous Hybrid Platforms

To make `OpenH` more portable, here I utilized `hwloc` to detect CPU topology. 

---------------------------------------------------------------------

> OpenH Creators: Simon Farrelly and Ravi Reddy Manumachu

> Hybrid Application Creator: Tianhao Liu

---------------------------------------------------------------------

### To laod the dataset:

1. create a dir `data` in <Application/forest>

2. downlaod dataset `trainingdata.txt` from link: `https://drive.google.com/file/d/1yULAtKeRhIYNfz_aDwrIT66gtd70DDkc/view?usp=sharing`

   and the dataset   `testingdata.txt` from link: `https://drive.google.com/file/d/1dtv-7rdj10L2eZL5bTIhb45JgQxnXglz/view?usp=sharing`

### To build and install:

1. cd <hclaffinity root directory>

Replace '<hclaffinity root directory>' by the directory 
where hclaffinity is downloaded.

2. Set the OpenACC environment.

$ source setacc.sh

3. make

4. set the env:

    ~~~sh
    export LD_LIBRARY_PATH=<dir to project root>/openhinstall/lib64:$LD_LIBRARY_PATH
    ~~~

5. train the machine learning model:

    ~~~sh
    cd <dir to openhinstall/Apps>
    
    ./tree_hybrid -r ../Data/trainingdata.txt -t ../Data/testingdata.txt -d 784 -c 49 -s 10000 -m 10000 -p 0
    
    ./tree_cpu -r ../Data/trainingdata.txt -t ../Data/testingdata.txt -d 784 -c 49 -s 10000 -m 10000 -p 0 -n 10
    
    ./tree_gpu -r ../Data/trainingdata.txt -t ../Data/testingdata.txt -d 784 -c 49 -s 10000 -m 10000 -p 0 -n 10
    ~~~

The tests and applications are installed in the openhinstall directory.

---------------------------------------------------------------------
