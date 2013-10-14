// -*- C++ -*-
/*!
 * @file  Master.cpp
 * @brief Mind and commander
 * @date $Date$
 *
 * $Id$
 */

#include "Master.h"

// Module specification
// <rtc-template block="module_spec">
static const char* master_spec[] =
  {
    "implementation_id", "Master",
    "type_name",         "Master",
    "description",       "Mind and commander",
    "version",           "0.0.1",
    "vendor",            "UDEM",
    "category",          "Administrative",
    "activity_type",     "PERIODIC",
    "kind",              "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",
    "conf.default.int_exec_delay", "50000",
    ""
  };
// </rtc-template>

/*!
 * @brief constructor
 * @param manager Maneger Object
 */
Master::Master(RTC::Manager* manager)
    // <rtc-template block="initializer">
  : RTC::DataFlowComponentBase(manager),
    m_p_status_visionIn("Status_vision", m_p_status_vision),
    m_p_result_visionIn("Result_vision", m_p_result_vision),
    m_p_status_kinematicsIn("Status_kinematics", m_p_status_kinematics),
    m_p_result_kinematicsIn("Result_kinematics", m_p_result_kinematics),
    m_p_status_hardwareIn("Status_hardware", m_p_status_hardware),
    m_p_option_visionOut("Option_vision", m_p_option_vision),
    m_p_target_kinematicsOut("Target_kinematics", m_p_target_kinematics),
    m_p_state_hardwareOut("Position_hardware", m_p_state_hardware)

    // </rtc-template>
{
}

/*!
 * @brief destructor
 */
Master::~Master()
{
}



RTC::ReturnCode_t Master::onInitialize()
{
  // Registration: InPort/OutPort/Service
  // <rtc-template block="registration">
  // Set InPort buffers
  addInPort("Status_vision", m_p_status_visionIn);
  addInPort("Result_vision", m_p_result_visionIn);
  addInPort("Status_kinematics", m_p_status_kinematicsIn);
  addInPort("Result_kinematics", m_p_result_kinematicsIn);
  addInPort("Status_hardware", m_p_status_hardwareIn);
  
  // Set OutPort buffer
  addOutPort("Option_vision", m_p_option_visionOut);
  addOutPort("Target_kinematics", m_p_target_kinematicsOut);
  addOutPort("Position_hardware", m_p_state_hardwareOut);
  
  // Set service provider to Ports
  
  // Set service consumers to Ports
  
  // Set CORBA Service Ports
  
  // </rtc-template>

  // <rtc-template block="bind_config">
  bindParameter("int_exec_delay", m_int_exec_delay, "50000");
  // </rtc-template>
  
  return RTC::RTC_OK;
}


RTC::ReturnCode_t Master::onFinalize()
{
  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t Master::onStartup(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t Master::onShutdown(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/


RTC::ReturnCode_t Master::onActivated(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}


RTC::ReturnCode_t Master::onDeactivated(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}


RTC::ReturnCode_t Master::onExecute(RTC::UniqueId ec_id)
{

	if (m_p_status_hardwareIn.isNew())
	{
		m_p_status_hardwareIn.read();
		std::cout << "Received hardwareIn: " << std::endl << m_p_status_hardware.data << std::endl;
	}

	if (m_p_result_visionIn.isNew())
	{
		m_p_result_visionIn.read();
		std::cout << "Received visionIn: " << std::endl << m_p_result_vision.data << std::endl;
	}

	if (m_p_result_kinematicsIn.isNew())
	{
		m_p_result_kinematicsIn.read();
		std::cout << "Received kinematicsIn: " << std::endl << m_p_result_kinematics.data << std::endl;
	}

  coil::usleep(1000);

  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t Master::onAborting(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t Master::onError(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t Master::onReset(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t Master::onStateUpdate(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t Master::onRateChanged(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/



extern "C"
{
 
  void MasterInit(RTC::Manager* manager)
  {
    coil::Properties profile(master_spec);
    manager->registerFactory(profile,
                             RTC::Create<Master>,
                             RTC::Delete<Master>);
  }
  
};


