# Multi-UAV Exploration Simulator

A GPU-accelerated simulation platform for LiDAR-based multi-UAV cooperative exploration.

## Docker Image

The Docker image is split into three parts because the original file is larger than GitHub's per-file upload limit.

### 1. Download

Download all four files from the GitHub Release page and place them in the same directory:

```text
gpu_livox_docker.tar.part-00
gpu_livox_docker.tar.part-01
gpu_livox_docker.tar.part-02
gpu_livox_docker.tar.sha256
```

Make sure the filenames are unchanged and that all three parts have been downloaded completely.

### 2. Merge

Open a terminal and enter the directory containing the downloaded files. For example:

```bash
cd ~/Downloads
```

Merge the three parts in order:

```bash
cat gpu_livox_docker.tar.part-* > gpu_livox_docker.tar
```

This creates the complete image file:

```text
gpu_livox_docker.tar
```

Verify the merged file:

```bash
sha256sum -c gpu_livox_docker.tar.sha256
```

The expected output is:

```text
gpu_livox_docker.tar: OK
```

The expected SHA256 value is:

```text
338798a39defdd93fc1237852f821fbad650061fc30a1ead5112288c39dd6367
```
