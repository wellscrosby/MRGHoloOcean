FROM nvidia/cudagl:9.2-runtime-ubuntu18.04

# OpenCV's runtime dependencies (and other dependencies)
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 python3-dev ipython3 module-init-tools curl build-essential python3-pip git \
    libglib2.0-dev cmake
RUN apt-get install -y libglib2.0-0 libsm6 libxrender-dev libxext6
RUN adduser --disabled-password --gecos "" holodeckuser

#install LCM
RUN git clone https://github.com/lcm-proj/lcm.git
RUN cd lcm && mkdir build && cd build && cmake .. && make install
# For some reason, installs to an incorrect location, add it to our path
ENV PYTHONPATH="${PYTHONPATH}:/usr/local/lib/python3/dist-packages"

# Install all python dependencies
RUN pip3 install -U pip setuptools wheel
RUN pip3 install numpy posix_ipc holodeck pytest opencv-python scipy
# Install worlds so we don't have to redownload unchanging worlds each time
USER holodeckuser
RUN python3 -c 'import holodeck; holodeck.install("DefaultWorlds")'
# Remove default holodeck
USER root
RUN pip3 uninstall -y holodeck
USER holodeckuser

WORKDIR /home/holodeckuser/

CMD /bin/bash
