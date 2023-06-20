#!/bin/bash

sudo taskset -c 7 /home/alireza/schedviz/util/trace.sh -out /tmp/schedvis -capture_seconds 30 &
sleep 2
taskset -c 5 ./pipe_lat 1 $1 > ~/output.txt 
echo `tail -n 2 ~/output.txt`
head -n -2 ~/output.txt > ~/output_wo_avg.txt 
sort -n ~/output_wo_avg.txt > ~/output_sorted_congested.txt 
line=`python3 -c "print(int($2*($1-11000)/100))"`
echo "$2" `head -n $line ~/output_sorted_congested.txt | tail -n 1`
