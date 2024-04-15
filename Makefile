##############################################################

COMPILER		=	nvc -O0
COMPILERFLAGS	=	-fpic -mp -acc
TESTFLAGS		=	${COMPILERFLAGS} -fast -Minfo=all

##############################################################

CPUFLAGS		=	-fast -Minfo=all -pthread -mp=multicore -fpic
GPUFLAGS		=	-fpic -mp=gpu -nomp -acc -fast -Minfo=all -pthread
HYBRIDFLAGS		=	-fpic -mp -acc -fast -Minfo=all -pthread
HYBRIDFLAGS2	=	-fpic -mp -acc -fast -Minfo=all -pthread -cudalib=cublas
FFTHYBRIDFLAGS	=	-fpic -mp -acc -fast -Minfo=all -pthread -Mmkl -cudalib=cufft

TREESEQFLAGS	=	-Minfo=all -pthread 
TREECPUFLAGS	=	-fopenmp
TREEGPUFLAGS	=	-acc -Minfo=all -fast -lpthread
TREEHYBRIDFLAGS	=	-fpic -mp -acc -fast -Minfo=all -pthread

##############################################################

GCCCOMPILER		=	gcc
GCCFLAGS		=	-fpic -fopenmp

##############################################################

LIBINC			=	-Iinclude

##############################################################

LIBHEADERS		=	openh.h

LIBSOURCES		=	openh.c \
					openherr.c \
					openhhelpers.c \
					openhversion.c
LIBSRCDIR       =   src
LIBOBJDIR       =   objs
LIBOBJS			=   $(patsubst %.c, $(LIBOBJDIR)/%.c.o, $(LIBSOURCES))
LIBHCLAFFINITY	=   libopenh.so
LIBHCLAFFINITYL	=   -lopenh
LIBFLAGS		=	-shared

OBLASDIR		=	/opt/openblas
LIBOBLASDIR		=	${OBLASDIR}/lib
LIBOBLAS		=	-lopenblas

MKLINCLUDE		=	${MKLROOT}/include/fftw

CUDAHOME		=	/usr/local/cuda
CUDAINC			=	${CUDAHOME}/include
CUDALIBDIR		=	${CUDAHOME}/lib64
CUDALIBS		=	-lcublas -lcublasLt
CUDAFFTLIBS		=	-lcufft

LIBHWLOC   		=   -lhwloc

##############################################################

TESTDIR			=	tests
TESTS			=	testopenhprint \
					testopenhenv testmaster testompbindclose \
					testompbindmaster testpcpuids testlcpuids \
					testgetacc testaccaffinity testpcompbind \
					testpcompbind2 testtokenizer \
					testht testnpcores testnlcores

##############################################################
#  MXM APPS
##############################################################

MXMAPPDIR		=	Applications/mxm

MXMCPUAPPS		=	mxmnaive_cpu

MXMGPUAPPS		=	mxmnaive_1gpu \
					mxmnaive_2gpu

MXMHYBRIDAPPS	=	mxmnaive_hybrid \
					mxmnaive_speeds

MXMOBLASAPPS	=	mxmopt_oblas_cpu \
					mxmopt_oblas_cpu2

MXMCUBLASAPPS	=	mxmopt_hybrid_oblas_cublas \
					mxmopt_hybrid_cublas \
					mxmopt_speeds_oblas_cublas \
					mxmopt_cublas_1gpu

MXMAPPS			=	${MXMCPUAPPS} \
					${MXMGPUAPPS} \
					${MXMHYBRIDAPPS} \
					${MXMOBLASAPPS} \
					${MXMCUBLASAPPS}

##############################################################
#  FFT APPS
##############################################################

FFTAPPDIR		=	Applications/fft

FFTCPUSRCS		=	${FFTAPPDIR}/transpose.c ${FFTAPPDIR}/cpufft.c
FFTGPUSRCS		=	${FFTAPPDIR}/gpufft.c
FFTAPPS			=	hfft2d

##############################################################
#  TREE APPS
##############################################################
FORESTAPPDIR	=	Applications/forest

FORESTUTILSRCS	=	${FORESTAPPDIR}/src/utils.c ${FORESTAPPDIR}/src/loadDataset.c
FORESTSEQSRCS	=	${FORESTUTILSRCS} ${FORESTAPPDIR}/src/tree_seq.c
FORESTCPUSRCS	=	${FORESTUTILSRCS} ${FORESTAPPDIR}/src/tree_cpu.c
FORESTGPUSRCS	=	${FORESTUTILSRCS} ${FORESTAPPDIR}/src/tree_gpu.c
FORESTHYBRIDSRCS =	${FORESTUTILSRCS} ${FORESTAPPDIR}/src/tree_hybrid.c

FORESTSEQAPP	=	tree_seq
FORESTCPUAPP	=	tree_cpu
FORESTGPUAPP	=	tree_gpu
FORESTHYBRIDAPP	=	tree_hybrid

##############################################################

# Notes OMP nested parallelism
# does not work with NVC compiler
OMPTESTS		= 	testompnestedspreadclose \
					testompnestedspreadall

##############################################################

INSTALLDIR		=	openhinstall
INSTALLDIRLIB	=	${INSTALLDIR}/lib64
INSTALLDIRINC	=	${INSTALLDIR}/include
INSTALLDIRTESTS	=	${INSTALLDIR}/tests
INSTALLDIRAPPS 	=	${INSTALLDIR}/Apps
INSTALLDIRDATA 	=	${INSTALLDIR}/Data

##############################################################

all: mkobjdir mkinstalldir cpdata\
	${LIBHCLAFFINITY} ${TESTS} ${OMPTESTS} \
	${MXMAPPS} ${FFTAPPS} \
	${FORESTGPUAPP} ${FORESTCPUAPP} ${FORESTSEQAPP} ${FORESTHYBRIDAPP}

mkobjdir:
	mkdir -p ${LIBOBJDIR}

mkinstalldir:
	mkdir -p ${INSTALLDIR}
	mkdir -p ${INSTALLDIRLIB}
	mkdir -p ${INSTALLDIRINC}
	mkdir -p ${INSTALLDIRTESTS}
	mkdir -p ${INSTALLDIRAPPS}
	mkdir -p ${INSTALLDIRDATA}

cpdata:
	cp Applications/forest/data/* ${INSTALLDIRDATA}/

clean:
	rm -rf ${LIBOBJDIR} ${INSTALLDIR} *.o

${LIBHCLAFFINITY}: ${LIBOBJS}
	${COMPILER} ${LIBFLAGS} -o ${INSTALLDIRLIB}/$@ $^
	(cd include && cp ${LIBHEADERS} ../${INSTALLDIRINC}/)

$(LIBOBJDIR)/%.c.o: $(LIBSRCDIR)/%.c
	$(COMPILER) $(COMPILERFLAGS) ${LIBINC} ${LIBHWLOC} -o $@ -c $<

${TESTS}:
	$(COMPILER) $(TESTFLAGS) ${LIBINC} \
		-o ${INSTALLDIRTESTS}/$@ ${TESTDIR}/$@.c \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL}

${OMPTESTS}:
	$(GCCCOMPILER) $(GCCFLAGS) \
		-o ${INSTALLDIRTESTS}/$@ ${TESTDIR}/$@.c

${MXMCPUAPPS}:
	$(COMPILER) $(CPUFLAGS) ${LIBINC} \
		-o ${INSTALLDIRAPPS}/$@ ${MXMAPPDIR}/$@.c \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL}

${MXMGPUAPPS}:
	$(COMPILER) $(GPUFLAGS) ${LIBINC} \
		-o ${INSTALLDIRAPPS}/$@ ${MXMAPPDIR}/$@.c \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL}

${MXMHYBRIDAPPS}:
	$(COMPILER) $(HYBRIDFLAGS) ${LIBINC} \
		-o ${INSTALLDIRAPPS}/$@ ${MXMAPPDIR}/$@.c \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL}

${MXMCUBLASAPPS}:
	$(COMPILER) $(HYBRIDFLAGS2) ${LIBINC} -I${OBLASDIR}/include -I${CUDAINC} \
		-o ${INSTALLDIRAPPS}/$@ ${MXMAPPDIR}/$@.c \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL} \
		-L${LIBOBLASDIR} ${LIBOBLAS} \
		-L${CUDALIBDIR} ${CUDALIBS}

${MXMOBLASAPPS}:
	$(COMPILER) $(HYBRIDFLAGS) ${LIBINC} -I${OBLASDIR}/include \
		-o ${INSTALLDIRAPPS}/$@ ${MXMAPPDIR}/$@.c \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL} \
		-L${LIBOBLASDIR} ${LIBOBLAS}

${FFTAPPS}:
	$(COMPILER) $(FFTHYBRIDFLAGS) ${LIBINC} -I${MKLINCLUDE} -I${CUDAINC} \
		-o ${INSTALLDIRAPPS}/$@ ${FFTAPPDIR}/$@.c ${FFTCPUSRCS} ${FFTGPUSRCS} \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL} \
		-L${CUDALIBDIR} ${CUDAFFTLIBS}

${FORESTSEQAPP}:
	$(COMPILER) $(TREESEQFLAGS) ${LIBINC} \
		-o ${INSTALLDIRAPPS}/$@ ${FORESTSEQSRCS} \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL}

${FORESTCPUAPP}:
	$(COMPILER) $(TREECPUFLAGS) ${LIBINC} \
		-o ${INSTALLDIRAPPS}/$@ ${FORESTCPUSRCS} \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL}

${FORESTGPUAPP}:
	$(COMPILER) $(TREEGPUFLAGS) ${LIBINC}\
		-o ${INSTALLDIRAPPS}/$@ ${FORESTGPUSRCS} \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL} \

${FORESTHYBRIDAPP}:
	$(COMPILER) $(TREEHYBRIDFLAGS) ${LIBINC}\
		-o ${INSTALLDIRAPPS}/$@ ${FORESTHYBRIDSRCS} \
		-L${INSTALLDIRLIB} ${LIBHWLOC} ${LIBHCLAFFINITYL} \


##############################################################
