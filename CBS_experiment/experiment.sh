#!/bin/bash

timeout=60

for instance in instances/scenarios/*.scen
do
	for cost_function in soc mks
	do
		./translator -f $instance -c $cost_function -t $timeout -i
	done	
done
