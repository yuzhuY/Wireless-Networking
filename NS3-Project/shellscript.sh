#!/bin/sh
./waf
for ((i=1;i<31;i++))
do
	now=$(./waf --run scratch/basic11)
	echo "$now">> output.txt
	echo "\n***********************************************">> output.txt
done
