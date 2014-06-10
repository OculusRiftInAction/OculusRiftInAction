
//#include <pcl/ModelCoefficients.h>
//#include <pcl/point_types.h>
//#include <pcl/sample_consensus/method_types.h>
//#include <pcl/sample_consensus/model_types.h>
//#include <pcl/filters/passthrough.h>
//#include <pcl/filters/project_inliers.h>
//#include <pcl/segmentation/sac_segmentation.h>
//#include <pcl/surface/concave_hull.h>
//#include <pcl/search/kdtree.h>
//#include <pcl/kdtree/kdtree_flann.h>
//#include <pcl/surface/mls.h>


//pcl::PointCloud<pcl::PointXYZ>::Ptr cloudFromGlmPoints(const std::vector<glm::vec3> & depthData) {
//  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
//  std::for_each(depthData.begin(), depthData.end(), [&](const glm::vec3 & v){
//    cloud->push_back(pcl::PointXYZ(v.x, v.y, v.z));
//  });
//  return cloud;
//}
//
//
//void foo(std::vector<glm::vec3> & depthData) {
//  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud = cloudFromGlmPoints(depthData);
//  pcl::PointCloud<pcl::PointXYZ>::Ptr
//    cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>),
//    cloud_projected(new pcl::PointCloud<pcl::PointXYZ>);
//
//  // Create a KD-Tree
//  pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
//
//
//  // Init object (second point type is for the normals, even if unused)
//  {
//    // Output has the PointNormal type in order to store the normals calculated by MLS
//    pcl::PointCloud<pcl::PointNormal> mls_points;
//    pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointNormal> mls;
//    mls_points.resize(cloud->size());
//    // Set parameters
//    mls.setInputCloud(cloud);
//    mls.setUpsamplingMethod(pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointNormal>::UpsamplingMethod::DISTINCT_CLOUD);
//    mls.setComputeNormals(false);
//
//    mls.setPolynomialFit(true);
//    mls.setSearchMethod(tree);
//    mls.setSearchRadius(0.03);
//    // Reconstruct
//    mls.process(mls_points);
//  }
//
//  for (int i = 0; i < cloud->size(); ++i) {
//    const pcl::PointXYZ & p = cloud->at(i);
//    depthData[i] = glm::vec3(p.x, p.y, p.z);
//  }
//
//  pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
//  pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
//  {
//    // Create the segmentation object
//    pcl::SACSegmentation<pcl::PointXYZ> seg;
//    // Optional
//    seg.setOptimizeCoefficients(true);
//    // Mandatory
//    seg.setModelType(pcl::SACMODEL_PLANE);
//    seg.setMethodType(pcl::SAC_RANSAC);
//    seg.setDistanceThreshold(0.01);
//    seg.setInputCloud(cloud_filtered);
//    seg.segment(*inliers, *coefficients);
//  }

  //SAY("PointCloud after segmentation has: %d data inliers.  Model coefficients: %f, %f, %f, %f",
  //  inliers->indices.size(),
  //  coefficients->values[0],
  //  coefficients->values[1],
  //  coefficients->values[2],
  //  coefficients->values[3]);

  //// Project the model inliers
  //pcl::ProjectInliers<pcl::PointXYZ> proj;
  //proj.setModelType(pcl::SACMODEL_PLANE);
  //proj.setIndices(inliers);
  //proj.setInputCloud(cloud_filtered);
  //proj.setModelCoefficients(coefficients);
  //proj.filter(*cloud_projected);
  //std::cerr << "PointCloud after projection has: "
  //  << cloud_projected->points.size() << " data points." << std::endl;

  //SAY("Model inliers: %d", inliers->indices.size());
  ////for (size_t i = 0; i < inliers->indices.size(); ++i) {
  ////  std::cerr << inliers->indices[i] << "    " << cloud->points[inliers->indices[i]].x << " "
  ////  << cloud->points[inliers->indices[i]].y << " "
  ////  << cloud->points[inliers->indices[i]].z << std::endl;
  ////}
//}
