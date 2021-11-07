# Parallel Determinacy Race Detection for Futures
 
This repository contains the prototype race detection system based 
on the F-Order algorithm. F-Order is the first known parallel race 
Detection algorithm that detects races on programs that use futures.

## UPDATE

Since the publication of the paper, we have found a bug in the code base.  
The fix of the bug causes the performance of the sort benchmark to slow down 
by about 30% when running with F-Order (while others remain similar), and the 
scalability remains the same.  

## License
   
Unless otherwise specified, the code in this repository is distributed
under the MIT license. All code from external projects -- the Cilk
Plus runtime, some benchmarks, and the LLVM compiler infrastructure --
is separately licensed.
    
## Using
     
### Artifact Evaluation
Before installation, make sure to install the Docker container system. Docker
can be obtained either through your operating systems package manager or by
downloading from the Docker website
[here](https://store.docker.com/search?type=edition&offering=community&operating_system=linux).

1. Make sure your machine has 24GB+ RAM.

2. Run the `build-docker.sh` script. This step will build the Docker container 
that has all the libraries needed to build and run any code contained in this
repository.

3. Run the `start-docker.sh` script to start the Docker container and set up
the Docker container environment.

4. In the container, run the `setup.sh` script. This should build the modified compiler and 
required Cilk Plus runtimes.

5. In the container, run the `build_benches.sh` script to build all benchmarks.

6. In the container, run the `run_[benchmark_name].sh` script under `run_scripts`
   folder to run a certain benchmark with all three configurations: baseline, 
   reachability and full detection.

7. All benchmarks do not have races. The experiments are meant to demonstrate the 
   algorithm’s scalability. 

### Compare to FutureRD
We have compared our parallel race detector against the FutureRD, the 
state-of-the-art sequential race detector for futures.

1. Go to futurerd folder.

2. Run the `build-docker.sh` script.

3. Run the `start-docker.sh` script to start the Docker container.

4. In the container, run the `setup.sh` script.

5. In the container, run the `build_benches.sh` script to build all benchmarks with FutureRD.

6. In the container, run the `run_[benchmark_name].sh` script under `run_scripts`
   folder to run a certain benchmark with all three configurations: baseline, 
   reachability and full detection.


