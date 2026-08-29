#include "uav_simulator/depth2pcld.h"


int main(int argc, char **argv)
{
  // 初始化 ros 节点
  ros::init(argc, argv, "depth_to_pointcloud_node");
  // 创建 ros 节点句柄
  ros::NodeHandle nh;
  // 创建深度图转点云的对象
  // DepthToPointCloud dtp(nh);
  // dtp.Init("/camera");

  DepthToPointCloud dtp1(nh);
  dtp1.Init("/camera_left");
  DepthToPointCloud dtp2(nh);
  dtp2.Init("/camera_right");
  DepthToPointCloud dtp3(nh);
  dtp3.Init("/camera_bottom");


  // 循环等待回调函数的执行
  ros::spin();
  return 0;
}
