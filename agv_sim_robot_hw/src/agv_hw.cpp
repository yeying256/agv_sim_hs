#include "agv_hw.h"
#include "stdio.h"
#include "zmotion.h"
#include "zmcaux.h"
#include "ros/ros.h"
#include "agv_msg/grab_agv.h"



namespace xj_control_ns
{
    Agv_hw_interface::Agv_hw_interface(/* args */)
    {
    }
    
    Agv_hw_interface::~Agv_hw_interface()
    {
    }

    void Agv_hw_interface::commandCheckHandler(const char *command, int ret)//检查指令执行结果
    {
        if (ret)//非 0 则失败
        {   
            printf("%s return code is %d\n", command, ret);
        }   
    }   
    
    //夹爪服务器
    bool Agv_hw_interface::Grab_Server(agv_msg::grab_agv::Request& req,
                    agv_msg::grab_agv::Request& resp){
        float high_unit = req.high;//将几何参数转换为用户单位输入
        float width_unit = req.width;//将几何参数转换为用户单位输入
        this->status_ = req.status;
        ZMC_HANDLE handle = NULL;
        ROS_INFO("夹爪收到的参数为:high=%.2f,width=%.2f")

        

        if((status_ % 2) == 0){
            ROS_INFO("夹爪处于运动状态");
            int retSGP1 =ZAux_Direct_Single_MoveAbs(handle,5, high);
            int retSGP2 =ZAux_Direct_Single_MoveAbs(handle,6, high);//上升运动电机同步运动
            int retSGP3 =ZAux_Direct_Single_MoveAbs(handle,7, width);//夹取运动电机运动
        }
        else if(status_ == 1){
            
        }
        else{
            ROS_INFO("夹爪处于静止状态");
        }
    }


    int Agv_hw_interface::MyDirect_SetSpeed(ZMC_HANDLE handle, int axis, float fValue)//自定义在线设置AGV电机速度函数
    {
    char cmdbuff[2048];
    char cmdbuffAck[2048];
    if (axis> 4)
    {
    return ERR_AUX_PARAERR;
    }
    sprintf(cmdbuff,"SPEED(%d)=%f",iaxis,fValue);//生成对应命令的字符串
    ZAux_DirectCommand(handle,cmdbuff,cmdbuffAck,2048);
    }

    bool Agv_hw_interface::init(ros::NodeHandle& root_nh, ros::NodeHandle& robot_hw_nh)
    {
        bool ret = robot_hw_nh.getParam("/agv_sim/robot_hw_test/joints", agv_joint_name);//从参数服务器中获取name
        if (ret == true)
        {
            std::cout<<"\033[1;32;40m 获得jointname成功\033[0m"<<std::endl;
        }else
        {
            std::cout<<"\033[1;31;40m 获得jointname失败\033[0m"<<std::endl;
        }
        
        ROS_ERROR("getParam ret= %d",ret);
        
        int agv_num_joints_ = agv_joint_name.size();
        for (size_t i = 0; i < agv_num_joints_; i++)
        {
            ROS_ERROR("jointname=%s",agv_joint_name[i].c_str());
        }
        joint_effort_state.resize(agv_num_joints_);
        joint_position_state.resize(agv_num_joints_);
        joint_velocity_state.resize(agv_num_joints_);
        joint_velocity_command.resize(agv_num_joints_);


        for(int i=0; i<agv_num_joints_; i++)
        {
            //State
            hardware_interface::JointStateHandle jointStateHandle(agv_joint_name[i].c_str(), &joint_position_state[i], &joint_velocity_state[i], &joint_effort_state[i]);
            joint_state_interface.registerHandle(jointStateHandle);
            ROS_INFO("joint_name[%d].c_str()=%s",i,jointStateHandle.getName().c_str());

            //Velocity
            hardware_interface::JointHandle jointVelocityHandle(joint_state_interface.getHandle(agv_joint_name[i].c_str()), &joint_velocity_command[i]);
            velocity_joint_interface.registerHandle(jointVelocityHandle);
        }

        registerInterface(&joint_state_interface);          //将类中的接口注册到ros中
        registerInterface(&velocity_joint_interface);    

//****************************************************************************************************
        ZMC_HANDLE handle = NULL;//连接句柄
        //PCI初始化
        for(int joint=0; joint<7;joint++)
        {
            uint32 cardnumcardnum = 0;
            //连接返回的句柄
            //PCI接口编号
            int retPCI = ZAux_OpenPci(cardnumcardnum, &handle); ////以太网连接控制器以太网连接控制器
            commandCheckHandler("ZAux_OpenPci", retPCI);
            if (ERR_SUCCESS != retPCI)
            {
                printf("PCI 连接失败!\n");
                handle = NULL;
                return 0;
            }
                else
            {
                printf("PCI 连接成功!\n");
            }
            //do something.....
        }

//*********************************************************************************************************
        //EtherCAT初始化使用Zmotion Tools软件进行，用这种方式完成初始化后进行总线轴的使能
        char *ip_addr = (char *)"192.168.0.11";//控制器 IP 地址
        int retOPEN = ZAux_OpenEth(ip_addr, &handle); //连接控制器
        if (ERR_SUCCESS != retOPEN)
        {
            printf("控制器连接失败!\n");
            handle = NULL;
            return -1;
        }
        printf("控制器连接成功！\n");
        ZAux_BusCmd_InitBus(handle);//总线初始化（针对 Zmotion tools 工具软件配置过总线参数控制器使用有效）
        int GetValue;
        int retGBI = ZAux_BusCmd_GetInitStatus(handle, &GetValue); //获取总线初始化状态 0：失败 ；1：成功（只针对 Zmotion tools 工具软件配置过总线参数控制器使用有效）
        commandCheckHandler("ZAux_BusCmd_GetInitStatus", retGBI);
        printf("总线初始化状态= %d \n", GetValue);
        int AxisEnable;
        for(int axis=0;axis<7; axis++)
        {
            int retGAE = ZAux_Direct_GetAxisEnable(handle, axis, &AxisEnable);// 读取轴使能： 0 表示关闭，1表示打开
            commandCheckHandler("ZAux_BusCmd_GetInitStatus", retGAE);
            printf("总线初始化使能状态为： %d \n", AxisEnable);
            if (AxisEnable<1)
            {
                retSAE = ZAux_Direct_SetAxisEnable(handle, axis, 1);// 设置轴使能： 0 表示关闭， 1 表示打开
                commandCheckHandler("ZAux_Direct_SetAxisEnable", retSAE);
                printf("打开使能\n");
            }
        }

        //机器人参数初始化********************************************************************************************
        for(int i=0;i < 7;i++)
        {
        
            if(i==0||i==2){//转向电机参数初始化
                ZAux_Direct_SetAtype(handle, i, 66);//设置各轴的类型为66（EtherCAT总线周期速度模式）
                ZAux_Direct_SetUnits(handle, i, 100); //设置各轴脉冲当量为 100
                ZAux_Direct_SetAccel(handle, i, 2000); //设置各轴加速度为 2000units/s/s
                ZAux_Direct_SetDecel(handle, i, 2000); //设置各轴减速度为 2000units/s/s
                ZAux_Direct_SetHomeWait(handle, i, 1000);//设置各轴回零等待时间
                ZAux_Direct_SetFastDec(handle, i, 3000) //设置转向急停时快减速度为 3000units/s/s
                ZAux_Direct_SetSramp(handle, i, 200); //设置各轴S曲线时间为 200ms
                ZAux_Direct_SetCreep(handle, i, 50);//设置回零时反向爬行速度
                ZAux_Direct_SetHomeWait(handle, i, 1000);//设置轴0回零等待时间
            }
            else if(i==1||i==3){//行走电机参数初始化
                ZAux_Direct_SetAtype(handle, i, 66);//设置各轴的类型为66（EtherCAT总线周期速度模式）
                ZAux_Direct_SetUnits(handle, i, 100); //设置轴 0 脉冲当量为 100
                ZAux_Direct_SetAccel(handle, i, 2000); //设置各轴加速度为 2000units/s/s
                ZAux_Direct_SetDecel(handle, i, 2000); //设置各轴减速度为 2000units/s/s
                ZAux_Direct_SetFastDec(handle, i, 3000) //设置行走急停时快减速度为 3000units/s/s
                ZAux_Direct_SetSramp(handle, i, 200); //设置轴 0 S 曲线时间为 200ms
            }
        //夹爪参数设置
            else{
                ZAux_Direct_SetAtype(handle, i, 65);//设置各轴的类型为66（EtherCAT总线周期位置模式）
                ZAux_Direct_SetUnits(handle, i, 100); //设置各轴脉冲当量为 100
                ZAux_Direct_SetSpeed(handle, i, 200); //设置轴 0 速度为 200units/s
                ZAux_Direct_SetAccel(handle, i, 2000); //设置各轴加速度为 2000units/s/s
                ZAux_Direct_SetDecel(handle, i, 2000); //设置各轴减速度为 2000units/s/s
                ZAux_Direct_SetHomeWait(handle, i, 1000);//设置各轴回零等待时间
                ZAux_Direct_SetFastDec(handle, i, 3000) //设置转向急停时快减速度为 3000units/s/s
                ZAux_Direct_SetSramp(handle, i, 200); //设置各轴S曲线时间为 200ms
                ZAux_Direct_SetCreep(handle, i, 50);//设置回零时反向爬行速度
                ZAux_Direct_SetHomeWait(handle, i, 1000);//设置轴0回零等待时间
            }
            
            
    
        }
        ZAux_Direct_SetDatumIn(handle, 0, 0);//设置转向电机1原点点开关
        ZAux_Direct_SetDatumIn(handle, 2, 1);//设置转向电机2原点点开关
        ZAux_Direct_SetDatumIn(handle, 4, 2);//设置转向电机2原点点开关
        ZAux_Direct_SetDatumIn(handle, 5, 2);//设置转向电机2原点点开关
        ZAux_Direct_SetDatumIn(handle, 6, 3);//设置转向电机2原点点开关

        
        robot_hw_nh.advertiseService("grab_control",Grab_Server);//启动夹爪运动服务器
        ROS_INFO("夹爪机器人服务器已启动！");
        ros::spin();
        return true;
    }

    void Agv_hw_interface::read(const ros::Time& time, const ros::Duration& period)
    {
        setlocale(LC_ALL,"");
        double velocity_unit;
        double position_unit;
        for(int i=0;i<agv_num_joints_;i++)
        {
            int retGS =ZAux_Direct_GetSpeed(handle, i, &velocity_unit); //获取轴的速度(unit/s)
            int retGMPOS = ZAux_Direct_GetMpos(handle, i, &position_unit);//获取反馈位置(unit)
            commandCheckHandler("ZAux_Direct_GetSpeed", retGS);//判断指令是否执行成功
            commandCheckHandler("ZAux_Direct_GetMpos", retGMPOS) ;//判断指令是否执行成功
            if(i==0||i==2){
                joint_velocity_state[i]=velocity_unit;//将用户单位速度转换为电机转速
                joint_position_state[i]=position_unit;//将用户单位位置转换为几何位置
                printf("轴%d的速度 Speed = %lf\n", i, joint_velocity_state[i]);
                printf("轴%d的位置 Mpos = %lf\n", i, joint_position_state[i]);
            }
            else{
                joint_velocity_state[i]=velocity_unit;//将用户单位速度转换为电机转速
                joint_position_state[i]=position_unit;//将用户单位位置转换为几何位置
                printf("轴%d的速度 Speed = %lf\n", i, joint_velocity_state[i]);
                printf("轴%d的位置 Mpos = %lf\n", i, joint_position_state[i]);
            }
        }

    }
    void Agv_hw_interface::write(const ros::Time& time, const ros::Duration& period)
    {
        setlocale(LC_ALL,"");
        status_;
        ZMC_HANDLE handle = NULL;
        double velocity_unit;
        switch (status_)
        {
        case 0:
            ROS_INFO("***急停状态！***");
            for(i=0; i<7; =++){
                int retCM = ZAux_Direct_Single_Cancel(handle, i, 2);
                commandCheckHandler("ZAux_Direct_Single_Cancel", retCM);}
            break;
        case 1:
            ROS_INFO("***回零状态***");
            for(i=0;i<7;i++){
                if(i==0||i==2||3<i<7){
                    ZAux_Direct_SetInvertIn(handle, i, 1);//设置需要回零的输入口电平反转
                    ZAux_Direct_SetDpos( handle, i, 0);//设置需要回零的轴指令位置清0
                    ZAux_Direct_SetMpos( handle,i, 0);//设置需要回零的轴反馈位置清0
                    int retBD = ZAux_BusCmd_Datum(handle, i,3);
                    commandCheckHandler("ZAux_BusCmd_Datum", retBD);
                }
                else{break;}
            }

            while (1)//等待轴 0 回零运动完成
            {
                Sleep(100);
                ZAux_Direct_GetHomeStatus(handle,i,&homestatus);//获取回零运动完成状态
                if (homestatus==1){break;}
            }
            break;
        case 2:
            ROS_INFO("***夹爪与底盘共同运动***");        
            for(int i=0: i<agv_num_joints_; i++){
                if(i==0||i==2){
                    joint_velocity_state=velocity_unit;//转向电机用户单位速度转换为电机转速
                    int retSVS = MyDirect_SetSpeed(handle, i, &joint_velocity_state[i]);
                    commandCheckHandler("MyDirect_SetSpeed", retSVS);
                }
                else{
                    joint_velocity_state=velocity_unit;//行走电机用户单位速度转换为电机转速
                    int retSVS = MyDirect_SetSpeed(handle, i, &joint_velocity_state[i]);
                    commandCheckHandler("MyDirect_SetSpeed", retSVS);
                }
            }
        case 3:
            ROS_INFO("***底盘单独运动***");
            for(int i=0: i<agv_num_joints_; i++){
                if(i==0||i==2){
                    joint_velocity_state=velocity_unit;//转向电机用户单位速度转换为电机转速
                    int retSVS = MyDirect_SetSpeed(handle, i, &joint_velocity_state[i]);
                    commandCheckHandler("MyDirect_SetSpeed", retSVS);
                }
                else{
                    joint_velocity_state=velocity_unit;//行走电机用户单位速度转换为电机转速
                    int retSVS = MyDirect_SetSpeed(handle, i, &joint_velocity_state[i]);
                    commandCheckHandler("MyDirect_SetSpeed", retSVS);
                }
            }
        case 4:
            ROS_INFO("***夹爪单独运动***");        
            for(int i=0: i<agv_num_joints_; i++){
                int retSVS = MyDirect_SetSpeed(handle, i, 0);
                commandCheckHandler("MyDirect_SetSpeed", retSVS);
            }
        default:
            break;
        }
    }

}
PLUGINLIB_EXPORT_CLASS(xj_control_ns::Agv_hw_interface, hardware_interface::RobotHW)