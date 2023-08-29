#!/bin/bash

timeout=100

for instance in instances/scenarios/*.scen
do
	for cost_function in makespan iter jump-old
	do
		./translator -f $instance -c $cost_function -t $timeout -i
	done	
done
