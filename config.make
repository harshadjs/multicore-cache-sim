##
## Configuration file for simulator
##

## PINTOOL: Keep this on
## PINTOOL=1

## If turned on, tracks private blocks. And so, disables coherence mechanism
## for all private blocks
PRIVATE_TRACKING=1

## Plot directory
PLOT_DIR=/tmp/plot

## Do not remove these parameters, otherwise the code will
## will not compile
## Directory set bits: Implies size of directory
DIR_SET_BITS=14

## Directory nways: Associativity of directory cache
DIR_NWAYS=4

###################
## OPTIMIZATIONS ##
###################
## Turn ON to enable all optimizations
OPTIMIZ=1

## Finer control
OPTIMIZ_HEAP=0
OPTIMIZ_RDONLY_COHERENCE=0
