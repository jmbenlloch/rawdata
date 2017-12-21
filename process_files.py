import argparse
import os
import json
import tables as tb
import sys
from matplotlib import pylab as plt

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
parser.add_argument('-t', '--type',
                    help='type of waveforms to show',
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


def plot_waveform(waveforms, sensors):
    nevts, nsensors, _ = waveforms.shape
    for evt in range(nevts):
        for s in range(nsensors):
            data = waveforms[evt, s, :]
            ymin = min(data)
            ymax = max(data)
            ymin = ymin - 0.1 * ymin
            ymax = ymax + 0.1 * ymax

            title = "Evt {}, elecid {}".format(evt, sensors[s][0])

            plt.ion()
            plt.plot(data, drawstyle='steps')
            plt.ylim(ymin, ymax)
            plt.title(title)
            plt.show()
            _ = input("Press [enter] to continue.")
            plt.clf()

def plot_file(h5file, options):
    h5in = tb.open_file(h5file)
    if options == 'pmt' or options == 'all':
        if 'pmtrwf' in h5in.root.RD:
            plot_waveform(h5in.root.RD.pmtrwf, h5in.root.Sensors.DataPMT[:])
        else:
            print("No pmts in the file")
    if options == 'blr' or options == 'all':
        if 'pmtblr' in h5in.root.RD:
            plot_waveform(h5in.root.RD.pmtblr, h5in.root.Sensors.DataBLR[:])
        else:
            print("No blrs in the file")
    if options == 'sipm' or options == 'all':
        if 'sipmrwf' in h5in.root.RD:
            plot_waveform(h5in.root.RD.sipmrwf, h5in.root.Sensors.DataSiPM[:])
        else:
            print("No sipms in the file")

plot_file(results.output, results.type)
