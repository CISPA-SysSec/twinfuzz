# Twinfuzz Artifact Evaluation NDSS 2025

## Setup provided:
- Local setup with requested hardware 

This readme file provides an overview of the artifact presented in our TwinFuzz paper, along with step-by-step instructions for running it either locally or remotely. Our artifact makes it possible to reproduce some of our most important results. The focus of the artifact is on the crash experiment with a specific hardware/software configuration. If access to other platforms listed in [Table Setup](#) is available, the artifact can also be used on these platforms. To facilitate artifact evaluation, we have included a Dockerfile containing all necessary files to build our prototype, as described in Section \ref{sec:design}, for a __Linux-Intel__ machine. While a local setup is possible, we recommend connecting to our remote laptop via SSH and VPN if your machine does not meet the required specifications.


## Description & Requirements

This section outlines all the information necessary to recreate the experimental environment for running our artifact. The experiments can be accessed through SSH using the key provided by your machine. To run the Dockerfile locally, your machine must support video hardware acceleration, have Intel VAAPI drivers, and an x86-64 CPU. If you do not have access to such a machine, we offer SSH access through a VPN to our lab’s network.

### Requirements

1. **Access Instructions**: The artifact is publicly available via a GitHub link. We will publish the artifact on Zenodo to ensure a permanent hosting of the artifact. The repository includes a Dockerfile for an Intel-Linux setup.

2. **Hardware Requirements**: To reproduce our experiments, your machine must have video hardware acceleration, Intel VAAPI drivers, and an Intel x86-64 CPU.

3. **Software Requirements**: The software required is the same as that needed to build and run FFmpeg with hardware acceleration enabled.

4. **Benchmarks**: No specific benchmarks are needed.


## Artifact Local Installation

### Locally

To build and install our prototype locally using the Dockerfile, we have created a script to simplify the process. Run the following command:

```bash
./run_docker.sh
```

This script builds the Docker image as follows:

```bash
docker build -t twinframe-linux-intel .
```

Then run the Docker container:

```bash
docker run --rm --device=/dev/dri -e LIBVA_DRIVER_NAME="iHD" -it twinframe-linux-intel /bin/bash
```

## Experiment Workflow

As explained in Section [Section Evaluation Setup](#), our experiments use Intel VAAPI drivers and an Intel CPU. We provide a script to simplify the evaluation of our prototype. This script performs differential fuzzing, generating both software and hardware frames, and compares their outputs to detect observable differences (e.g., they differ in content or size, or a crash occurs).


To run the evaluation with the corpus used in our experiments (comprising 10 trials of 24 hours each), navigate to the `/out` directory within the container and run:


```bash
root:/out# ./evaluation_script.sh
```

You can customize the script by modifying the parameters directly in the bash script.

## Crash Reproduction

After running the evaluation, you will find ten folders (each corresponding to a trial), named `ffmpeg_AV_CODEC_H264_fuzzer_trial_X` (where `X` represents an integer from 1 to 10). Each folder contains a corpus folder and a potential list of discovered crashes. Additionally, you may find `hw_frame.pgm` and `sw_frame.pgm` files if TwinFuzz found observable differences.

To reproduce a crash, run the `ffmpeg_AV_CODEC_H264_fuzzer` binary from the `/out` directory and provide the input file corresponding to the crash:



```bash
root:/out# ./ffmpeg_AV_CODEC_H264_fuzzer trial_2/crash-artifact-2-crash-c21a135d3b3e925269f52437ff4c38503017a40e
```

After executing the crash input, the frame decoding process will continue until a frame mismatch is detected, after which AddressSanitizer will raise an abort signal. You should see an output similar to the following:

```bash
root:/out/ffmpeg_AV_CODEC_ID_H264_fuzzer_trial_2# ../ffmpeg_AV_CODEC_ID_H264_fuzzer crash-artifact-2-crash-c21a135d3b3e925269f52437ff4c38503017a40e
../ffmpeg_AV_CODEC_ID_H264_fuzzer: Running 1 inputs 1 time(s) each.
Running: crash-artifact-2-crash-c21a135d3b3e925269f52437ff4c38503017a40e
--> Decoding completed correctly
--> Decoding completed correctly
differed on data line 0 (640, 640)
HW/SW buffer contents differ; see {hw,sw}_frame.pgm
==2776== ERROR: libFuzzer: deadly signal
SUMMARY: libFuzzer: deadly signal
```

