FROM adamrehn/ue4-runtime:20.04-cudagl11.2.0-noaudio

USER root
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl git libglib2.0-dev software-properties-common

# OpenCV's runtime dependencies (and other dependencies)
RUN apt-get install -y libglib2.0-0 libsm6 libxrender-dev libxext6

# Install all python versions to test on
RUN add-apt-repository ppa:deadsnakes/ppa
RUN apt-get update && apt-get install -y python3-dev python3-pip \
    python3.6-dev python3.7-dev python3.8-dev python3.9-dev
RUN pip3 install setuptools wheel tox

# Setup user
RUN adduser --disabled-password --gecos "" holodeckuser
USER holodeckuser

CMD /bin/bash
