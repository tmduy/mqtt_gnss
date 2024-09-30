/*
 * @author Duy Tran
 * @date 2024-09-30
*/

#ifndef __GNSS_SENDER_H__
#define __GNSS_SENDER_H__

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include <iostream>
#include <string>
#include <mosquitto.h>
#include <ctime>        // For std::time and std::gmtime
#include <cstring>
#include <cstdlib>      // For std::rand
#include <chrono>
#include <thread>
#include <iomanip>      // For std::setw and std::setfill
#include <sstream>      // For std::stringstream

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Function declarations
 **********************************************************************************************************************/
std::string generateGNSSData();
void on_publish(struct mosquitto *mosq, void *obj, int mid);
void gnssDataHandler(struct mosquitto *mosq);

#endif // __GNSS_SENDER_H__