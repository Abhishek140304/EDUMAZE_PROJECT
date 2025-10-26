# Use a base image with a C++ compiler and build tools
FROM gcc:latest AS builder

# Install dependencies for building
RUN apt-get update && apt-get install -y cmake

# Copy your application source code, including the submodules in 'vendor'
WORKDIR /app
COPY . .

# Build your application
# The CMAKE_PREFIX_PATH tells cmake where to find the headers
RUN cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/app/vendor/asio/asio/include;/app/vendor/Crow/include" && \
    cmake --build .

# Final stage: create a smaller runtime image
FROM debian:stable-slim
WORKDIR /app

# Copy only the compiled application and necessary assets
COPY --from=builder /app/Edumaze .


# Expose the port your app runs on
EXPOSE 18080

# Command to run the executable
CMD ["./Edumaze"]