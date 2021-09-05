FROM nvidia/cudagl:9.2-runtime-ubuntu18.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    module-init-tools curl build-essential git libglib2.0-dev cmake

# OpenCV's runtime dependencies (and other dependencies)
RUN apt-get install -y libglib2.0-0 libsm6 libxrender-dev libxext6

# Install all python versions to test on
RUN apt-get install -y python3-dev python3-pip \
    python3.5 python3.6-dev python3.7-dev python3.8-dev
RUN pip3 install setuptools wheel tox

CMD /bin/bash
