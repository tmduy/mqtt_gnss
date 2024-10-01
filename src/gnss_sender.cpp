/*
 * @author Duy Tran
 * @date 2024-10-01
*/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "../inc/gnss_sender.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define QOS_LEVEL               (0U)              /* QoS level 0 is applied in this project */
#define LATITUDE_DEGREE_MAX     (90U)             /* Maximum value for latitude degrees */
#define LONGITUDE_DEGREE_MAX    (180U)            /* Maximum value for longitude degrees */
#define MINUTES_IN_DEGREE       (60U)             /* Conversion from degrees to minutes */
#define PRECISION_FACTOR        (1000000U)        /* Factor for generating random precision */

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables and functions
 **********************************************************************************************************************/
static std::string getFormattedTime(const std::tm* timeStruct);
static std::string getFormattedDate(const std::tm* timeStruct);
static std::string calculateChecksum(const std::string& sentence);

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief Generates GNSS data in NMEA format.
 * 
 * @return A string containing the NMEA GPRMC sentence.
 **********************************************************************************************************************/
std::string generateGNSSData ()
{
    // Get current timestamp
    std::time_t t = std::time(0);
    std::tm* now = std::gmtime(&t);

    std::string utc = getFormattedTime(now);
    std::string date = getFormattedDate(now);

    // Generate random latitude and longitude values
    double latVal = (rand() % LATITUDE_DEGREE_MAX) + 
                    ((double)(rand() % PRECISION_FACTOR)) / PRECISION_FACTOR;
    double longVal = (rand() % LONGITUDE_DEGREE_MAX) + 
                     ((double)(rand() % PRECISION_FACTOR)) / PRECISION_FACTOR;

    // Convert latitude and longitude to degrees and minutes
    int latDegrees = (int)latVal;
    double latMinutes = (latVal - latDegrees) * MINUTES_IN_DEGREE;

    int longDegrees = (int)longVal;
    double longMinutes = (longVal - longDegrees) * MINUTES_IN_DEGREE;

    // Randomly assign latitude, longitude, and magnetic variation directions
    char latDirection = (rand() % 2 == 0) ? 'N' : 'S';
    char lonDirection = (rand() % 2 == 0) ? 'E' : 'W';
    char varDirection = (rand() % 2 == 0) ? 'E' : 'W';

    // Create the full NMEA GPRMC sentence
    std::string nmeaData = "$GPRMC,";
    nmeaData += utc + ",";
    nmeaData += "A,";  // Fixed to active status (A = data valid, V = data invalid)
    nmeaData += std::to_string(latDegrees) + std::to_string(latMinutes) + "," + latDirection + ",";
    nmeaData += std::to_string(longDegrees) + std::to_string(longMinutes) + "," + lonDirection + ",";
    nmeaData += "0.0,0.0,";  // Speed and course over ground
    nmeaData += date + ",";
    nmeaData += "0.0,";  // Fixed magnetic variation degree to 0.0
    nmeaData += varDirection;
    nmeaData += ",A";  // Fixed Positioning system mode indicator to "A" (Autonomous)
    nmeaData += "*" + calculateChecksum(nmeaData);  // Append the checksum

    return nmeaData;
}

/*******************************************************************************************************************//**
 * @brief Callback function called when the message is published successfully.
 * 
 * @param mosq Pointer to the Mosquitto instance.
 * @param obj Pointer to user-defined object (not used).
 * @param mid Message ID of the published message.
 **********************************************************************************************************************/
void on_publish (struct mosquitto *mosq, void *obj, int mid)
{
    std::cout << "Message with mid " << mid << " published successfully." << std::endl;
}

/*******************************************************************************************************************//**
 * @brief Handles GNSS data and publishes it using MQTT.
 * 
 * @param mosq Pointer to the Mosquitto instance.
 **********************************************************************************************************************/
void gnssDataHandler (struct mosquitto *mosq)
{
    std::string gnssData = generateGNSSData();

    const char* topic = "gnss/data";
    int ret = mosquitto_publish(mosq, NULL, topic, gnssData.size(), gnssData.c_str(), QOS_LEVEL, false);

    if (ret != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed to publish GNSS data, error: " << ret << std::endl;
    }
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief Converts a given time to a string in the format of "HHMMSS".
 * 
 * @param timeStruct A pointer to a tm structure representing the current time.
 * @return A string representing the time in "HHMMSS" format.
 **********************************************************************************************************************/
static std::string getFormattedTime (const std::tm* timeStruct)
{
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << timeStruct->tm_hour
       << std::setw(2) << std::setfill('0') << timeStruct->tm_min
       << std::setw(2) << std::setfill('0') << timeStruct->tm_sec
       << ".00";
    return ss.str();
}

/*******************************************************************************************************************//**
 * @brief Converts a given date to a string in the format of "DDMMYY".
 * 
 * @param timeStruct A pointer to a tm structure representing the current time.
 * @return A string representing the date in "DDMMYY" format.
 **********************************************************************************************************************/
static std::string getFormattedDate (const std::tm* timeStruct)
{
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << timeStruct->tm_mday
       << std::setw(2) << std::setfill('0') << (timeStruct->tm_mon + 1)
       << std::setw(2) << std::setfill('0') << (timeStruct->tm_year % 100);
    return ss.str();
}

/*******************************************************************************************************************//**
 * @brief Calculate the checksum for an NMEA sentence.
 * 
 * @param sentence The NMEA sentence to calculate the checksum for.
 * @return The checksum in hexadecimal string format.
 **********************************************************************************************************************/
static std::string calculateChecksum (const std::string& sentence)
{
    unsigned char checksum = 0;
    for (size_t i = 1; i < sentence.length(); ++i)
    {
        checksum ^= sentence[i];
    }

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)checksum;
    return ss.str();
}

/***********************************************************************************************************************
 * Main function
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief Entry point of the GNSS sender application.
 * 
 * Initializes the Mosquitto library and client, connects to the MQTT broker, and periodically publishes GNSS data to
 * a specified topic.
 * 
 * @param argc Argument count (not used).
 * @param argv Argument vector (not used).
 * 
 * @return Exit status code (0 for success, -1 for failure).
 **********************************************************************************************************************/
int main (int argc, char* argv[])
{
    // Initialize the Mosquitto library
    mosquitto_lib_init();

    // Create a new Mosquitto client instance
    struct mosquitto *mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq)
    {
        std::cerr << "Failed to create Mosquitto instance!" << std::endl;
        return 1;
    }

    // Set the publish callback function
    mosquitto_publish_callback_set(mosq, on_publish);

    // Connect to the MQTT broker
    if (mosquitto_connect(mosq, "localhost", 1883, 60))
    {
        std::cerr << "Unable to connect to the MQTT broker!" << std::endl;
        return 1;
    }

    // Publish GNSS data periodically
    for (int i = 0; i < 5; ++i)
    {
        gnssDataHandler(mosq);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Cleanup and destroy the Mosquitto client instance
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
