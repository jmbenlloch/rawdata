import tables as tb
import sys
from matplotlib import pylab as plt


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

def plot_file(h5file):
    h5in = tb.open_file(h5file)
    if 'pmtrwf' in h5in.root.RD:
        plot_waveform(h5in.root.RD.pmtrwf, h5in.root.Sensors.DataPMT[:])
    if 'pmtblr' in h5in.root.RD:
        plot_waveform(h5in.root.RD.pmtblr, h5in.root.Sensors.DataBLR[:])
    if 'sipmrwf' in h5in.root.RD:
        plot_waveform(h5in.root.RD.sipmrwf, h5in.root.Sensors.DataSiPM[:])

if __name__ == '__main__':
    h5file = sys.argv[1]
    plot_file(h5file)
