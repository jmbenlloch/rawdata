FROM ubuntu:bionic

RUN apt-get update -y && \
    apt-get install -y make g++ libhdf5-dev libhdf5-100 libjsoncpp1 libjsoncpp-dev  libmysqlclient-dev wget && \
	ln -s /usr/lib/x86_64-linux-gnu/libhdf5_hl_cpp.so /usr/lib/x86_64-linux-gnu/libhdf5_hl.so && \
	ln -s /usr/lib/x86_64-linux-gnu/libhdf5_serial.so /usr/lib/x86_64-linux-gnu/libhdf5.so  && \
    apt-get install -y git && \
    apt-get autoremove -y && \
    apt-get clean -y && \
    rm -rf /var/cache/apt/archives/* && \
    rm -rf /var/lib/apt/lists/*

ENV JSONINC /usr/include/jsoncpp/json
ENV MYSQLINC /usr/include/mysql
ENV HDF5INC /usr/include/hdf5/serial
ENV RD_DIR /rawdata
ENV PATH /miniconda/bin:${PATH}

COPY . /rawdata
WORKDIR /rawdata

# Install pytest from miniconda
RUN wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O miniconda.sh && \
	/bin/bash miniconda.sh -b -p /miniconda && \
	conda env create -f /rawdata/environment.yml

RUN make all; make tests
#RUN /rawdata/tests

#SHELL ["conda", "run", "-n", "rawdata", "/bin/bash", "-c"]
#RUN pytest -v /rawdata/testing
