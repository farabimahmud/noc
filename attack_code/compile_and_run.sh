#!/bin/bash

echo "Compiling lat_test.cpp";
g++ -g lat_test.cpp -o lat_test.o -Wno-format;

case $? in
    0)
    echo "Compiled Successfully.";
    ;;

    1)
    echo "Could not Compile Succesfully!";
    exit 1;
    ;;

    2) 
    echo "Compiled with Warnings!";
    ;;

    *)
    echo "Unknown error! Exiting!";
    exit 1;
    ;;
esac

echo "Executing the latency test with ASLR disabled";

rm -rfv "latency.log";

n_cpus=32;

for ((i=0; i<$n_cpus; i++ ))
do
   echo "Executing taskset -c $i setarch $(uname -m) -R ./lat_test.o $i";
   taskset -c $i setarch $(uname -m) -R ./lat_test.o $i;
done

