#!/bin/bash
TRIALS=10
TIMEOUT="24h"
fuzzer_name="ffmpeg_AV_CODEC_ID_H264_fuzzer"
export LIBVA_DRIVER_NAME=iHD
export LIBVA_DRIVERS_PATH=/usr/lib/x86_64-linux-gnu/dri/

function run_fuzzer() {
    CORE_ID=$(( (trial - 1) % $(nproc) ))
    trial_dir="${fuzzer_name}_trial_${trial}"
    mkdir "$trial_dir"
    cd "$trial_dir"
    mkdir corpus
    cd ..
    echo "Copying corpus for trial $trial..."
    cp -r corpus/* "$trial_dir/corpus"

    echo "Starting fuzzer trial $trial..."
    pushd "$trial_dir" >/dev/null
    taskset -c $CORE_ID timeout $TIMEOUT ./../$fuzzer_name ../$trial_dir/corpus -ignore_timeouts=1 -fork=1 -ignore_crashes=1 -ignore_ooms=1 -timeout=5 "-artifact_prefix=crash-artifact-${trial}-"
    fuzzer_exit_code=$?
    popd >/dev/null

    if [ $fuzzer_exit_code -eq 124 ]; then
        echo "Fuzzer trial $trial timed out ($TIMEOUT)."
    else
        echo "Fuzzer trial $trial completed."
    fi
}

# Main
echo "Starting $TRIALS trials of the fuzzer..."
for trial in $(seq 1 $TRIALS); do
    echo "Trial $trial"
    run_fuzzer
done
echo "All trials completed."


