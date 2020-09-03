FROM python
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Denver

RUN apt-get -y update
RUN apt-get -y upgrade
RUN apt-get -y install libgl1-mesa-glx

RUN useradd -m myuser
USER myuser

RUN pip install pytest opencv-python holodeck
RUN python3 -c 'import holodeck; holodeck.install("DefaultWorlds")'
RUN pip uninstall -y holodeck

CMD /bin/bash
