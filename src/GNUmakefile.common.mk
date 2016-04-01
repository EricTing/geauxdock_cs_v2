
TARGET_GPU := 0
TARGET_CPU := 1
TARGET_MIC := 2

MARCRO_TARGET += -DTARGET_GPU=$(TARGET_GPU) -DTARGET_CPU=$(TARGET_CPU) -DTARGET_MIC=$(TARGET_MIC)
#MARCRO_TARGET += -DTARGET_DEVICE=$(TARGET_CPU)
#MARCRO_TARGET += -DTARGET_DEVICE=$(TARGET_MIC)
#MARCRO_TARGET += -DTARGET_DEVICE=$(TARGET_GPU)





HOST := smic

ifeq ($(HOST), smic)
# module purge
# module load impi/4.1.3.048/intel64
# module load intel/14.0.2
# module load cuda/6.5
	COMPILER_HOST := intel
	#COMPILER_HOST := gnu
	LIBPATH += -L/usr/local/packages/cuda/6.5/lib64
	HEADPATH += -I/usr/local/packages/cuda/6.5/include
	GPU := K20XM
	NGPU := 1

else ifeq ($(HOST), shelob)
	COMPILER_HOST := gnu
	GPU := K20XM
	NGPU := 2

else ifeq ($(HOST), ece)
	COMPILER_HOST := gnu
	GPU := GTX780
	NGPU := 1

else ifeq ($(HOST), lasphi)
	COMPILER_HOST := gnu
	GPU := GTX980
	NGPU := 1
endif





# CUDA execution configurations
#
# it can be generated by the runtime
# but
# it is also required at compiling time for optimizing launch bounds


# 2D thread block size restrictions:
# BDX = 32, 64, 128, ... 1024
# BDY <= BDx
# BDY * BDX > MAXWEI
# BDX * BDY <= 1024   (always true)
# BDX <= warpSize     (always true)


ifeq ($(GPU), GTX780)
	CXXFLAGS_DEV += -gencode arch=compute_35,code=sm_35
	MC_BperMP := 2
	GD := 24
	BD := 512
else ifeq ($(GPU), K20XM)
	CXXFLAGS_DEV += -gencode arch=compute_35,code=sm_35
	MC_BperMP := 2
	GD := 28
	BD := 512

#	MC_BperMP := 1
#	GD := 14
#	BD := 1024

#	MC_BperMP := 4
#	GD := 56
#	BD := 256
else ifeq ($(GPU), GTX980)
	CXXFLAGS_DEV += -gencode arch=compute_35,code=sm_52
	MC_BperMP := 4
	GD := 64
#	BD := 320
	BD := 256
else
	CXXFLAGS_DEV += -arch=compute_35
	MC_BperMP := 2
	GD := 16
	BD := 512
endif


# CUDA warp parameters
#WARP_SZ := 32
# Threads Per Block
#WARPperB := $(shell echo $(BD)/$(WARP_SZ) | bc)
# rounding up to nearest power of 2
#BD_POWER2 := $(shell echo "from math import log; a=2**int(log($(BD),2)); print a" | python)


MARCRO_GPU += -DMC_BperMP=$(MC_BperMP)
MARCRO_GPU += -DGD=$(GD)
MARCRO_GPU += -DBD=$(BD)
MARCRO_GPU += -DNGPU=$(NGPU)
MARCRO_GPU += -DENABLE_CUDA_LDG







ifeq ($(COMPILER_HOST), intel)
	CXX_MPI := mpiicpc
	CXX_HOST := icpc
	FC_HOST := ifort
	CXX_DEV := nvcc
else ifeq ($(COMPILER_HOST), gnu)
	CXX_MPI := mpicxx
	CXX_HOST := g++
	FC_HOST := gfortran
	CXX_DEV := nvcc
else ifeq ($(COMPILER_HOST), customized1)
	CXX_MPI := mpicxx
	CXX_HOST := g++
	FC_HOST := gfortran
	CXX_DEV := nvcc
endif





INTEL_REPORT_FLAGS += -vec-report
INTEL_REPORT_FLAGS += -par-report
INTEL_REPORT_FLAGS += -opt-report=3
INTEL_REPORT_FLAGS += -opt-report-phase=all
INTEL_REPORT_FLAGS += -opt-report-file=tmp/optrpt.txt




HEADPATH += -I. -I../common -I../frontend -I../backend_gpu -I../backend_cpu_mic -I../analysis

#LIBPATH +=
LIBPATH += #-Wl,-rpath=/usr/local/compilers/Intel/cluster_studio_xe_2013.1.046/composer_xe_2013_sp1.2.144/compiler/lib/mic

LINKFLAGS += -lm -lrt -lyeahc
#LINKFLAGS += -lpapi
ifeq ($(COMPILER_HOST), intel)
	LINKFLAGS += -openmp
#	LINKFLAGS += $(INTEL_REPORT_FLAGS)
else
	LINKFLAGS += -fopenmp
endif





FCFLAGS_HOST += -O3



CXXFLAGS_HOST += $(HEADPATH) $(MARCRO_MAKE)
CXXFLAGS_HOST += -Wall
ifeq ($(COMPILER_HOST), intel)
	CXXFLAGS_HOST += -O3
#	CXXFLAGS_HOST += -fast
#	CXXFLAGS_HOST += -fma
	CXXFLAGS_HOST += -ip -ipo
#	CXXFLAGS_HOST += -xAVX
#	CXXFLAGS_HOST += -xSSE4.2
#	CXXFLAGS_HOST += -no-vec -DNOVEC
#	CXXFLAGS_HOST += -vec-guard-write
#	CXXFLAGS_HOST += -no-simd # ignore SIMD pragmas, slow down 2%
#	CXXFLAGS_HOST += -vec-threshold0   # should consistantly turn this off thoughout the benchmarks
#	CXXFLAGS_HOST += -fp-model source
	CXXFLAGS_HOST += -fno-fnalias -ansi-alias -fargument-noalias
#	CXXFLAGS_HOST += -mavx
	CXXFLAGS_HOST += -openmp
else
	CXXFLAGS_HOST += -O3
	CXXFLAGS_HOST += -fopenmp
endif



CXXFLAGS_DEV += $(HEADPATH) $(MARCRO_MAKE) $(MARCRO_GPU)
CXXFLAGS_DEV += -O3
CXXFLAGS_DEV += -use_fast_math
CXXFLAGS_DEV += -Xptxas -dlcm=ca

# for development purpose, making performance worse
#CXXFLAGS_DEV += -lineinfo
XXFLAGS_DEV += -Xptxas -v
#CXXFLAGS_DEV += -keep --keep-dir tmp -G --source-in-pt



