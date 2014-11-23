#!/bin/bash
if [ "$SIMULATOR_HOME" = "" ]; then
	echo "SIMULATOR_HOME not defined. Please define it and point it to home of simulator"
	exit 0
fi

source ${SIMULATOR_HOME}/config.make

gnuplot -e "output_file='${PLOT_DIR}/report.pdf'" \
	-e "page_track_input_file='${PLOT_DIR}/page_track'" \
	-e "block_track_input_file='${PLOT_DIR}/block_track'" \
	-e "cache_stats_input_file='${PLOT_DIR}/cache_stats'" \
	${SIMULATOR_HOME}/plot/draw_graphs.plg
