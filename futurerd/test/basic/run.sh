#!/bin/bash
num_failures=0
failures=""
for f in test[0-9] test[0-9][0-9]; do
    echo "---------------------------------------------------------"
    echo "Running $f"
    ./$f
    rc=$?; if [[ $rc != 0 ]]; then
               let "num_failures += 1"
               failures="$failures $f"
           fi
    echo "---------------------------------------------------------"
done

if [[ $num_failures == 0 ]]; then
    echo "All tests passed!"
else
   echo "$num_failures test failed: $failures"
fi
