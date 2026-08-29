==本项目基于bq项目无人机仿真项目，使用前可以参考bq_README.md==

==使用本仓库时，请先删除原livox_laser_simulation文件夹，再解压livox_laser_simulation.zip，之后一起编译使用==

## 使用说明
==若要启动真实场景下的5架无人机探索测试请进行以下操作，并且确保你的电脑有128Gb的内存（包括swap空间）：==

1. 修改world文件内对应的模型本地目录，要修改的world文件为uav_simulator/worlds/saomiao/world_saomiao.world

   ```
   找到文件内的105行，为以下内容
             <geometry>
               <mesh>
                 <uri>/home/lxy/workspace/my_project/fame_simulator/src/wrjqsimulator/uav_simulator/worlds/saomiao/dae/1.dae</uri>
                 <scale>1 1 1</scale>
               </mesh>
             </geometry>
             <transparency>0</transparency>
             <cast_shadows>1</cast_shadows>
           </visual>
           <collision name='collision'>
             <laser_retro>0</laser_retro>
             <max_contacts>10</max_contacts>
             <pose>0 0 0 0 -0 0</pose>
             <geometry>
               <mesh>
                 <uri>/home/lxy/workspace/my_project/fame_simulator/src/wrjqsimulator/uav_simulator/worlds/saomiao/dae/1.dae</uri>
                 <scale>1 1 1</scale>
               </mesh>
             </geometry>
   ---
   其中两个uri标签内的1.dae路径要替换为本机该workspace的目录，举例来说，若你将该仓库放置在~/xx目录下，则两个uri应为以下：
             <geometry>
               <mesh>
                 <uri>/home/你的主机名/xx/src/wrjqsimulator/uav_simulator/worlds/saomiao/dae/1.dae</uri>
                 <scale>1 1 1</scale>
               </mesh>
             </geometry>
             <transparency>0</transparency>
             <cast_shadows>1</cast_shadows>
           </visual>
           <collision name='collision'>
             <laser_retro>0</laser_retro>
             <max_contacts>10</max_contacts>
             <pose>0 0 0 0 -0 0</pose>
             <geometry>
               <mesh>
                 <uri>/home/你的主机名/xx/src/wrjqsimulator/uav_simulator/worlds/saomiao/dae/1.dae</uri>
                 <scale>1 1 1</scale>
               </mesh>
             </geometry>
   ```

3. 同理要使用saomiao/dae/下的别的地形文件，也是修改第一条中的两个uri置对应的dae名字

2. 启动带有真实地形及五架无人机的仿真器

   ```bash
   cd yourWorkSpace
   source devel/setup.bash
   source src/wrjqsimulator/uav_simulator/gazeboSetup.bash
   roslaunch uav_simulator test_5.launch
   ```

---

## 新提交记录
### v0.0.6
1. 修复了gazeboSetup.zsh中的bug
2. 添加了专门用于本机测试的launch文件，test_drone2_up.xx，使用world_plustwo2.world
3. 由于imu没有用，暂时降低了imu插件的频率至1Hz
4. 为world_plustwo2.world添加了测试用的ode，目前physics的设置为，在不影响激光雷达频率的情况下，可以适当增加real_time_update_rate
  <max_step_size>0.002</max_step_size>
  <real_time_factor>1</real_time_factor>
  <real_time_update_rate>100</real_time_update_rate>

## 以下为旧的说明，仅供参考

1. 启动仿真

   ```bash
   source devel/setup.bash && source src/bq-uav-simulator-cmu/uav_simulator/gazeboSetup.bash
   roslaunch uav_simulator drone1.launch
   ```

2. 更改激光安装位置
   文件1：uav_simulator/urdf/xacro/drone_mid360.gazebo

   ```
   line 113:
     <joint name="body_to_livox" type="fixed" >
       <parent link="base_link" />
       <child link="livox_base" />
       <origin xyz="0 0 -0.10" rpy="3.1415926 0 0" /> <!-- 改这行 -->
     </joint>
   ```

   文件2：uav_simulator/launch/drone1.launch

   ```
   line 42: 同xacro文件对应
     <param name="lidar/rollDegree" value="180" type="double"/>
     <param name="lidar/pitchDegree" value="0" type="double"/>
     <param name="lidar/yawDegree" value="0" type="double"/>
     <param name="lidar/x" value="0" type="double"/>
     <param name="lidar/y" value="0" type="double"/>
     <param name="lidar/z" value="-0.2" type="double"/>
   ```

3. 添加新的无人机，目前drone1.launch只会启动2架无人机
   文件1：uav_simulator/launch/drone1.launch

   ```
   以group形式添加新的无人机，参考以下格式，x为你要添加的无人机id号; 生成的位置，控制的remap
     <group ns="dronex">
       <arg name="mymodel" default="$(find uav_simulator)/urdf/xacro/drone_mid360.gazebo " />
       <param name="quadcopter_description" command="$(find xacro)/xacro --inorder $(arg mymodel) topic:=dronex/scan"/>
       <node name="spawn_urdf" pkg="gazebo_ros" type="spawn_model" 
           args="-urdf -model dronex -param quadcopter_description -x 0 -y 0 -z 1.7"/>
       <node name="keyboard_control" pkg="uav_simulator" type="keyboard_control" >
       	<remap from="/CERLAB/quadcopter/cmd_vel" to="CERLAB/quadcopter/cmd_vel"/>
       	<remap from="/CERLAB/quadcopter/land" to="CERLAB/quadcopter/land"/>
       	<remap from="/CERLAB/quadcopter/posctrl" to="CERLAB/quadcopter/posctrl"/>
       	<remap from="/CERLAB/quadcopter/reset" to="CERLAB/quadcopter/reset"/>
       	<remap from="/CERLAB/quadcopter/takeoff" to="CERLAB/quadcopter/takeoff"/>
       	<remap from="/CERLAB/quadcopter/vel_mode" to="CERLAB/quadcopter/vel_mode"/>
       </node>
       <node name="msg_translator" pkg="uav_simulator" type="msgTransForRacer" output="screen">
       	<remap from="~pointonetopic" to="scan"/>
       	<remap from="~pointtwotopic" to="pointcloud_lidar"/>
       	<remap from="~lidarPointInWorld" to="pointcloud"/>
       	<remap from="~odomtopic" to="CERLAB/quadcopter/odom"/>
       	<remap from="~sensorPoseTopic" to="CERLAB/quadcopter/sensor_pose"/>
       	<remap from="~quadCommand" to="/planning/pos_cmd_x"/>
       	<remap from="~mavCommand" to="CERLAB/quadcopter/cmd_acc"/>
       	<remap from="~setPointPose" to="CERLAB/quadcopter/setpoint_pose"/>
       	<param name="lidar/rollDegree" value="180" type="double"/>
       	<param name="lidar/pitchDegree" value="0" type="double"/>
       	<param name="lidar/yawDegree" value="0" type="double"/>
       	<param name="lidar/x" value="0" type="double"/>
       	<param name="lidar/y" value="0" type="double"/>
       	<param name="lidar/z" value="-0.2" type="double"/>
       </node>
   </group> 
   ```

---

## 提交记录

### v0.0.5
- 重新更新了无人机的启动位置
- 添加了新的复杂场景裁剪模型及无材质真实场景模型

### v0.0.4

- 添加了真实场景的相关文件，见使用说明

### v0.03
- 添加了边缘分布的4架无人机启动launch - bianyuan_test_drone4_up.launch
- 修改了激光雷达的参数，最大探测距离30m，无噪声
- 添加了200x200场景丰富后的world - world_plustwo2.world

### v0.0.1

- 基础之上，根据群里改进算法文档进行了改进，具体可以见群里文件查看

---

### v0.0.2

- 添加一键起飞和起飞至指定高度接口
- 修改了无人机Position控制相关的PID参数

---

