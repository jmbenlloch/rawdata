#!/bin/bash

source /miniconda/etc/profile.d/conda.sh
conda activate rawdata
/rawdata/tests && pytest -v /rawdata/testing
