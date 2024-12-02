# TWINFUZZ Artifact Evaluation NDSS 2025

This repository contains the code for the academic prototype presented in the paper **"TWINFUZZ: Differential Testing of Video Hardware Acceleration Stacks,"** published at NDSS 2025.

**Author List:** Matteo Leonelli, Addison Crump, Meng Wang, Florian Bauckholt, Keno Hassler, Ali Abbasi, Thorsten Holz.

## Intro 

This README provides an overview of the artifact presented in our **TwinFuzz** paper, along with step-by-step instructions for running it locally. The artifact enables the reproduction of key results from the paper, focusing on the crash experiment using a specific hardware/software configuration. To facilitate artifact evaluation, a Dockerfile is included, containing all necessary files to build the prototype on a **linux-intel** machine.

## Description & Requirements

This section provides all the details required to set up the experimental environment needed to run our artifact.
To run the Dockerfile locally, your machine must support video hardware acceleration, have Intel VAAPI drivers, and an x86-64 CPU.

### Requirements

1. **Access Instructions**: The artifact is publicly available via a [Zenodo link](https://doi.org/10.5281/zenodo.14222438). The repository includes a Dockerfile for an __intel-linux__ setup.

2. **Hardware Requirements**: To reproduce our experiments, your machine must have video hardware acceleration, Intel VAAPI drivers, and an Intel x86-64 CPU.

3. **Software Requirements**: The software required is the same as that needed to build and run FFmpeg with hardware acceleration enabled.

4. **Benchmarks**: No specific benchmarks are needed.

## Major Claims

- **(C1):** We propose a technique for *indirectly* guiding an unmodified fuzzer to abstract over a hardware acceleration stack that is otherwise difficult to introspect.
- **(C2):** We present a new method for testing video hardware acceleration stacks. We derive a differential oracle that may indicate the presence of both correctness and security-relevant faults by differentially testing software implementations against the corresponding full hardware acceleration stack.
- **(C3):** We implement a prototype of our approach in a tool called `TWINFUZZ`, capable of fuzzing a specific hardware acceleration stack, providing an observable difference and potentially triggering memory safety bugs using memory sanitizers.


## Artifact Local Installation

This subsection refers to the folder __local_setup__ in the repository. To build and install our prototype locally using the Dockerfile, we have created a script to simplify the process. Run the following command:

```bash
./run_docker.sh
```

This script builds and runs the Docker image as follows.

To build the docker container, use the following command:
```bash
docker build -t twinframe-linux-intel .
```

To run the Docker container, use the following command:
```bash
docker run --rm --device=/dev/dri -e LIBVA_DRIVER_NAME="iHD" -it twinframe-linux-intel /bin/bash
```

## Experiment Workflow

We provide a script to simplify the evaluation and to demonstrate the __functionality__ of our prototype. This script performs differential fuzzing guided by the software implementation mentioned in C1, generating both software and hardware frames. It compares their outputs to detect observable differences as mentioned in C2 (e.g., they differ in content or size, or a crash occurs). These two contributions led to this prototype, which is our C3 contribution.


To run the evaluation with the corpus used in our experimental prototype (comprising 10 trials of 24 hours each), navigate to the `/out` directory within the container and run:

```bash
root:/out# ./evaluation_script.sh
```

You can customize the script by modifying parameters directly in the bash script, such as the number of trials and the duration of a single fuzzing campaign.

## Results Reproduction

After running the default evaluation, you will find ten folders (each corresponding to a trial) named `ffmpeg_AV_CODEC_H264_fuzzer_trial_X` (where `X` represents an integer from 1 to 10). Each folder contains a corpus folder and a potential list of discovered crashes. Additionally, you may find `hw_frame.pgm` and `sw_frame.pgm` files if the tool found observable differences.

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


These steps offer a straightforward way to evaluate the availability and functionality of the code base from our project described in the paper.
