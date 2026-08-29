#!/bin/zsh

dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
export GAZEBO_MODEL_PATH=$GAZEBO_MODEL_PATH:${dir}/src/wrjqsimulator/uav_simulator/models
export GAZEBO_RESOURCE_PATH=$GAZEBO_RESOURCE_PATH:${dir}/src/wrjqsimulator/uav_simulator/urdf
export GAZEBO_PLUGIN_PATH=$GAZEBO_PLUGIN_PATH:${dir}/src/wrjqsimulator/uav_simulator/plugins
