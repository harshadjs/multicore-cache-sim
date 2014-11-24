#!/usr/bin/python
import subprocess
import os

## Set these variables and you should be good!
parsec_path="/mnt/test/parsec_2.1/parsec-2.1"
## /home/harshad/tools/benchmarks/parsec/parsec-3.0/"
simulator_path="/home/harshad/CMU/15740_research/simulator/"
pin_path="/home/harshad/tools/pin/"

benchmarks=[ "fluidanimate", "blackscholes", "canneal", "swaptions", "x264" ]
# benchmarks=[ "facesim", "blackscholes", "bodytrack", "ferret", \
#              "ferret", "fluidanimate", "freqmine", "raytrace", \
#              "swaptions", "vips", "x264" ]

## Install all the benchmarks
def install_benchmarks():
    for bm in benchmarks:
        command = "sudo %s/bin/parsecmgmt -a build -p %s" % \
                  (parsec_path, bm)
        os.system(command)



# print "Installing benchmarks"
# install_benchmarks()

for bm in benchmarks:
    # command = "sudo %s/bin/parsecmgmt -n 2 -s \"%s/pin -pin_memory_range 0x30000000:0x50000000\
    #           -t %s/bin/simulator.so --\" \
    #           -a run -p %s" % (parsec_path, pin_path, simulator_path, bm)
    command = "sudo %s/bin/parsecmgmt -n 2 -s \"%s/pin\
              -t %s/bin/simulator.so --\" \
              -a run -p %s" % (parsec_path, pin_path, simulator_path, bm)

    os.system(command)

## Generate graphs
command = simulator_path + "/plot/plot.sh"
os.environ['SIMULATOR_HOME'] = simulator_path
os.system(command)