#include <mqtt/async_client.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <chrono>
#include <thread>
#include <random>
#include <sstream>
#include <iomanip>

// Configuration from environment variables
const std::string MQTT_BROKER = []() {
    const char* broker = std::getenv("MQTT_BROKER");
    return broker ? std::string(broker) : "mqtt-broker";
}();

const int MQTT_PORT = []() {
    const char* port = std::getenv("MQTT_PORT");
    return port ? std::atoi(port) : 1883;
}();

const int PUBLISH_INTERVAL = []() {
    const char* interval = std::getenv("PUBLISH_INTERVAL");
    return interval ? std::atoi(interval) : 2;
}();

const std::string MQTT_ADDRESS = "tcp://" + MQTT_BROKER + ":" + std::to_string(MQTT_PORT);
const std::string CLIENT_ID = "sensor_temperature";
const std::string TOPIC = "factory/sensors/temperature";
const int QOS = 1;

class TemperatureSensor {
private:
    mqtt::async_client client_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;

public:
    TemperatureSensor()
        : client_(MQTT_ADDRESS, CLIENT_ID),
          rng_(std::random_device{}()),
          dist_(20.0, 80.0) {
    }

    bool connect() {
        try {
            std::cout << "[Temperature Sensor] Connecting to MQTT broker at " << MQTT_ADDRESS << "..." << std::endl;
            
            mqtt::connect_options connOpts;
            connOpts.set_keep_alive_interval(20);
            connOpts.set_clean_session(true);
            connOpts.set_automatic_reconnect(true);
            
            auto tok = client_.connect(connOpts);
            tok->wait();
            
            std::cout << "[Temperature Sensor] ✓ Connected successfully!" << std::endl;
            return true;
        } catch (const mqtt::exception& exc) {
            std::cerr << "[Temperature Sensor] ✗ Connection failed: " << exc.what() << std::endl;
            return false;
        }
    }

    void publishLoop() {
        std::cout << "[Temperature Sensor] Starting publishing loop (interval: " 
                  << PUBLISH_INTERVAL << "s)..." << std::endl;
        
        while (true) {
            try {
                double temperature = dist_(rng_);
                std::string payload = createJsonPayload(temperature);
                
                mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, payload);
                pubmsg->set_qos(QOS);
                
                client_.publish(pubmsg)->wait();
                
                std::cout << "[Temperature Sensor] Published: " << payload << std::endl;
                
            } catch (const mqtt::exception& exc) {
                std::cerr << "[Temperature Sensor] ✗ Publish error: " << exc.what() << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(PUBLISH_INTERVAL));
        }
    }

    void disconnect() {
        try {
            client_.disconnect()->wait();
            std::cout << "[Temperature Sensor] Disconnected." << std::endl;
        } catch (const mqtt::exception& exc) {
            std::cerr << "[Temperature Sensor] Disconnect error: " << exc.what() << std::endl;
        }
    }

private:
    std::string createJsonPayload(double value) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "{\"type\":\"temperature\",\"value\":" << value << "}";
        return oss.str();
    }
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Temperature Sensor Microservice" << std::endl;
    std::cout << "  Smart Factory Monitoring System" << std::endl;
    std::cout << "========================================" << std::endl;
    
    TemperatureSensor sensor;
    
    // Retry connection logic
    int retries = 0;
    const int MAX_RETRIES = 10;
    
    while (retries < MAX_RETRIES) {
        if (sensor.connect()) {
            break;
        }
        retries++;
        std::cout << "[Temperature Sensor] Retry " << retries << "/" << MAX_RETRIES 
                  << " in 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    if (retries == MAX_RETRIES) {
        std::cerr << "[Temperature Sensor] Failed to connect after " << MAX_RETRIES 
                  << " attempts. Exiting." << std::endl;
        return 1;
    }
    
    try {
        sensor.publishLoop();
    } catch (const std::exception& e) {
        std::cerr << "[Temperature Sensor] Fatal error: " << e.what() << std::endl;
        sensor.disconnect();
        return 1;
    }
    
    sensor.disconnect();
    return 0;
}
