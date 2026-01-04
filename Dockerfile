FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y nodejs && \
    apt-get install -y npm

RUN apt-get install -y build-essential && \
    apt-get install -y curl && \
    apt-get install -y git gh && \
    apt-get install -y pkg-config make cmake ninja-build && \
    apt-get install -y libasound2-dev libpulse-dev libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libthai-dev

# SDL3_ttf dependencyap
RUN apt-get install -y libharfbuzz-dev

RUN apt-get install -y libopencv-dev

RUN npm i -g @openai/codex