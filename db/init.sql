-- Smart Factory Monitoring System - Database Schema

CREATE TABLE IF NOT EXISTS sensor_readings (
    id SERIAL PRIMARY KEY,
    sensor_type TEXT NOT NULL,
    value DOUBLE PRECISION NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create index for faster queries
CREATE INDEX idx_sensor_readings_timestamp ON sensor_readings(timestamp DESC);
CREATE INDEX idx_sensor_readings_type ON sensor_readings(sensor_type);

-- Insert sample comment for verification
INSERT INTO sensor_readings (sensor_type, value) VALUES ('system', 0.0);

COMMENT ON TABLE sensor_readings IS 'Stores all sensor readings from the factory floor';
COMMENT ON COLUMN sensor_readings.sensor_type IS 'Type of sensor: temperature, vibration, etc.';
COMMENT ON COLUMN sensor_readings.value IS 'Sensor reading value';
COMMENT ON COLUMN sensor_readings.timestamp IS 'Time when reading was recorded';
