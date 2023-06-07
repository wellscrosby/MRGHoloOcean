FROM adamrehn/ue4-runtime:20.04-cudagl11.2.0-noaudio

USER root
ENV DEBIAN_FRONTEND=noninteractive
RUN curl -o cuda-keyring_1.0-1_all.deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-keyring_1.0-1_all.deb && dpkg -i cuda-keyring_1.0-1_all.deb
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl git libglib2.0-dev software-properties-common

# OpenCV's runtime dependencies (and other dependencies)
RUN apt-get install -y libglib2.0-0 libsm6 libxrender-dev libxext6

# Install all python versions to test on
RUN add-apt-repository ppa:deadsnakes/ppa
RUN apt-get update && apt-get install -y python3-dev python3-pip \
    python3.6-dev python3.6-distutils python3.7-dev python3.7-distutils \
    python3.8-dev python3.9-dev python3.10-dev python3.10-distutils
RUN pip3 install setuptools wheel tox posix_ipc numpy

# Setup user
USER ue4

CMD /bin/bash
