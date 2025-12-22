# Stage 1: Dependencies
FROM gcc:14.2.0 AS deps

USER root
WORKDIR /root

# Install tools
RUN apt update; \
    apt install -y python3-pip cmake gdb
RUN pip install conan --break-system-packages

# Copy only files needed for dependency installation
COPY ./conanfile.txt /root/conanfile.txt
COPY ./.devcontainer/conan-profile-release /root/conan-profile-release

# Install dependencies
RUN conan install . --output-folder=build --build=missing \
    -pr:h conan-profile-release \
    -pr:b conan-profile-release

# https://github.com/grpc-ecosystem/grpc-health-probe
# The grpc_health_probe utility allows you to query health of gRPC services
# that expose service their status through the gRPC Health Checking Protocol.
RUN GRPC_HEALTH_PROBE_VERSION=v0.4.13 && \
    wget -qO/bin/grpc_health_probe https://github.com/grpc-ecosystem/grpc-health-probe/releases/download/${GRPC_HEALTH_PROBE_VERSION}/grpc_health_probe-linux-amd64 && \
    chmod +x /bin/grpc_health_probe

# Stage 2: Build
FROM deps AS build

# Copy the rest of the project files
COPY ./common.cmake /root/common.cmake
COPY ./CMakeLists.txt /root/CMakeLists.txt
COPY ./proto /root/proto
COPY ./src /root/src

# Build the project
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
RUN cmake --build build --config Release

# Stage 3: Runtime
FROM debian:12-slim AS runtime_service

WORKDIR /app

# Copy binaries + config
COPY --from=build /root/build/geo /app/geo
COPY --from=build /usr/local/lib64/libstdc++.so.6 /lib/x86_64-linux-gnu/libstdc++.so.6
COPY --from=build /bin/grpc_health_probe /bin/grpc_health_probe
COPY ./geo-config.json /app/geo-config.json

# Expose the application port
EXPOSE 50051

# Run the application
CMD ["./geo", "--config", "geo-config.json"]

# Stage 4: Runtime for test client
FROM python:3.11-slim AS runtime_tests

WORKDIR /

# Copy test client and proto definitions
COPY ./tests /tests
COPY ./proto /proto

# Install Python dependencies for the test client
RUN pip install -r /tests/requirements.txt

# Default server address
ENV SERVER_ADDRESS=0.0.0.0:50051

# Run the test client
CMD ["python3", "/tests/main.py"]
