#include <iostream>
#include <cmath>
#include <random>
#include "ros/ros.h"
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/PointCloud.h>
#include <sensor_msgs/point_cloud_conversion.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/TwistStamped.h>
#include <nav_msgs/Odometry.h>
#include <Eigen/Eigen>
#include "quadrotor_msgs/PositionCommand.h"
#include "mavros_msgs/PositionTarget.h"
#include "std_msgs/Bool.h"
#include "std_msgs/Empty.h"
#include "std_msgs/Float32.h"

#include <pcl/common/common.h>
#include <pcl/conversions.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/radius_outlier_removal.h>
#include <pcl/filters/passthrough.h>
#include <pcl/point_types.h>
#include <pcl/search/kdtree.h>
#include <pcl/search/organized.h>
#include <pcl/segmentation/conditional_euclidean_clustering.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>
#include <pcl_ros/transforms.h>
#include <pcl/common/transforms.h>
#include <pcl/filters/crop_box.h>

using namespace Eigen;

typedef pcl::PointXYZ VPoint;
typedef pcl::PointCloud<VPoint> VPointCloud;

ros::Publisher pointcloudtwo_pub;
ros::Publisher sensorPosePub;
ros::Publisher mavCommandPub;
ros::Publisher lidaraPointInWorld;
ros::Publisher setPointPosePub;
ros::Publisher takeOffPub;

Matrix4d lidar2body;
Matrix4d lidar2world;
Matrix4d body2world;
Eigen::Quaterniond lidar2world_quat;

float heightX = 0;
float heightY = 0;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0);  

void commandCallback(const quadrotor_msgs::PositionCommandConstPtr& msg) {  
  mavros_msgs::PositionTarget pubMsg;
  pubMsg.header = msg->header;
  pubMsg.position = msg->position;
  pubMsg.velocity = msg->velocity;
  pubMsg.acceleration_or_force = msg->acceleration;
  pubMsg.yaw = msg->yaw;
  pubMsg.yaw_rate = msg->yaw_dot;
  pubMsg.coordinate_frame = 1;
  //mavCommandPub.publish(pubMsg);

  // 使用Eigen库创建一个旋转矩阵
  Eigen::Matrix3d rotation_matrix;
  rotation_matrix = Eigen::AngleAxisd(msg->yaw, Eigen::Vector3d::UnitZ());

  // 将旋转矩阵转换为四元数
  Eigen::Quaterniond quaternion(rotation_matrix);
  geometry_msgs::PoseStamped abcMsg;
  abcMsg.header = msg->header;
  abcMsg.pose.position = msg->position;
  abcMsg.pose.orientation.x = quaternion.x();
  abcMsg.pose.orientation.y = quaternion.y();
  abcMsg.pose.orientation.z = quaternion.z();
  abcMsg.pose.orientation.w = quaternion.w();
  
  setPointPosePub.publish(abcMsg);
}  

void AccCallback(const geometry_msgs::TwistStampedConstPtr& msg) {  
  double random_num_x = dis(gen);
  double random_num_y = dis(gen);
  double random_num_z = dis(gen);
  double acc_x = std::abs(msg->twist.linear.x) < 2.0 ? std::abs(msg->twist.linear.x) : std::abs(msg->twist.linear.x) - random_num_x;
  double acc_y = std::abs(msg->twist.linear.y) < 2.0 ? std::abs(msg->twist.linear.y) : std::abs(msg->twist.linear.y) - random_num_y;
  double acc_z = std::abs(msg->twist.linear.z) < 2.0 ? std::abs(msg->twist.linear.z) : std::abs(msg->twist.linear.z) - random_num_z;
  // ROS_FATAL_STREAM_THROTTLE(0.3, "Acc x y z," << acc_x << "," << acc_y << "," << acc_z);
}

void VelCallback(const geometry_msgs::TwistStampedConstPtr& msg) {  
  double random_num_x = dis(gen);
  double random_num_y = dis(gen);
  double random_num_z = dis(gen);
  double vel_x = std::abs(msg->twist.linear.x);
  double vel_y = std::abs(msg->twist.linear.y);
  double vel_z = std::abs(msg->twist.linear.z);
  double vel_roll = std::abs(msg->twist.angular.x) < 0.6 ? std::abs(msg->twist.angular.x) : std::abs(msg->twist.angular.x) - random_num_x;
  double vel_pitch = std::abs(msg->twist.angular.y) < 0.6 ? std::abs(msg->twist.angular.y) : std::abs(msg->twist.angular.y) - random_num_y;
  double vel_yaw = std::abs(msg->twist.angular.z);
  // ROS_FATAL_STREAM_THROTTLE(0.3, "vel x y z roll pitch yaw," << vel_x << "," << vel_y << "," << vel_z << "," << vel_roll << "," << vel_pitch << "," << vel_yaw);
}

void pointcloud1Callback(const sensor_msgs::PointCloud2ConstPtr& mmwCloudMsg) {  
  //sensor_msgs::PointCloud2 laserCloudMsg;
  //convertPointCloudToPointCloud2(*mmwCloudMsg, laserCloudMsg);
  //laserCloudMsg.header.stamp = mmwCloudMsg->header.stamp;
  //laserCloudMsg.header.frame_id = "livox_base";
  //pointcloudtwo_pub.publish(laserCloudMsg);
  
  // to pcl
  boost::shared_ptr<VPointCloud> originCloud = boost::make_shared<VPointCloud>();
  //pcl::fromROSMsg(laserCloudMsg, *originCloud);
  pcl::fromROSMsg(*mmwCloudMsg, *originCloud);
  // to body
  boost::shared_ptr<VPointCloud> bodyCloud = boost::make_shared<VPointCloud>();
  pcl::transformPointCloud(*originCloud, *bodyCloud, lidar2body);
  // crop
  pcl::CropBox<VPoint> crop;
  crop.setInputCloud(bodyCloud);
  crop.setMin(Eigen::Vector4f(-0.4, -0.4, -0.05, 1.0));
  crop.setMax(Eigen::Vector4f(0.4, 0.4, 0.15, 1.0));
  crop.setNegative(true); // outside
  crop.filter(*bodyCloud);
  // to world
  boost::shared_ptr<VPointCloud> worldCloud = boost::make_shared<VPointCloud>();
  pcl::transformPointCloud(*bodyCloud, *worldCloud, body2world);
  
  // for (auto &pt : originCloud->points) {
  // 	Eigen::Vector4d vPoint(pt.x, pt.y, pt.z, 1.0);
  // 	if (-0.25 <= pt.x && pt.x <= 0.25 && -0.25 <= pt.y && pt.y <= 0.25 && -0.25 <= pt.z && pt.z <= 0.1)
  // 		continue;
  // 	Eigen::Vector4d vWorldPoint = lidar2world * vPoint;

  //   pt.x = static_cast<float>(vWorldPoint.x());
  //   pt.y = static_cast<float>(vWorldPoint.y());
  //   pt.z = static_cast<float>(vWorldPoint.z());
  //   filterCloud->points.push_back(pt);
	// }
  sensor_msgs::PointCloud2 lidarPointInWorld;
  pcl::toROSMsg(*worldCloud, lidarPointInWorld);
  lidarPointInWorld.header.stamp = mmwCloudMsg->header.stamp;
  lidarPointInWorld.header.frame_id = "world";
  lidaraPointInWorld.publish(lidarPointInWorld);
}  

void odomCallback(const nav_msgs::OdometryConstPtr& msg) {
	Matrix4d Pose_receive = Matrix4d::Identity();

  Eigen::Vector3d request_position;
  Eigen::Quaterniond request_pose;
  request_position.x() = msg->pose.pose.position.x;
  request_position.y() = msg->pose.pose.position.y;
  request_position.z() = msg->pose.pose.position.z;
  request_pose.x() = msg->pose.pose.orientation.x;
  request_pose.y() = msg->pose.pose.orientation.y;
  request_pose.z() = msg->pose.pose.orientation.z;
  request_pose.w() = msg->pose.pose.orientation.w;
  Pose_receive.block<3, 3>(0, 0) = request_pose.toRotationMatrix();
  Pose_receive(0, 3) = request_position(0);
  Pose_receive(1, 3) = request_position(1);
  Pose_receive(2, 3) = request_position(2);
  body2world = Pose_receive;
  
  lidar2world = body2world * lidar2body;
  lidar2world_quat = lidar2world.block<3, 3>(0, 0);

	geometry_msgs::PoseStamped pubMsg;
	pubMsg.header = msg->header;
	pubMsg.header.frame_id = "world";
	pubMsg.pose.position.x = lidar2world(0, 3);
	heightX = pubMsg.pose.position.x;
  pubMsg.pose.position.y = lidar2world(1, 3);
  heightY = pubMsg.pose.position.y;
  pubMsg.pose.position.z = lidar2world(2, 3);
  pubMsg.pose.orientation.w = lidar2world_quat.w();
  pubMsg.pose.orientation.x = lidar2world_quat.x();
  pubMsg.pose.orientation.y = lidar2world_quat.y();
  pubMsg.pose.orientation.z = lidar2world_quat.z();
	sensorPosePub.publish(pubMsg);
}

void TakeOffCallback(const std_msgs::BoolConstPtr& msg) {  
  std_msgs::Empty send_msgs;
  takeOffPub.publish(send_msgs);
}  

void FlyToHeightCallback(const std_msgs::Float32ConstPtr& msg) {  
  
  geometry_msgs::PoseStamped abcMsg;
  abcMsg.pose.position.x = heightX;
  abcMsg.pose.position.y = heightY;
  abcMsg.pose.position.z = msg->data;
  
  setPointPosePub.publish(abcMsg);
}

int main(int argc, char **argv) {
  // 随机数

  // 初始化ROS节点  
  ros::init(argc, argv, "msg_translator");
  // 创建节点句柄  
  ros::NodeHandle n("~");
  // 获取外参
  double rollDegree, pitchDegree, yawDegree;
  double x, y, z;
  n.param("lidar/rollDegree", rollDegree, 0.0);
  n.param("lidar/pitchDegree", pitchDegree, 0.0);
  n.param("lidar/yawDegree", yawDegree, 0.0);
  n.param("lidar/x", x, 0.0);
  n.param("lidar/y", y, 0.0);
  n.param("lidar/z", z, 0.0);
  //ROS_INFO("Param, %f, %f", rollDegree, z);

  
  // 初始化传感器外参
  lidar2body << 1.0, 0.0, 0.0, x,
  									0.0, 1.0, 0.0, y,
  									0.0, 0.0, 1.0, z,
  									0.0, 0.0, 0.0, 1.0;
  double rollRad = rollDegree * M_PI / 180.0;  
  double pitchRad = pitchDegree * M_PI / 180.0;  
  double yawRad = yawDegree * M_PI / 180.0;  
  
  // 创建从RPY得到的旋转矩阵  
  Eigen::AngleAxisd rollAngle(rollRad, Eigen::Vector3d::UnitX());  
  Eigen::AngleAxisd pitchAngle(pitchRad, Eigen::Vector3d::UnitY());  
  Eigen::AngleAxisd yawAngle(yawRad, Eigen::Vector3d::UnitZ());  
  Eigen::Matrix3d rotation = yawAngle.toRotationMatrix() * pitchAngle.toRotationMatrix() * rollAngle.toRotationMatrix();  
  lidar2body.block<3, 3>(0, 0) = rotation;
  
  // 创建一个订阅者，订阅名为chatter的话题，队列长度为1000  
  ros::Subscriber pointcloude1_sub = n.subscribe<sensor_msgs::PointCloud2>("pointonetopic", 1, pointcloud1Callback);
  ros::Subscriber odom_sub = n.subscribe<nav_msgs::Odometry>("odomtopic", 1, odomCallback);
  ros::Subscriber quadCommandSub = n.subscribe<quadrotor_msgs::PositionCommand>("quadCommand", 1, commandCallback);
  ros::Subscriber takeOffSub = n.subscribe<std_msgs::Bool>("/takeoff", 1, TakeOffCallback);
  ros::Subscriber flyToHeight = n.subscribe<std_msgs::Float32>("/flyToHeight", 1, FlyToHeightCallback);

  // ros::Subscriber accSub = n.subscribe<geometry_msgs::TwistStamped>("/drone1/CERLAB/quadcopter/acc", 1, AccCallback);
  // ros::Subscriber velSub = n.subscribe<geometry_msgs::TwistStamped>("/drone1/CERLAB/quadcopter/vel", 1, VelCallback);
  
  pointcloudtwo_pub = n.advertise<sensor_msgs::PointCloud2>("pointtwotopic", 1);
  sensorPosePub = n.advertise<geometry_msgs::PoseStamped>("sensorPoseTopic", 1);
  mavCommandPub = n.advertise<mavros_msgs::PositionTarget>("mavCommand", 1);
  lidaraPointInWorld = n.advertise<sensor_msgs::PointCloud2>("lidarPointInWorld", 1);
  setPointPosePub = n.advertise<geometry_msgs::PoseStamped>("setPointPose", 1);
  takeOffPub = n.advertise<std_msgs::Empty>("takeoff", 1);
  
  // 循环等待回调函数  
  ros::spin();  
  
  return 0;  

}
