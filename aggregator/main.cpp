#include <mqtt/async_client.h>
#include <pqxx/pqxx>
#include <iostream>
#include <cstdlib>
#include <string>
#include <chrono>
#include <thread>
#include <sstream>
#include <regex>

// Configuration from environment variables
const std::string MQTT_BROKER = []() {
    const char* broker = std::getenv("MQTT_BROKER");
    return broker ? std::string(broker) : "mqtt-broker";
}();

const int MQTT_PORT = []() {
    const char* port = std::getenv("MQTT_PORT");
    return port ? std::atoi(port) : 1883;
}();

const std::string DB_HOST = []() {
    const char* host = std::getenv("DB_HOST");
    return host ? std::string(host) : "database";
}();

const int DB_PORT = []() {
    const char* port = std::getenv("DB_PORT");
    return port ? std::atoi(port) : 5432;
}();

const std::string DB_NAME = []() {
    const char* name = std::getenv("DB_NAME");
    return name ? std::string(name) : "factory_monitoring";
}();

const std::string DB_USER = []() {
    const char* user = std::getenv("DB_USER");
    return user ? std::string(user) : "factory_user";
}();

const std::string DB_PASSWORD = []() {
    const char* pass = std::getenv("DB_PASSWORD");
    return pass ? std::string(pass) : "factory_pass";
}();

const std::string MQTT_ADDRESS = "tcp://" + MQTT_BROKER + ":" + std::to_string(MQTT_PORT);
const std::string CLIENT_ID = "aggregator_service";
const std::string TOPIC_SUBSCRIBE = "factory/sensors/#";
const int QOS = 1;

class AggregatorService : public virtual mqtt::callback {
private:
    mqtt::async_client mqttClient_;
    std::string dbConnectionString_;
    
public:
    AggregatorService()
        : mqttClient_(MQTT_ADDRESS, CLIENT_ID) {
        
        // Build PostgreSQL connection string
        std::ostringstream oss;
        oss << "host=" << DB_HOST 
            << " port=" << DB_PORT
            << " dbname=" << DB_NAME
            << " user=" << DB_USER
            << " password=" << DB_PASSWORD;
        dbConnectionString_ = oss.str();
        
        mqttClient_.set_callback(*this);
    }

    bool connectMQTT() {
        try {
            std::cout << "[Aggregator] Connecting to MQTT broker at " << MQTT_ADDRESS << "..." << std::endl;
            
            mqtt::connect_options connOpts;
            connOpts.set_keep_alive_interval(20);
            connOpts.set_clean_session(true);
            connOpts.set_automatic_reconnect(true);
            
            auto tok = mqttClient_.connect(connOpts);
            tok->wait();
            
            std::cout << "[Aggregator] ✓ Connected to MQTT broker!" << std::endl;
            
            // Subscribe to sensor topics
            mqttClient_.subscribe(TOPIC_SUBSCRIBE, QOS)->wait();
            std::cout << "[Aggregator] ✓ Subscribed to topic: " << TOPIC_SUBSCRIBE << std::endl;
            
            return true;
        } catch (const mqtt::exception& exc) {
            std::cerr << "[Aggregator] ✗ MQTT connection failed: " << exc.what() << std::endl;
            return false;
        }
    }

    bool testDatabaseConnection() {
        try {
            std::cout << "[Aggregator] Testing database connection..." << std::endl;
            pqxx::connection conn(dbConnectionString_);
            
            if (conn.is_open()) {
                std::cout << "[Aggregator] ✓ Database connected: " << conn.dbname() << std::endl;
                return true;
            }
            return false;
        } catch (const std::exception& e) {
            std::cerr << "[Aggregator] ✗ Database connection failed: " << e.what() << std::endl;
            return false;
        }
    }

    void run() {
        std::cout << "[Aggregator] Service is running. Waiting for messages..." << std::endl;
        
        // Keep the service running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    void disconnect() {
        try {
            mqttClient_.disconnect()->wait();
            std::cout << "[Aggregator] Disconnected from MQTT." << std::endl;
        } catch (const mqtt::exception& exc) {
            std::cerr << "[Aggregator] Disconnect error: " << exc.what() << std::endl;
        }
    }

private:
    // MQTT callback - called when message arrives
    void message_arrived(mqtt::const_message_ptr msg) override {
        try {
            std::string topic = msg->get_topic();
            std::string payload = msg->to_string();
            
            // Parse JSON manually (simple parser for this use case)
            std::string sensorType = extractJsonValue(payload, "type");
            std::string valueStr = extractJsonValue(payload, "value");
            
            if (sensorType.empty() || valueStr.empty()) {
                std::cerr << "[Aggregator] ✗ Invalid JSON format: " << payload << std::endl;
                return;
            }
            
            double value = std::stod(valueStr);
            
            // Save to database
            saveToDatabase(sensorType, value);
            
            std::cout << "[Aggregator] Received from topic \"" << topic 
                      << "\": {type: " << sensorType << ", value: " << value 
                      << "} -> Saved to DB ✓" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[Aggregator] ✗ Error processing message: " << e.what() << std::endl;
        }
    }

    void connection_lost(const std::string& cause) override {
        std::cerr << "[Aggregator] ✗ Connection lost: " << cause << std::endl;
    }

    void saveToDatabase(const std::string& sensorType, double value) {
        try {
            pqxx::connection conn(dbConnectionString_);
            pqxx::work txn(conn);
            
            std::string query = "INSERT INTO sensor_readings (sensor_type, value) VALUES (" +
                               txn.quote(sensorType) + ", " +
                               txn.quote(value) + ")";
            
            txn.exec(query);
            txn.commit();
            
        } catch (const std::exception& e) {
            std::cerr << "[Aggregator] ✗ Database error: " << e.what() << std::endl;
            throw;
        }
    }

    // Simple JSON value extractor (for demo purposes)
    std::string extractJsonValue(const std::string& json, const std::string& key) {
        std::regex regex("\"" + key + "\"\\s*:\\s*\"?([^,}\"]+)\"?");
        std::smatch match;
        
        if (std::regex_search(json, match, regex)) {
            return match[1].str();
        }
        return "";
    }
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Aggregator Service" << std::endl;
    std::cout << "  Smart Factory Monitoring System" << std::endl;
    std::cout << "========================================" << std::endl;
    
    AggregatorService aggregator;
    
    // Test database connection with retries
    int dbRetries = 0;
    const int MAX_DB_RETRIES = 10;
    
    while (dbRetries < MAX_DB_RETRIES) {
        if (aggregator.testDatabaseConnection()) {
            break;
        }
        dbRetries++;
        std::cout << "[Aggregator] Database retry " << dbRetries << "/" << MAX_DB_RETRIES 
                  << " in 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    if (dbRetries == MAX_DB_RETRIES) {
        std::cerr << "[Aggregator] Failed to connect to database. Exiting." << std::endl;
        return 1;
    }
    
    // Connect to MQTT with retries
    int mqttRetries = 0;
    const int MAX_MQTT_RETRIES = 10;
    
    while (mqttRetries < MAX_MQTT_RETRIES) {
        if (aggregator.connectMQTT()) {
            break;
        }
        mqttRetries++;
        std::cout << "[Aggregator] MQTT retry " << mqttRetries << "/" << MAX_MQTT_RETRIES 
                  << " in 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    if (mqttRetries == MAX_MQTT_RETRIES) {
        std::cerr << "[Aggregator] Failed to connect to MQTT broker. Exiting." << std::endl;
        return 1;
    }
    
    try {
        aggregator.run();
    } catch (const std::exception& e) {
        std::cerr << "[Aggregator] Fatal error: " << e.what() << std::endl;
        aggregator.disconnect();
        return 1;
    }
    
    aggregator.disconnect();
    return 0;
}
