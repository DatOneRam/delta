#! /usr/bin/env bash
echo "Generating results and graph..."
{ sudo time -po experiment_results.txt sudo ./delta.out ; } #& sudo sh logd.sh
echo "Done."
exit 0

