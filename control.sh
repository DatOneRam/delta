#! /usr/bin/env bash
echo "Making control_results.txt..."
{ sudo time -po control_results.txt sudo ./control.out ; } #& sudo sh logc.sh
echo "Done."
exit 0

