#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --gpus=1
#SBATCH --output=run.out
#SBATCH --partition=gpuA100x4
#SBATCH --account=mzu-delta-gpu
#SBATCH --job-name=threaded-test
#SBATCH --time=00:20:00      # hh:mm:ss for the job
#SBATCH --constraint="scratch"

srun ./main 100

