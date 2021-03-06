#include <webots/Supervisor.hpp>
#include <iostream>
#include <ctime>
#include <fstream>
#include <sys/time.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include "GaitControl.hpp"
#include "robotSim.hpp"


#include <webots_ros/get_float.h>
#include <webots_ros/get_int.h>
#include <webots_ros/set_bool.h>
#include <webots_ros/set_float.h>
#include <webots_ros/set_int.h>

#include <webots_ros/Int8Stamped.h>
#include <webots_ros/Int32Stamped.h>
#include "webots_ros/Float64ArrayStamped.h"

#include "Ros.hpp"

#include <std_msgs/Header.h>
#include <geometry_msgs/PoseStamped.h>
#include <sensor_msgs/Joy.h>



#define TIME_STEP   10    //ms
#define FREQUENCY   0.3   //Hz
#define NUM_MOTORS   18

extern "C" {
  int wb_robot_init();
  int wb_robot_cleanup();
}


using namespace std;
using namespace webots;

class RosKrock : public Ros {
  public :
    RosKrock();
    virtual ~RosKrock();

  protected :
    virtual void setupRobot();
    virtual void setRosDevices(const char **hiddenDevices, int numberHiddenDevices);
    virtual void launchRos(int argc,char **argv);
    virtual int step(int duration); // the argument of this method is ignored

  private :
    // very useful static cast to later on use methods from class RobotSim
    RobotSim * robotSim() { return static_cast<RobotSim *>(mRobot); }

    // ROS subscribers/publishers
    ros::Publisher kPosePublisher;
    //ros::Publisher kPosePublisherGPS;
    ros::Publisher kTorquesFeedbackPublisher;
    ros::Publisher kTouchSensorsPublisher;

    ros::Subscriber kGaitChooserSubscriber;
    ros::Subscriber kSpawnPoseSubscriber;
    ros::Subscriber kKeyboardKeySubscriber;
    ros::Subscriber kJoySubscriber;

    //ros::ServiceServer kSpawnPoseSubscriber;

    // ROS callbacks
    void setGaitCallback(const webots_ros::Int8Stamped::ConstPtr& msg);
    void setSpawnPoseCallback(const geometry_msgs::PoseStamped::ConstPtr& msg);
    //bool setSpawnPoseService(const geometry_msgs::PoseStamped::ConstPtr& msg);
    void manualControlCallback(const webots_ros::Int32Stamped::ConstPtr& msg);
    void joyControlCallback(const sensor_msgs::Joy::ConstPtr& msg);

    bool selectGait(int index);

    GaitControl *controller; // this class is called controller but does not really do that, it's mostly a logger and a sort of variable updater

    float t, dt;
    int controller_mode; // 0 auto, 1 keyboard
    float freq, freq_left, freq_right;
    double  angref[NUM_MOTORS],
            feedbackAngles[NUM_MOTORS],
            feedbackTorques[NUM_MOTORS],
            rollPitchYaw[3],
            gpsDataFront[3],
            gpsDataHind[3],
            feetTouchSensors[12]; // 3D force sensor readings from eahc foot
    const double freq_offset;
    char *gait_files[4];
    int current_gait_idx;
    ofstream gpsLog;
    ofstream rpyLog;
    ofstream anglesLog;
    ofstream torquesLog;

};

RosKrock::RosKrock() :
        gait_files{"./gaits/angles_stop.txt","./gaits/angles_normal.txt","./gaits/angles_high.txt","./gaits/angles_low.txt"},
        current_gait_idx(0),
        controller_mode(0),
        freq(FREQUENCY),
        freq_left(0),
        freq_right(0),
        freq_offset(0.8)
{
    t = 0.;
    dt = TIME_STEP/1000.; // no idea why they do that instead of using TIME_STEP, ask Tomislav

    //gait_files =

    gpsLog.open("./logData/gpsLog.txt");
    rpyLog.open("./logData/rpyLog.txt");
    anglesLog.open("./logData/anglesLog.txt");
    torquesLog.open("./logData/torquesLog.txt");

}

RosKrock::~RosKrock(){
    wb_robot_cleanup();
    kPosePublisher.shutdown();
    ros::shutdown();
    delete mRobot;
}





void RosKrock::setGaitCallback(const webots_ros::Int8Stamped::ConstPtr& msg){
    //ROS_INFO ("New gait option received: %d", msg->data);
    int new_gait_idx = msg->data;

    selectGait(new_gait_idx);
}

bool RosKrock::selectGait(int new_gait_idx){
    if (new_gait_idx >=0 && new_gait_idx <=3){
        if (new_gait_idx != current_gait_idx){
            ROS_INFO ("Changing gait to: %s", gait_files[new_gait_idx]);
            controller = new GaitControl(FREQUENCY, gait_files[new_gait_idx]);
            current_gait_idx = new_gait_idx;
        }
        else{
            ROS_INFO ("Selected gait is the same as current!");
        }
        return true;
    }
    else{
        ROS_INFO ("Wrong gait index. Current gait kept: %s", gait_files[current_gait_idx]);
    }
    return false;
}


void RosKrock::setSpawnPoseCallback(const geometry_msgs::PoseStamped::ConstPtr& msg){
    ROS_INFO ("New spawn pose received: ");

    Node *node_robot = robotSim()->getFromDef("ROBOT");

    Field* translationField;
    Field* orientationField;

    translationField = node_robot->getField("translation");
    orientationField = node_robot->getField("rotation");

    const double translationValues [3]= {msg->pose.position.x,msg->pose.position.y,msg->pose.position.z};
    const double orientationValues [4]= {msg->pose.orientation.x,msg->pose.orientation.y,msg->pose.orientation.z,msg->pose.orientation.w};

    ROS_INFO ("New spawn pose received. T [%f,%f,%f] R [%f,%f,%f,%f]",translationValues[0],translationValues[1],translationValues[2],orientationValues [0],orientationValues [1],orientationValues [2],orientationValues [3]);


    //
    // SUPERVISOR_NEEDED?
    //
    // get the robot node to set the translantion and rotation
    // is it possible to do it without the superviosr capabilities?

    translationField->setSFVec3f(translationValues);
    orientationField->setSFRotation(orientationValues);

}

//
// How do we declare a service in webots_ros to spanw our robot
// instead of using a topic callback?
//

/*
bool RosKrock::setSpawnPoseService(const geometry_msgs::PoseStamped::ConstPtr& msg){
    ROS_INFO ("New spawn pose received: ");

    Node *node_robot = robotSim()->getFromDef("ROBOT");

    Field* translationField;
    Field* orientationField;

    translationField = node_robot->getField("translation");
    orientationField = node_robot->getField("rotation");

    const double translationValues [3]= {msg->pose.position.x,msg->pose.position.y,msg->pose.position.z};
    const double orientationValues [4]= {msg->pose.orientation.x,msg->pose.orientation.y,msg->pose.orientation.z,msg->pose.orientation.w};

    ROS_INFO ("New spawn pose received. T [%f,%f,%f] R [%f,%f,%f,%f]",translationValues[0],translationValues[1],translationValues[2],orientationValues [0],orientationValues [1],orientationValues [2],orientationValues [3]);

    translationField->setSFVec3f(translationValues);
    orientationField->setSFRotation(orientationValues);

    return true;
}
*/

void RosKrock::manualControlCallback(const webots_ros::Int32Stamped::ConstPtr& msg){
    ROS_INFO("From keyboard: %d", msg->data);
    int key = msg->data;
    float offset = 0.5;
    switch(key)
    {
    case 314: // left
        freq_left = freq - offset;
        freq_right = freq + offset;
        break;
    case 316: // right
        freq_left = freq + offset;
        freq_right = freq - offset;
        break;
    case 315: // up
        freq_left = freq + offset;
        freq_right = freq + offset;
        break;
    case 317: //down ??
        freq_left = freq - offset;
        freq_right = freq - offset;
        break;
    case 312: // mode
        controller_mode += 1;
        controller_mode = (controller_mode%2);
        ROS_INFO("MODE TOGGLE: %d", controller_mode);
        break;
    case 0: // Gait selection
        selectGait(0);
        break;
    case 1:
        selectGait(1);
        break;
    default:
        ROS_INFO("unknown key: %d",key);
        break;
    }

}

void RosKrock::joyControlCallback(const sensor_msgs::Joy::ConstPtr& msg){
    vector<float> axes = msg->axes;
    vector<int> buttons = msg->buttons;
    if (buttons[0] == 1){ // toggle mode
        controller_mode += 1;
        controller_mode = (controller_mode%2);
        ROS_INFO("MODE TOGGLE: %d", controller_mode);
    }
    if (buttons[1] == 1){ // toggle gait
        int new_gait_idx = current_gait_idx+1;
        new_gait_idx = (new_gait_idx%4);
        selectGait(new_gait_idx);
        ROS_INFO("GAIT TOGGLE: %d", current_gait_idx);
    }
    if (controller_mode==1){
        freq_left = (freq_offset * axes[3]) - (freq_offset * axes[2]);
        freq_right = (freq_offset * axes[3]) + (freq_offset * axes[2]);
    }
}

void RosKrock::setupRobot(){
    wb_robot_init();
    mRobot = new RobotSim(TIME_STEP);
    if (mRobot->getType() == Node::SUPERVISOR){
        cout << "This robot is a supervisor" << endl;
    }
    cout<<"RobotSIM CREATED"<<endl;
    controller = new GaitControl(FREQUENCY, gait_files[current_gait_idx]);
}

void RosKrock::setRosDevices(const char **hiddenDevices, int numberHiddenDevices){

    const char * krock_devices[46] = {
        "FLpitch_motor", "FLyaw_motor", "FLroll_motor", "FLknee_motor",
        "FRpitch_motor", "FRyaw_motor", "FRroll_motor", "FRknee_motor",
        "HLpitch_motor", "HLyaw_motor", "HLroll_motor", "HLknee_motor",
        "HRpitch_motor", "HRyaw_motor", "HRroll_motor", "HRknee_motor",
        "spine1_motor", "spine2_motor",
        "neck1_motor", "tail1_motor", "tail2_motor",
        "FLpitch_sensor", "FLyaw_sensor", "FLroll_sensor", "FLknee_sensor",
        "FRpitch_sensor", "FRyaw_sensor", "FRroll_sensor", "FRknee_sensor",
        "HLpitch_sensor", "HLyaw_sensor", "HLroll_sensor", "HLknee_sensor",
        "HRpitch_sensor", "HRyaw_sensor", "HRroll_sensor", "HRknee_sensor",
        "spine1_sensor", "spine2_sensor",
        "neck1_sensor", "tail1_sensor", "tail2_sensor",
        "fl_touch", "fr_touch", "hl_touch", "hr_touch",
    };

    Ros::setRosDevices(krock_devices, 46);

}

void RosKrock::launchRos(int argc, char **argv){
    // call the standard ros class setup and services configuration
    // there are too many that are not useful for our case, we can rewrite this
    // method to avoid having a pletora of services that we do not need
    Ros::launchRos(argc, argv);

    // our services

    // our subscribers
    kGaitChooserSubscriber = nodeHandle()->subscribe("/krock/gait_chooser",10, &RosKrock::setGaitCallback, this);
    kSpawnPoseSubscriber = nodeHandle()->subscribe("/krock/spawn_pose",10, &RosKrock::setSpawnPoseCallback, this);
    kKeyboardKeySubscriber = nodeHandle()->subscribe("/krock/keyboard/key",10, &RosKrock::manualControlCallback, this);
    kJoySubscriber = nodeHandle()->subscribe("/joy",10, &RosKrock::joyControlCallback, this);

    // our publishers
    kPosePublisher = nodeHandle()->advertise<geometry_msgs::PoseStamped>(name()+"/pose", 1);
    //kPosePublisherGPS = nodeHandle()->advertise<geometry_msgs::PoseStamped>(name()+"/pose_gps", 1);
    kTorquesFeedbackPublisher = nodeHandle()->advertise<webots_ros::Float64ArrayStamped>(name()+"/torques_feedback", 1);
    kTouchSensorsPublisher = nodeHandle()->advertise<webots_ros::Float64ArrayStamped>(name()+"/touch_sensors", 1);

    // Services
    //
    // HOW TO DECLARE A SERVICE? Look in the manual/API
    //

    //kSpawnPoseSubscriber = nodeHandle()->advertiseService("spawn_krock", &RosKrock::setSpawnPoseService);


}


int RosKrock::step(int duration){

    //ROS_INFO ("Running step func with duration %d", duration);

    // controller calls
    switch(controller_mode){
        case 0: // auto
            controller->setTimeStep(dt);
            controller->runStep();
            controller->getAngles(angref);
            break;
        case 1: // keyboard
            controller->setTimeStep(dt);
            controller->runStep(freq_left, freq_right);
            controller->getAnglesManual(angref);
            break;
    };

    // send new joint angles to the robot
    robotSim()->setAngles(angref);

    // get feedback from the robot
    robotSim()->GetIMU(rollPitchYaw);
    robotSim()->GetPosition(gpsDataFront, gpsDataHind);
    robotSim()->getPositionTorques(feedbackAngles, feedbackTorques);
    robotSim()->ReadTouchSensors(feetTouchSensors);


    // NOTE: This is the way the native robot controllers retrieves data of pose, joints, torques

    // gpsDataFront stores the same information as the data in transition,rotation fields of the node ROBOT.
    //cout << "Front position(x,y,z): " << gpsDataFront[0] << gpsDataFront[1] << gpsDataFront[2] << endl;
    //cout << "Orientation(roll, pitch, yaw): " << rollPitchYaw[0] << rollPitchYaw[1] << rollPitchYaw[2] << endl;

    std::vector<double> tmp_torques;
    std::vector<double> tmp_touchs;

    // LOG DATA
    gpsLog << t << "\t";
    for(int i=0; i<3; i++){
        gpsLog << gpsDataFront[i] << "\t";
    }
    gpsLog << endl;

    rpyLog << t << "\t";
    for(int i=0; i<3; i++){
        rpyLog << rollPitchYaw[i] << "\t";
    }
    rpyLog << endl;

    anglesLog << t << "\t";
    for(int i=0; i<NUM_MOTORS; i++){
        anglesLog << feedbackAngles[i] << "\t";
    }
    anglesLog << endl;

    // torques
    torquesLog << t << "\t";
    for(int i=0; i<NUM_MOTORS; i++){
        tmp_torques.push_back(feedbackTorques[i]);
        torquesLog << feedbackTorques[i] << "\t";
    }
    torquesLog << endl;

    // touch sensors (3D force readings per foot)
    for(int i=0; i<12; i++){
        tmp_touchs.push_back(feetTouchSensors[i]);
    }


    // NOTE: This is the way to extract pose information from webots API (compared with way above)

    /*SUPERVISOR_NEEDED?*/
    // We obtain the current pose of the robot using the supervisor functions of the
    // native controller
    Node *node_robot = robotSim()->getFromDef("ROBOT");
    geometry_msgs::PoseStamped pose_krock;// = new geometry_msgs::PoseStamped();
    //double stime = robotSim()->getTime();

    webots_ros::Float64ArrayStamped torques_krock;
    webots_ros::Float64ArrayStamped touch_sensors_krock;


    if (node_robot){
        // Populating pose message
        Field* translationField;
        Field* orientationField;
        pose_krock.header.frame_id= "world";
        pose_krock.header.stamp = ros::Time::now();//

        translationField = node_robot->getField("translation");
        const double* translationValues = translationField->getSFVec3f();
        pose_krock.pose.position.x = translationValues[0];
        pose_krock.pose.position.y = translationValues[1];
        pose_krock.pose.position.z = translationValues[2];

        orientationField = node_robot->getField("rotation");
        const double* orientationValues = orientationField->getSFRotation();
        pose_krock.pose.orientation.x = orientationValues[0];
        pose_krock.pose.orientation.y = orientationValues[1];
        pose_krock.pose.orientation.z = orientationValues[2];
        pose_krock.pose.orientation.w = orientationValues[3];

        //cout << translationValues[0] << translationValues[1] << translationValues[2] << orientationValues[0] << orientationValues[1] << orientationValues[2]<< orientationValues[3] << endl;

        // Populating torques feedback message
        torques_krock.header.frame_id = "world"; // torque information does not depend on the frame
        torques_krock.header.stamp = ros::Time::now();

        torques_krock.data = tmp_torques;

        // Populating touch sensor readings
        touch_sensors_krock.header.frame_id = "world"; // from the ROS side (or maybe from webots side) we need to generate tf links and messages with a proper frame_ide for each foot, hecen each force vector would have the right frame_ide
        touch_sensors_krock.header.stamp = ros::Time::now();

        touch_sensors_krock.data = tmp_touchs;

        kPosePublisher.publish(pose_krock);
        kTorquesFeedbackPublisher.publish(torques_krock);
        kTouchSensorsPublisher.publish(touch_sensors_krock);

    }

    t+=dt;

    return robotSim()->step(TIME_STEP);
}


//############################ MAIN PROGRAM ###########################################################

int main(int argc, char **argv)
{
    cout<<"MAIN STARTS"<<endl;

    RosKrock *krock = new RosKrock();
    krock->run(argc, argv);

    delete krock;

  return 0;
}
