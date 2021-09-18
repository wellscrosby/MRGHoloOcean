FROM nvidia/cudagl:11.4.1-runtime-ubuntu20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl git libglib2.0-dev software-properties-common build-essential \
    python3-dev python3-pip ipython3

# OpenCV's runtime dependencies
RUN apt-get install -y libglib2.0-0 libsm6 libxrender-dev libxext6

RUN pip3 install setuptools wheel

RUN adduser --disabled-password --gecos "" holooceanuser
USER holooceanuser

WORKDIR /home/holooceanuser/source/holoocean/

# This should be COPY ../ but docker doesn't allow copying files outside the context
# To copy the project files either run the build command in this directory with the
# previous directory as the context: docker build -t frostlab/holoocean[:tag] -f ./Dockerfile ..
# or run it from the parent directory and provide the docker file location
# docker build -t frostlab/holoocean[:tag] -f ./docker/Dockerfile .
COPY ./ .
RUN pip3 install .

CMD ["/bin/bash"]