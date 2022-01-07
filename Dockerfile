FROM adamrehn/ue4-runtime:20.04-cudagl11.2.0-noaudio

USER root
ENV DEBIAN_FRONTEND=noninteractive
# Install repo with python 3.6/3.7
RUN apt-get update && apt-get install -y --no-install-recommends software-properties-common
RUN add-apt-repository ppa:deadsnakes/ppa

RUN apt-get update && apt-get install -y --no-install-recommends \
    curl git libglib2.0-dev  \
    # OpenCV's runtime dependencies (and other dependencies)
    libglib2.0-0 libsm6 libxrender-dev libxext6 \
    # all python versions
    python3-dev python3-pip \
    python3.6-dev python3.7-dev python3.8-dev python3.9-dev python3.10-dev

RUN pip3 install setuptools wheel tox

# Setup user
USER ue4

CMD /bin/bash
