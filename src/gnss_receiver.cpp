/*
 * @author Duy Tran
 * @date 2024-10-01
*/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "../inc/gnss_receiver.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define QOS_LEVEL               (0U)              /* QoS level 0 is applied in this project */

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables and functions
 **********************************************************************************************************************/
static void handle_signal(int signal);

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
std::string receivedMessage = "";  // Global variable to store the received message
std::atomic<bool> running(true);   // Atomic flag for running the loop

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief Initializes the SQLite database.
 * 
 * This function opens an SQLite database and creates a table for GNSS data if it doesn't already exist.
 * 
 * @return Pointer to the SQLite database object, or nullptr if an error occurs.
 **********************************************************************************************************************/
sqlite3* initDatabase ()
{
    sqlite3* db;
    char* errMsg = 0;
    int rc = sqlite3_open("gnss_data.db", &db);

    if (rc)
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    }
    else
    {
        std::cout << "Opened database successfully." << std::endl;
    }

    // Create a table for GNSS data if it doesn't already exist
    const char* sql = "CREATE TABLE IF NOT EXISTS GNSS_DATA("
                      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "NMEA_DATA TEXT NOT NULL);";

    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else
    {
        std::cout << "Table created successfully." << std::endl;
    }

    return db;
}

/*******************************************************************************************************************//**
 * @brief Callback function to handle incoming MQTT messages.
 * 
 * This function is called whenever a message is received from the MQTT broker. It stores the message in the global
 * variable `receivedMessage`.
 * 
 * @param mosq Pointer to the Mosquitto instance.
 * @param userdata Pointer to user data (not used in this case).
 * @param message Pointer to the message received from the MQTT broker.
 **********************************************************************************************************************/
void on_message (struct mosquitto* mosq, void* userdata, const struct mosquitto_message* message)
{
    receivedMessage = std::string(static_cast<char*>(message->payload), message->payloadlen);
}

/*******************************************************************************************************************//**
 * @brief Logs the received GNSS data with enhanced information.
 * 
 * This function logs the GNSS data with a timestamp and log level. It provides more detailed information for debugging
 * and monitoring purposes.
 * 
 * @param gnssData The GNSS data to be logged.
 **********************************************************************************************************************/
void logGNSSData (const std::string& gnssData)
{
    // Get the current time for the log entry
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);

    // Format the log entry with a timestamp and a log level
    std::cout << "[INFO] "
              << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S")  // Timestamp in YYYY-MM-DD HH:MM:SS format
              << " - GNSS Data Received: " 
              << gnssData 
              << std::endl;
}

/*******************************************************************************************************************//**
 * @brief Validates the NMEA format of the GNSS data.
 * 
 * This function checks if the received GNSS data is in a valid NMEA format, specifically the GPRMC sentence.
 * 
 * @param gnssData The GNSS data to be validated.
 * 
 * @return True if the data is valid, false otherwise.
 **********************************************************************************************************************/
bool validateNMEAFormat (const std::string& gnssData)
{
    // Check if it starts with "$GPRMC"
    if (gnssData.rfind("$GPRMC", 0) == 0)
    {
        std::cout << "Valid NMEA data" << std::endl;
        return true;
    }
    else
    {
        std::cout << "Invalid NMEA data" << std::endl;
        return false;
    }
}

/*******************************************************************************************************************//**
 * @brief Stores valid GNSS data in the SQLite database.
 * 
 * This function inserts the valid GNSS data into the `GNSS_DATA` table in the SQLite database.
 * 
 * @param db Pointer to the SQLite database object.
 * @param gnssData The valid GNSS data to be stored.
 **********************************************************************************************************************/
void storeValidData (sqlite3* db, const std::string& gnssData)
{
    char* errMsg = 0;
    std::string sql = "INSERT INTO GNSS_DATA (NMEA_DATA) VALUES ('" + gnssData + "');";

    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else
    {
        std::cout << "Inserted valid GNSS data into the database." << std::endl;
    }
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief Signal handler for graceful shutdown.
 * 
 * This function is triggered by SIGINT or SIGTERM signals to set the `running` flag to false, allowing for a controlled
 * shutdown of the application.
 * 
 * @param signal The signal received (e.g., SIGINT, SIGTERM).
 **********************************************************************************************************************/
static void handle_signal (int signal)
{
    std::cout << "Signal received, shutting down..." << std::endl;
    running = false;
}

/***********************************************************************************************************************
 * Main function
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief Entry point of the GNSS receiver application.
 * 
 * This function sets up signal handling, initializes the SQLite database, connects to the MQTT broker, subscribes to
 * the GNSS data topic, and enters the main loop to process incoming GNSS data.
 * 
 * @param argc Argument count (not used).
 * @param argv Argument vector (not used).
 * 
 * @return Exit status code (0 for success, -1 for failure).
 **********************************************************************************************************************/
int main (int argc, char* argv[])
{
    // Set up signal handlers for SIGINT and SIGTERM
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    mosquitto_lib_init();

    struct mosquitto* mosq = mosquitto_new(NULL, true, NULL);

    if (mosq == NULL)
    {
        std::cerr << "Failed to create Mosquitto to client!" << std::endl;
        return -1;
    }

    // Initialize SQLite database
    sqlite3* db = initDatabase();
    if (db == nullptr)
    {
        return -1; // Exit if the database initialization fails
    }

    // Set up message callback to receive data
    mosquitto_message_callback_set(mosq, on_message);

    // Connect to the MQTT broker
    if (mosquitto_connect(mosq, "localhost", 1883, 60) != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Unable to connect to MQTT broker!" << std::endl;
        return -1;
    }

    // Subscribe to the topic "gnss/data"
    if (mosquitto_subscribe(mosq, NULL, "gnss/data", QOS_LEVEL) != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "Failed to subscribe to topic!" << std::endl;
        return -1;
    }

    // Main loop to receive and process the message
    while (running)
    {
        // Process the MQTT loop
        mosquitto_loop(mosq, -1, 1);

        // If a message is received, process it
        if (!receivedMessage.empty())
        {
            // Log the GNSS data
            logGNSSData(receivedMessage);

            // Validate the NMEA format of the data
            bool isValid = validateNMEAFormat(receivedMessage);

            // If the data is valid, store it in the SQLite database
            if (isValid)
            {
                storeValidData(db, receivedMessage);
            }

            // Clear the message after processing
            receivedMessage.clear();
        }
    }

    // Cleanup
    sqlite3_close(db);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
