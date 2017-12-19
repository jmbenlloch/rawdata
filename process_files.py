import argparse
import os
import json

from subprocess import call

parser = argparse.ArgumentParser()
parser.add_argument('-f', '--file',
                    help='raw data file',
                    required=True,
                    action='store')
parser.add_argument('-o', '--output',
                    help='output hdf5 file',
                    required=True,
                    action='store')
parser.add_argument('-e', '--events',
                    help='number of events to process',
                    required=True,
                    action='store')
results = parser.parse_args()

data = {"file_in" : results.file,
        "file_out": results.output,
        "two_files": False,
        "no_db" : True,
        "max_events": int(results.events)}


filename = results.file.split('/')[-1]
config_file = '/tmp/' + filename + '.json'
with open(config_file, 'w') as outfile:
    json.dump(data, outfile)

cmd = 'export PATH=/home/dateuser/miniconda/bin:$PATH; source /home/dateuser/rawdata/manage.sh env; /home/dateuser/rawdata/decode {0}'.format(config_file)
print(cmd)
call(cmd, shell=True, executable='/bin/bash')
