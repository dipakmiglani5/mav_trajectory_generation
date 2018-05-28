/**
 * @file offb_node.cpp
 * @brief Offboard control example node, written with MAVROS version 0.19.x, PX4 Pro Flight
 * Stack and tested in Gazebo SITL
 */

#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <math.h>

#define PI 3.141

mavros_msgs::State current_state;
void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "xy_offb_node");
    ros::NodeHandle nh;

    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>
            ("mavros/state", 10, state_cb);
    ros::Publisher local_pos_pub = nh.advertise<geometry_msgs::PoseStamped>
            ("mavros/setpoint_position/local", 10);
    ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool>
            ("mavros/cmd/arming");
    ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>
            ("mavros/set_mode");

    //the setpoint publishing rate MUST be faster than 2Hz
    ros::Rate rate(20.0);

    // wait for FCU connection
    while(ros::ok() && !current_state.connected){
        ros::spinOnce();
        rate.sleep();
    }

    float theta = 0.0;

    geometry_msgs::PoseStamped pose;
    pose.pose.position.x = 0;
    pose.pose.position.y = 0;
    pose.pose.position.z = 2;

    // Vector
    // pose.pose.orientation.x = 0;
    // pose.pose.orientation.y = 0;
    // pose.pose.orientation.z = 0;

    //Scaler
    // pose.pose.orientation.w = 0;

    //send a few setpoints before starting
    for(int i = 250; ros::ok() && i > 0; --i){
        local_pos_pub.publish(pose);
        ros::spinOnce();
        rate.sleep();
    }

    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.custom_mode = "OFFBOARD";

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;

    ros::Time last_request = ros::Time::now();

    bool flag1 = false;
    bool flag2 = false;
    
    while(ros::ok()){

        if( !current_state.armed &&
                (ros::Time::now() - last_request > ros::Duration(5.0))){
                if( arming_client.call(arm_cmd) &&
                    arm_cmd.response.success){
                    ROS_INFO("Vehicle armed");
                    flag2  =  true;
                }
                last_request = ros::Time::now();
            } else {
            if( current_state.mode != "OFFBOARD" &&
            (ros::Time::now() - last_request > ros::Duration(5.0))){
            if( set_mode_client.call(offb_set_mode) &&
                offb_set_mode.response.mode_sent){
                ROS_INFO("Offboard enabled");
                flag1  = true;
            }
            last_request = ros::Time::now();
            
        }
    }

        if (flag1 == true && flag2 == true){
                pose.pose.position.x = theta;
                pose.pose.position.y = 6*sin(theta*PI/6);
                // theta += 1.0/12 ;
                theta += 1.0/100 ;
                // pose.pose.position.x = 0;
                // pose.pose.position.y = 0;
                // pose.pose.orientation.z = sin(theta*PI/2);
                // pose.pose.orientation.w = cos(theta*PI/2);

                // theta += 1.0/90;


                //ROS_INFO("Value of w = %f, Value of z = %f" , pose.pose.orientation.w, pose.pose.orientation.z);
                ROS_INFO("Value of x = %f, Value of y = %f" , pose.pose.position.x, pose.pose.position.y);

                local_pos_pub.publish(pose);

        }

       

        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}

