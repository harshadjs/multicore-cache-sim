#!/usr/bin/python
import subprocess
import os
import sys

from itertools import combinations

## Set these variables and you should be good!

parsec_path="/mnt/test/parsec_2.1/parsec-2.1"
#parsec_path="/tmp/parsec-2.1/"
#/home/harshad/tools/benchmarks/parsec/parsec-3.0/
simulator_path="/home/harshad/CMU/15740_research/simulator/"
pin_path="/home/harshad/tools/pin/"
results_path="%s/results" % simulator_path
#simulator_path="/afs/andrew.cmu.edu/usr11/tloffred/Coursework/Arch/gitproj/"
#pin_path="/afs/andrew.cmu.edu/usr11/tloffred/Coursework/Arch/proj/pin-2.14"

benchmarks=[ "fluidanimate", "blackscholes" ]
#benchmarks=[ "fluidanimate", "blackscholes", "canneal", "swaptions" ]
#benchmarks=[ "blackscholes" ]
#benchmarks=[ "canneal" ]
#benchmarks=[  "x264" ]
# benchmarks=[ "facesim", "blackscholes", "bodytrack", "ferret", \
#              "ferret", "fluidanimate", "freqmine", "raytrace", \
#              "swaptions", "vips", "x264" ]

# fluidanimate: 55% (80%)
# blackscholes: 40% (90%)
# canneal: 98% (99%)
# swaptions: 35% (80%)
# x264: 60% (80%)

conf_params = [ "OPTIMIZ_HEAP", "OPTIMIZ_RDONLY_COHERENCE", "OPTIMIZ_UPDATE_PROTOCOL" ]

## Install all the benchmarks
def install_benchmarks():
    for bm in benchmarks:
        command = "%s/bin/parsecmgmt -a build -p %s" % \
                  (parsec_path, bm)
        os.system(command)


# print "Installing benchmarks"
# install_benchmarks()

expt_num = 0
os.system("mkdir -p %s" % results_path)

for i in range(0, len(conf_params) + 1):
    for conf in combinations(conf_params, i):
        expt_num = expt_num + 1

        print "#############################################################"
        print "############## Experiment number: %d ########################" % expt_num
        print "#############################################################"

        if i == 2:
            continue

        # if expt_num != 3:
        #     continue

        f = open('/tmp/optimiz.conf', 'w')

        if os.path.exists("%s/%d" % (results_path, expt_num + 1)):
            print "Skipping test number %d\n" % expt_num
            continue

        ## Create config file
        for param in conf:
            f.write("%s=1\n" % param)
        f.write("PLOT_DIR=%s/%d/\n" % (results_path, expt_num))
        f.close()

        os.system("mkdir -p %s/%d" % (results_path, expt_num))
        os.system("cp /tmp/optimiz.conf %s/%d" % (results_path, expt_num))

        ## Build with this config ##
        os.system("make -C %s" % simulator_path)

        for bm in benchmarks:
            command = "%s/bin/parsecmgmt -n 8 -s \"%s/pin \
            -t %s/bin/simulator.so --\" \
            -a run -i simsmall -p %s" % (parsec_path, pin_path, simulator_path, bm)
            #-a run -p %s" % (parsec_path, pin_path, simulator_path, bm)
            # -a run -i simsmall -p %s" % (parsec_path, pin_path, simulator_path, bm)


            print "Executing \'" + command + "\'"

            os.system(command)

            ## Generate graphs
            command = simulator_path + "/plot/plot.sh"
            os.environ['SIMULATOR_HOME'] = simulator_path
            os.system(command)
            os.system("zenity --info --text=\'[%d] Benchmark %s completed\'&" \
                      % (expt_num, bm))

        os.system("zenity --info --text=\'Test number %d executed.\'&" \
                  % expt_num)
