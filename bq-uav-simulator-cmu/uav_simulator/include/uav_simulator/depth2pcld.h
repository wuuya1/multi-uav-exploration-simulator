#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_types.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl/conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/filters/extract_indices.h>
#include <opencv2/opencv.hpp>
#include <cv_bridge/cv_bridge.h>

class DepthToPointCloud {
private:
  ros::NodeHandle nh_;
  ros::Subscriber depth_sub_;
  ros::Publisher pc_pub_;
  ros::Publisher pc_pub_filtered_;
  std::string depth_sub_topic_;
  std::string pc_pub_topic_;
  std::string tof_pub_topic_;
  double camera_fx_; // 焦距
  double camera_fy_;
  double camera_cx_; // 光心
  double camera_cy_;
  double camera_factor_; // 比例因子

  bool inited_{false};

public:
  DepthToPointCloud(ros::NodeHandle &nh) {
    nh_ = nh;
    // 从参数服务器获取相机参数
    nh_.param("camera_fx", camera_fx_, 554.254691191187);
    nh_.param("camera_fy", camera_fy_, 554.254691191187);
    nh_.param("camera_cx", camera_cx_, 320.5);
    nh_.param("camera_cy", camera_cy_, 240.5);
    nh_.param("camera_factor", camera_factor_, 1.0);
    // 获取深度图和点云话题的名称
    nh_.param("depth_topic", depth_sub_topic_, std::string("/camera/depth/image_raw"));
    nh_.param("pc_topic", pc_pub_topic_, std::string("/camera/depth/pcld"));
  }

  bool Init(std::string topic_pre) {
    depth_sub_topic_ = topic_pre + "/depth/image_raw";
    pc_pub_topic_ = topic_pre + "/depth/pointcloud";
    tof_pub_topic_ = topic_pre + "/depth/tof";

    // 订阅深度图话题
    depth_sub_ = nh_.subscribe(depth_sub_topic_, 1, &DepthToPointCloud::depthCallback, this);
    // 发布点云话题
    pc_pub_ = nh_.advertise<sensor_msgs::PointCloud2>(pc_pub_topic_, 1);
    // 发布tof点云话题
    pc_pub_filtered_ = nh_.advertise<sensor_msgs::PointCloud2>(pc_pub_topic_ + "_filtered", 1);

    inited_ = true;
    return true;
  }

  void depthCallback(const sensor_msgs::ImageConstPtr &depth_msg) {
    if (!inited_) {
      ROS_ERROR("Initialize instance first!");
      return ;
    }
    // 创建点云对象
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>);

    // 获取深度图的宽高
    int width = depth_msg->width;
    int height = depth_msg->height;

    std::shared_ptr<float> depth_img(new float[width*height], std::default_delete<float[]>());
    // 将 sensor_msgs::ImageConstPtr 中的数据复制到 float 数组中
    memcpy(depth_img.get(), &depth_msg->data[0], width*height*sizeof(float));
    // 创建 cv::Mat 类型的对象，大小为 480*640，类型为 CV_32FC1，使用 float 数组的指针作为数据源
    cv::Mat depth_mat(height, width, CV_32FC1, depth_img.get());

    // cv_bridge::CvImagePtr cv_ptr;
    // try
    // {
    //   cv_ptr = cv_bridge::toCvCopy(depth_msg, sensor_msgs::image_encodings::TYPE_32FC1);
    // }
    // catch (cv_bridge::Exception& e)
    // {
    //   ROS_ERROR("cv_bridge exception: %s", e.what());
    //   return;
    // }

    // 获取深度图的数据指针
    const float *depth_data = reinterpret_cast<const float *>(&depth_msg->data[0]);

    // // 显示 mat 对象
    // cv::imshow("Depth Mat", depth_mat);
    // cv::waitKey(1);

    // 定义要提取的像素的大小
    const int patch_size = 8;

    // 计算图片中心的坐标
    int center_x = width / 2 + 1;
    int center_y = height / 2 + 1;

    // 计算要提取的像素的起始和结束的坐标
    int start_x = center_x - patch_size / 2;
    int start_y = center_y - patch_size / 2;
    int end_x = center_x + patch_size / 2;
    int end_y = center_y + patch_size / 2;

    // 取8*8点阵
    for (int v = start_y; v < end_y; v++) {
      for (int u = start_x; u < end_x; u++) {
        // 获取深度值，单位为毫米
        float depth = depth_mat.at<float>(v, u);
        // 如果深度值为0，表示无效，跳过该点
        if (depth == 0 || depth > 10000)
          continue;
        // 创建一个点
        pcl::PointXYZ p;
        // 根据相机内参和深度值，计算该点的空间坐标
        p.z = double(depth) / camera_factor_;
        p.x = 20 * ((float)u - camera_cx_) * p.z / camera_fx_;
        p.y = 20 * ((float)v - camera_cy_) * p.z / camera_fy_;
        // 将该点加入到点云中
        cloud_filtered->points.push_back(p);
      }
    }

    // 遍历深度图的每个像素
    for (int v = 0; v < height; v++) {
      for (int u = 0; u < width; u++)
      {
        // 获取深度值，单位为毫米
        float d = depth_data[v * width + u];
        // 如果深度值为0，表示无效，跳过该点
        if (d == 0)
          continue;
        // 创建一个点
        pcl::PointXYZ p;
        // 根据相机内参和深度值，计算该点的空间坐标
        p.z = double(d) / camera_factor_;
        p.x = (u - camera_cx_) * p.z / camera_fx_;
        p.y = (v - camera_cy_) * p.z / camera_fy_;
        // 将该点加入到点云中
        cloud->points.push_back(p);
      }
    }

    // 设置点云的属性
    cloud->height = 1;
    cloud->width = cloud->points.size();
    cloud->is_dense = false;
    cloud_filtered->height = 1;
    cloud_filtered->width = cloud_filtered->points.size();
    cloud_filtered->is_dense = false;

    // 创建点云消息对象
    sensor_msgs::PointCloud2 pc_msg;
    // 将点云对象转换为点云消息
    pcl::toROSMsg(*cloud, pc_msg);
    // 设置点云消息的头部信息
    pc_msg.header = depth_msg->header;
    // 发布点云消息
    pc_pub_.publish(pc_msg);

    // 将索引集合对应的点云转换为 sensor_msgs::PointCloud2 格式，并发布出去
    sensor_msgs::PointCloud2 cloud_filtered_msg;
    pcl::toROSMsg(*cloud_filtered, cloud_filtered_msg);
    cloud_filtered_msg.header.frame_id = "camera_link";
    pc_pub_filtered_.publish(cloud_filtered_msg);
  }
};
