#!/bin/sh

./1_mklist.py \
    ../../data/astex/proteins/1a07C.pdb \
    ../../data/astex/ligands \
    ../../data/parameters/08ff_opt \
    ../../data/parameters/paras \
    ../../data/parameters/08_nor_a \
    ../../data/parameters/08_nor_b \
    1 \
    0.044 \
    0.036 \
    0.02 \
    0.08 \
    1 \
    > a1.csv


# prt_fn
# lig_dir
# weight_fn
# enepara_fn
# nora_fn
# norb_fn
# n_temp
# temp_high
# temp_low
# tras_scale
# rot_scale
# n_dump       total monte carlo steps = steps_per_dump * n_dump


