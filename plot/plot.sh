#!/bin/bash
source ../config.make

gnuplot -e "output_file='${PLOT_DIR}/page_tracking.pdf'" -e \
	"input_file='${PLOT_DIR}/page_track'" stackedbar_page.plg

gnuplot -e "output_file='${PLOT_DIR}/block_tracking.pdf'" -e \
	"input_file='${PLOT_DIR}/block_track'" stackedbar_block.plg

