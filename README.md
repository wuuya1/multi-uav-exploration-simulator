# Exploration Simulator Deployment Guide

## 1. Download the Docker Image

The Docker image is split into three parts because the original file is larger
than GitHub's per-file upload limit.

Download all four files from the
[GitHub Releases page](https://github.com/wuuya1/multi-uav-exploration-simulator/releases)
and place them in the same directory:

```text
gpu_livox_docker.tar.part-00
gpu_livox_docker.tar.part-01
gpu_livox_docker.tar.part-02
gpu_livox_docker.tar.sha256
```

Make sure the filenames are unchanged and that all three parts have been
downloaded completely.

Open a terminal and enter the directory containing the downloaded files. For
example:

```bash
cd ~/Downloads
```

Merge the three parts in order:

```bash
cat gpu_livox_docker.tar.part-* > gpu_livox_docker.tar
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

## 2. Download the Source Code

Create the ROS workspace on the host and clone this repository into its `src`
directory:

```bash
mkdir -p /home/ubuntu/docker_ws/explore_ws/src
cd /home/ubuntu/docker_ws/explore_ws/src
git clone https://github.com/wuuya1/multi-uav-exploration-simulator.git gpulivoxgazebows
```

The exploration algorithm source directory `mapper`
is distributed separately. To run the complete exploration system, place it
beside `gpulivoxgazebows`:

```text
/home/ubuntu/docker_ws/explore_ws/src/
├── mapper/
└── gpulivoxgazebows/
```

## 3. Docker Deployment

### Graphics Driver

Make sure that the graphics driver is installed on the host:

```bash
sudo apt update
sudo apt install mesa-utils
glxinfo | grep -i opengl
```

If `OpenGL renderer string:` contains information about the graphics card, the
desktop is being rendered by the GPU. If it contains `llvmpipe`, the desktop is
using software rendering.

In that case, configure the host to use the NVIDIA GPU first:

```bash
sudo prime-select nvidia
```

Run `glxinfo | grep -i opengl` again to confirm that the GPU renderer is being
used. Restart the computer after the switch so that the setting takes effect.

### NVIDIA Docker Support

`nvidia-docker2` must be installed before GPU rendering can be used inside the
container:

```bash
# Add NVIDIA's GPG key.
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg

curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list

# Update the package index.
sudo apt-get update

# Install nvidia-docker2.
sudo apt-get install -y nvidia-docker2

# Configure the Docker container runtime with nvidia-ctk.
sudo nvidia-ctk runtime configure --runtime=docker

# Restart Docker.
sudo systemctl restart docker

# Test the NVIDIA runtime with a CUDA container.
sudo docker run --rm --gpus all nvidia/cuda:11.0.3-base-ubuntu20.04 nvidia-smi
```

If the terminal displays normal `nvidia-smi` output, the NVIDIA Docker runtime
has been installed correctly.

### Load the Image

```bash
sudo docker load -i /your/image/path/image_name.tar
```

For example:

```bash
sudo docker load -i /home/ubuntu/docker_images/gpu_livox_docker.tar
```

### Create the Container

```bash
sudo docker run -it --gpus all \
  -e NVIDIA_DRIVER_CAPABILITIES=all \
  --name explore_container \
  -v /home/ubuntu/docker_ws:/workspace \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /dev/dri:/dev/dri \
  --device=/dev/snd \
  --device=/dev/dri/renderD128 \
  -e DISPLAY=:1 \
  -w /workspace \
  gpu_livox_docker:latest \
  /bin/bash
```

1. `sudo docker run -it --gpus all` starts an interactive container and makes
   all available GPU resources accessible.
2. `-e NVIDIA_DRIVER_CAPABILITIES=all` enables all NVIDIA driver capabilities,
   including compute, display, and video codecs.
3. `--name explore_container` names the container `explore_container`.
4. `-v /home/ubuntu/docker_ws:/workspace` mounts the host Docker workspace at
   `/workspace` inside the container. Therefore, the host directory
   `/home/ubuntu/docker_ws/explore_ws` is available as
   `/workspace/explore_ws` inside the container.
5. `-v /tmp/.X11-unix:/tmp/.X11-unix` mounts the X11 UNIX socket so GUI programs
   can use the host display.
6. `-v /dev/dri:/dev/dri` mounts the host GPU display devices.
7. `--device=/dev/snd` allows the container to access the host audio device.
8. `--device=/dev/dri/renderD128` allows access to the host GPU rendering
   device.
9. `-e DISPLAY=:1` sets the X11 display used by GUI programs.
10. `-w /workspace` sets the container working directory.
11. `gpu_livox_docker:latest` selects the Docker image.
12. `/bin/bash` starts a Bash shell inside the container.

When a prompt such as `root@4b31edc407bc:/workspace#` appears, the container
has been created successfully.

### Re-enter the Container

```bash
sudo docker ps                    # List running containers.
sudo docker ps -a                 # List all containers, including stopped ones.
sudo docker exec -it container_name /bin/bash
sudo docker exec -it <container_id> /bin/bash

# Example:
sudo docker start explore_container
sudo docker exec -it explore_container /bin/bash
```

### Optional Quick-Start Command

Create a helper script by following these steps:

```bash
cd ~/.fishros/bin
sudo touch dnoetic
vi dnoetic
```

Place the following commands in `dnoetic`. Change the container name if
necessary:

```bash
xhost +local: >> /dev/null
echo "Control explore_container: restart(r), enter(e), start(s), stop(c), delete(d), test(t):"
read choose
case $choose in
s) docker start explore_container;;
r) docker restart explore_container;;
e) docker exec -it explore_container /bin/bash;;
c) docker stop explore_container;;
d) docker stop explore_container && docker rm explore_container && sudo rm -rf /home/ubuntu/.fishros/bin/explore_container;;
t) docker exec -it explore_container /bin/bash -c "source /ros_entrypoint.sh && roscore";;
esac
```

Reload Docker group membership for the current session so Docker commands can
be executed without `sudo`:

```bash
newgrp docker
```

If `dnoetic` cannot be executed, add its directory to `PATH`:

```bash
nano ~/.bashrc  # Alternatively, use: gedit ~/.bashrc
```

Append the following line to `~/.bashrc`:

```bash
export PATH=$PATH:$HOME/.fishros/bin
```

Reload the Bash environment:

```bash
source ~/.bashrc
```

### Deploy the Algorithm in the Container

After entering the Docker container, run:

```bash
cd /workspace
mkdir -p explore_ws/src

# Ensure mapper and gpulivoxgazebows are both
# located in /workspace/explore_ws/src before continuing.
cd /workspace/explore_ws
catkin_make
```

The environment is ready when compilation completes successfully.

## 4. Run the Code

### Start the Simulator

```bash
cd /workspace/explore_ws

# Source the environment.
source devel/setup.bash
source src/gpulivoxgazebows/bq-uav-simulator-cmu/uav_simulator/gazeboSetup.bash

# Start the published village simulation scenario with five UAVs.
roslaunch uav_simulator docker_drone5.launch
```

### Start the RViz Visualization Node

```bash
cd /workspace/explore_ws
source devel/setup.bash
roslaunch exploration_manager rviz.launch
```

### Start the Exploration Nodes Individually

Key parameters such as the UAV ID and total number of UAVs can be provided on
the command line. For example, start the exploration algorithm for UAV 1 in a
five-UAV swarm:

```bash
cd /workspace/explore_ws
source devel/setup.bash
roslaunch exploration_manager swarm_exploration_lidar_single.launch drone_id:=1 drone_num:=5
```

To run multiple UAV algorithms on one host, open additional terminals and run:

```bash
source devel/setup.bash
roslaunch exploration_manager swarm_exploration_lidar_single.launch drone_id:=2 drone_num:=5
roslaunch exploration_manager swarm_exploration_lidar_single.launch drone_id:=3 drone_num:=5
roslaunch exploration_manager swarm_exploration_lidar_single.launch drone_id:=4 drone_num:=5
roslaunch exploration_manager swarm_exploration_lidar_single.launch drone_id:=5 drone_num:=5
```

### Publish the Takeoff Commands

```bash
rostopic pub /takeoff std_msgs/Bool "data: false"

# Set the hover height according to the mapped environment.
rostopic pub /flyToHeight std_msgs/Float32 "data: 5.0"
```

### Stop All Gazebo Processes

Pressing Ctrl+C may not stop every Gazebo process:

```bash
pkill gzserver
pkill gzclient
rm -rf build devel
```

### Perform a Clean Rebuild

```bash
rm -rf build devel
catkin_make
```

### Install the Required LKH Version

```bash
wget http://akira.ruc.dk/~keld/research/LKH-3/LKH-3.0.7.tgz
tar xvfz LKH-3.0.7.tgz
cd LKH-3.0.7
make
sudo cp LKH /usr/local/bin
cd ..
rm LKH-3.0.7.tgz
```
