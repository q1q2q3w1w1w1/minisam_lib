#include <iostream>
#include <Eigen/Core>
#include "linear/NoiseModel.h"
#include "nonlinear/NonlinearFactorPointerGraph.h"
#include "nonlinear/ISAM2Pointer.h"
#include "geometry/Cal3_S2Stereo.h"
#include "slam/PriorFactor.h"
#include "slam/ProjectionFactor.h"
#include "slam/PriorFactorPose3.h"
#include "SFMdata.h"
#include "gmfconfig.h"
#define GMF_Using_Pose3

int main()
{

   // Define the camera calibration parameters
  Cal3_S2* K(new Cal3_S2(50.0, 50.0, 0.0, 50.0, 50.0));

  // Define the camera observation noise model
  IsotropicNoiseModel* noise = //
      new IsotropicNoiseModel(2, 1.0); // one pixel in u and v

  cout<<"noise.thisR()"<<noise->thisR()<<endl;

  // Create the set of ground-truth landmarks
  std::vector<Eigen::Vector3d> points = createPoints();

  // Create the set of ground-truth poses
  std::vector<Pose3> poses = createPoses();

 // Create an iSAM2 object. Unlike iSAM1, which performs periodic batch steps to maintain proper linearization
  // and efficient variable ordering, iSAM2 performs partial relinearization/reordering at each step. A parameter
  // structure is available that allows the user to set various properties, such as the relinearization threshold
  // and type of linear solver. For this example, we we set the relinearization threshold small so the iSAM2 result
  // will approach the batch result.
  ISAM2Params parameters;
  parameters.optimizationParamsDogleg=new ISAM2DoglegParams;
  parameters.relinearizeThresholdDouble = 0.01;
  parameters.relinearizeSkip = 1;
  ISAM2Pointer isam(parameters);

  // Create a Factor Graph and Values to hold the new data
  NonlinearFactorPointerGraph graph;
  NonlinearFactorPointerGraph nullgraph;
  std::map<int,Eigen::VectorXd> nullEstimateVector;
  std::map<int,Pose3>nullEstimatePose;

  std::map<int,Eigen::VectorXd> initialEstimateVector;
  std::map<int,Pose3> initialEstimatePose;

  // Loop over the different poses, adding the observations to iSAM incrementally
  for (int i = 0; i < poses.size(); ++i)
    {

    // Add factors for each landmark observation
    for (int j = 0; j < points.size(); ++j)
        {
      // Create ground truth measurement
      SimpleCamera camera(poses[i], *K);
      Eigen::Vector2d measurement = camera.projectPoint(points[j]);
      // Add measurement
      GenericProjectionFactor* nf=new GenericProjectionFactor(measurement, noise,
               i+PoseConst, j+PointConst, K);
      graph.push_back(nf);
    }

    cout<<"graph.size(): "<<graph.size()<<endl;

    // Intentionally initialize the variables off from the ground truth
    Pose3 noise(Rot3::Rodrigues(-0.1, 0.2, 0.25), Eigen::Vector3d(0.05, -0.10, 0.20));
    //Pose3 initial_xi = poses[i].compose(noise);
    Pose3 initial_xi = poses[i]*noise;

    // Add an initial guess for the current pose
    initialEstimatePose.insert(std::make_pair(i+PoseConst, initial_xi));

    // If this is the first iteration, add a prior on the first pose to set the coordinate frame
    // and a prior on the first landmark to set the scale
    // Also, as iSAM solves incrementally, we must wait until each is observed at least twice before
    // adding it to iSAM.
    if (i == 0)
        {
      // Add a prior on pose x0, with 30cm std on x,y,z 0.1 rad on roll,pitch,yaw
      DiagonalNoiseModel* poseNoise =new DiagonalNoiseModel(
          (Eigen::VectorXd(6) << Eigen::Vector3d::Constant(0.3), Eigen::Vector3d::Constant(0.1)).finished());
      cout<<"poseNoise.thisR():"<<endl<<poseNoise->thisR()<<endl;
      PriorFactorPose3* npfp=new PriorFactorPose3(0+PoseConst, poses[0], poseNoise);

      cout<<poses[0].matrix()<<endl;
      graph.push_back(npfp);
      //graph.push_back(PriorFactorPose3(0, poses[0], poseNoise));

      // Add a prior on landmark l0
      IsotropicNoiseModel* pointnoise = //
      new IsotropicNoiseModel(3, 0.1);
       cout<<"pointnoise.thisR():"<<endl<<pointnoise->thisR()<<endl;
      PriorFactor* np=new PriorFactor(0+PointConst, points[0], pointnoise);
      graph.push_back(np);

      //graph.push_back(PriorFactor(0, points[0], pointnoise));

      // Add initial guesses to all observed landmarks
      Eigen::Vector3d noise(-0.25, 0.20, 0.15);
      for (int j = 0; j < points.size(); ++j)
        {
        // Intentionally initialize the variables off from the ground truth
        Eigen::VectorXd initial_lj(3);
        initial_lj = points[j] + noise;
        initialEstimateVector.insert(std::make_pair(j+PointConst, initial_lj));
      }

    }
    else
     {
            cout<<"Pose map"<<endl;
   for(auto& pp:isam.thetaPose_)
   {
       cout<<pp.second<<endl;
   }
   cout<<"vector  map"<<endl;
    for(auto& pv:isam.theta_)
   {
       cout<<pv.second<<endl;
   }
      // Update iSAM with the new factors

      isam.update(graph, initialEstimateVector,initialEstimatePose);
             cout<<"Pose map"<<endl;
   for(auto& pp:isam.thetaPose_)
   {
       cout<<pp.second<<endl;
   }
   cout<<"vector  map"<<endl;
    for(auto& pv:isam.theta_)
   {
       cout<<pv.second<<endl;
   }

      isam.update(nullgraph, nullEstimateVector,nullEstimatePose);
        cout<<"Pose map"<<endl;
   for(auto& pp:isam.thetaPose_)
   {
       cout<<pp.second<<endl;
   }
   cout<<"vector  map"<<endl;
    for(auto& pv:isam.theta_)
   {
       cout<<pv.second<<endl;
   }
   std::map<int,Pose3> pose3lin;
      std::map<int,Eigen::VectorXd> currentEstimate = isam.calculateEstimate(&pose3lin);


       cout<<"Pose map"<<endl;
   for(auto& pp:pose3lin)
   {
       cout<<pp.second<<endl;
   }
   cout<<"vector  map"<<endl;
    for(auto& pv:currentEstimate)
   {
       cout<<pv.second<<endl;
   }

      cout << "****************************************************" << endl;
      cout << "Frame " << i << ": " << endl;
      // Clear the factor graph and values for the next iteration
      graph.resize(0);
      initialEstimatePose.clear();
      initialEstimateVector.clear();
    }
  }

    return 0;
}