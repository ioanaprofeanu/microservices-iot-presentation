# Multi-stage build for C++ Temperature Sensor Microservice
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    libssl-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Build and install Paho MQTT C library
WORKDIR /tmp
RUN git clone https://github.com/eclipse/paho.mqtt.c.git && \
    cd paho.mqtt.c && \
    git checkout v1.3.13 && \
    cmake -Bbuild -H. -DPAHO_WITH_SSL=ON -DPAHO_ENABLE_TESTING=OFF && \
    cmake --build build --target install && \
    ldconfig

# Build and install Paho MQTT C++ library
RUN git clone https://github.com/eclipse/paho.mqtt.cpp.git && \
    cd paho.mqtt.cpp && \
    git checkout v1.3.2 && \
    cmake -Bbuild -H. -DPAHO_BUILD_DOCUMENTATION=OFF -DPAHO_BUILD_SAMPLES=OFF && \
    cmake --build build --target install && \
    ldconfig

# Copy application source
WORKDIR /app
COPY CMakeLists.txt .
COPY main.cpp .

# Build the application
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build

# Runtime stage - minimal image
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Copy compiled libraries from builder
COPY --from=builder /usr/local/lib/libpaho* /usr/local/lib/
COPY --from=builder /usr/local/include/mqtt /usr/local/include/mqtt

# Copy the compiled binary
COPY --from=builder /app/build/sensor-temp /usr/local/bin/sensor-temp

# Update library cache
RUN ldconfig

# Run the sensor
CMD ["sensor-temp"]
