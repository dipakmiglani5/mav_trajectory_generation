#include <mav_trajectory_generation/polynomial_optimization_linear.h>
#include <mav_trajectory_generation/trajectory.h>
#include <mav_trajectory_generation_ros/ros_visualization.h>
#include <geometry_msgs/PoseArray.h>

geometry_msgs::PoseArray way_;
void waycb(const geometry_msgs::PoseArray::ConstPtr &msg)
{
    way_ = *msg;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "waypoint_node");
    ros::NodeHandle n;

    ros::Subscriber way_sub = n.subscribe<geometry_msgs::PoseArray>("/waypoints", 10, waycb);

    ros::Publisher vis_pub = n.advertise<visualization_msgs::MarkerArray>("trajectory_traject", 10);

    ros::Rate sleep_rate(10);

    while (ros::ok())
    {   
        

        mav_trajectory_generation::Vertex::Vector vertices;
        const int dimension = 3;
        const int derivative_to_optimize = mav_trajectory_generation::derivative_order::ACCELERATION;
        mav_trajectory_generation::Vertex start(dimension), middle(dimension), end(dimension);

        // Time count
        ros::Time t0 = ros::Time::now();

        if(way_.poses.size()>0)
        {
            start.makeStartOrEnd(Eigen::Vector3d(way_.poses[0].position.x, way_.poses[0].position.y,0), derivative_to_optimize);
            vertices.push_back(start);

                std::cout << "debug="  << "," << way_.poses.size() << std::endl;
            for(int ii=1;ii < way_.poses.size()-1; ii++ )
            {
    	        middle.addConstraint(mav_trajectory_generation::derivative_order::POSITION, Eigen::Vector3d(way_.poses[ii].position.x, way_.poses[ii].position.y, 0));
                vertices.push_back(middle);
            }

            end.makeStartOrEnd(Eigen::Vector3d(way_.poses[way_.poses.size() - 1].position.x, way_.poses[way_.poses.size() - 1].position.y,0), derivative_to_optimize);
            vertices.push_back(end);
        
            //compute the segment times
            std::vector<double> segment_times;
            const double v_max = 1.0;
            const double a_max = 3.0;
            const double magic_fabian_constant = 6.5; // A tuning parameter
            segment_times = estimateSegmentTimes(vertices, v_max, a_max, magic_fabian_constant);


            //N denotes the number of coefficients of the underlying polynomial
            //N has to be even.
            //If we want the trajectories to be snap-continuous, N needs to be at least 10.

            const int N = 10;
            mav_trajectory_generation::PolynomialOptimization<N> opt(dimension);
            opt.setupFromVertices(vertices, segment_times, derivative_to_optimize);
            opt.solveLinear();

            //ROS_INFO("Take %f sec to get optimal traject", (ros::Time::now() - t0).toSec());

            //Obtain the polynomial segments
            mav_trajectory_generation::Segment::Vector segments;
            opt.getSegments(&segments);

            //creating Trajectories
            
            mav_trajectory_generation::Trajectory trajectory;
            opt.getTrajectory(&trajectory);
            
            //evaluating the trajectory at particular instances of time
            // Single sample:
            double sampling_time = 2.0;
            int derivative_order = mav_trajectory_generation::derivative_order::POSITION;
            Eigen::VectorXd sample = trajectory.evaluate(sampling_time, derivative_order);

            // Sample range:
            double t_start = 2.0;
            double t_end = 10.0;
            double dt = 0.01;
            std::vector<Eigen::VectorXd> result;
            std::vector<double> sampling_times; // Optional.
            trajectory.evaluateRange(t_start, t_end, dt, derivative_order, &result, &sampling_times);
            //std::cout << result.size() << std::endl;
            //std::cout << sampling_times.size() << std::endl;

            //visualizing Trajectories
            visualization_msgs::MarkerArray markers;
            double distance = 1.6; // Distance by which to seperate additional markers. Set 0.0 to disable.
            std::string frame_id = "world";
            // From Trajectory class:
            mav_trajectory_generation::drawMavTrajectory(trajectory, distance, frame_id, &markers);


        // for (int i = 0; i < segments.size(); i++)
        // {
        //     std::cout << i << "," << segments[i] << "," <<  segments.size() << std::endl;
        // }
        // std::cout << result[1] << std::endl;
        

        vis_pub.publish(markers);
    }
        ros::spinOnce();
        vertices.clear();
        sleep_rate.sleep();
    }
    return 0;
}