# Network-File-System 

In this project I built a network file system built with [Filesystem in Userspace](https://sourceforge.net/projects/fuse/) and [gRPC](https://grpc.io/)

To evaluate its performance, I tested this system's run time (latency) for the most common linux commands: ls, cat, mdir, rmdir, rm, >newfile, cd. While slower than the same commands running on local linux system, their latencies are comparable to those in the [SSHFS](https://github.com/libfuse/sshfs) file sytem. I also tested the throughput and latency of file reading and writing.

[Report of Design, Implementation and Performance](NFS-Report.pdf)

System requirements: Ubuntu with FUSE(client only) and gRPC(C++).  
