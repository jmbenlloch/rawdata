import tables as tb
import os
import pytest
import json
import numpy as np

from subprocess import check_output
from pytest     import mark


@pytest.fixture(scope = 'session')
def RD_DIR():
    return os.environ['RD_DIR']

@pytest.fixture(scope = 'session')
def tmpdir(tmpdir_factory):
    return tmpdir_factory.mktemp('rawdata')

@mark.slow
@mark.parametrize('filein, fileout, twofiles',
                (('run_3733.gdc1next.rd', 'run_3733.gdc1.h5', False),
                 ('run_3733.gdc2next.rd', 'run_3733.gdc2.h5', False),
                 ('run_3733.gdc1next.rd', 'run_3733.gdcs.h5', True)))
def test_outputfile(tmpdir, RD_DIR, filein, fileout, twofiles):
    testfile = RD_DIR + '/testing/samples/' + fileout
    fileout  = str(tmpdir) + '/' + fileout
    filein   = RD_DIR + '/testing/samples/' + filein

    data = {"file_in" : filein,
            "file_out": fileout,
#            "host": "127.0.0.1",
#            "user": "root",
#            "pass": "root",
#            "dbname": "NEWDB",
            "two_files": twofiles}

    config_file = fileout + '.json'
    with open(config_file, 'w') as outfile:
        json.dump(data, outfile)

    cmd = '{}/decode {}'.format(RD_DIR, config_file)
    output = check_output(cmd, shell=True, executable='/bin/bash')

    h5out  = tb.open_file(fileout)
    h5test = tb.open_file(testfile)

    # Check run info
    runinfo      = h5out .root.Run.runInfo[:][0]
    runinfo_test = h5test.root.Run.runInfo[:][0]
    np.testing.assert_array_equal(runinfo, runinfo_test)

    # Check events info
    events      = h5out .root.Run.events[:]
    events_test = h5test.root.Run.events[:]
    np.testing.assert_array_equal(events, events_test)

    # Check pmts waveforms
    pmtrwf      = h5out .root.RD.pmtrwf[:,:,:]
    pmtrwf_test = h5test.root.RD.pmtrwf[:,:,:]
#    np.testing.assert_array_equal(pmtrwf, pmtrwf_test)

    # Check blrs waveforms
    pmtblr      = h5out .root.RD.pmtblr[:,:,:]
    pmtblr_test = h5test.root.RD.pmtblr[:,:,:]
#    np.testing.assert_array_equal(pmtblr, pmtblr_test)

    # Check sipms waveforms
    #sipmrwf      = h5out .root.RD.sipmrwf[:,:645,:1]
    #sipmrwf_test = h5test.root.RD.sipmrwf[:,:645,:1]
    sipmrwf      = h5out .root.RD.sipmrwf[:,:,:]
    sipmrwf_test = h5test.root.RD.sipmrwf[:,:,:]
    np.testing.assert_array_equal(sipmrwf, sipmrwf_test)

    # Close files
    h5out.close()
    h5test.close()


@mark.slow
def test_split_outputfile(tmpdir, RD_DIR):
    #run without splitting
    fileout    = str(tmpdir) + '/' + '6323.h5'
    filein   = RD_DIR + '/testing/samples/' + 'run_6323.rd'
    data = {"file_in" : filein,
            "file_out": fileout,
            "two_files": False}

    config_file = fileout + '.json'
    with open(config_file, 'w') as outfile:
        json.dump(data, outfile)

    cmd = '{}/decode {}'.format(RD_DIR, config_file)
    output = check_output(cmd, shell=True, executable='/bin/bash')

    #run with splitting
    fout_trg1  = str(tmpdir) + '/' + '6323_trg1.h5'
    fout_trg2  = str(tmpdir) + '/' + '6323_trg2.h5'
    data = {"file_in" : filein,
            "file_out" : fout_trg1,
            "file_out2": fout_trg2,
            "split_trg": True,
            "two_files": False}

    config_file = fileout + '.json'
    with open(config_file, 'w') as outfile:
        json.dump(data, outfile)

    cmd = '{}/decode {}'.format(RD_DIR, config_file)
    output = check_output(cmd, shell=True, executable='/bin/bash')

    #open output files
    h5out       = tb.open_file(fileout)
    h5out_trg1  = tb.open_file(fout_trg1)
    h5out_trg2  = tb.open_file(fout_trg2)

    # Check run info
    runinfo      = h5out     .root.Run.runInfo[:][0]
    runinfo_trg1 = h5out_trg1.root.Run.runInfo[:][0]
    runinfo_trg2 = h5out_trg2.root.Run.runInfo[:][0]
    np.testing.assert_array_equal(runinfo, runinfo_trg1)
    np.testing.assert_array_equal(runinfo, runinfo_trg2)

    # Check events info
    events      = h5out     .root.Run.events[:]
    events_trg1 = h5out_trg1.root.Run.events[:]
    events_trg2 = h5out_trg2.root.Run.events[:]
    np.testing.assert_array_equal(events[0::2], events_trg1)
    np.testing.assert_array_equal(events[1], events_trg2)

    # Check pmt sensors info
    datapmt      = h5out     .root.Sensors.DataPMT[:]
    datapmt_trg1 = h5out_trg1.root.Sensors.DataPMT[:]
    datapmt_trg2 = h5out_trg2.root.Sensors.DataPMT[:]
    np.testing.assert_array_equal(datapmt, datapmt_trg1)
    np.testing.assert_array_equal(datapmt, datapmt_trg2)

    # Check sipm sensors info
    datasipm      = h5out     .root.Sensors.DataSiPM[:]
    datasipm_trg1 = h5out_trg1.root.Sensors.DataSiPM[:]
    datasipm_trg2 = h5out_trg2.root.Sensors.DataSiPM[:]
    np.testing.assert_array_equal(datasipm, datasipm_trg1)
    np.testing.assert_array_equal(datasipm, datasipm_trg2)

    # Check trigger configuration
    trg_conf      = h5out     .root.Trigger.configuration[:]
    trg_conf_trg1 = h5out_trg1.root.Trigger.configuration[:]
    trg_conf_trg2 = h5out_trg2.root.Trigger.configuration[:]
#    np.testing.assert_array_equal(trg_conf, trg_conf_trg1)
#    np.testing.assert_array_equal(trg_conf, trg_conf_trg2)

    # Check trigger events
    trg_evt      = h5out     .root.Trigger.events[:]
    trg_evt_trg1 = h5out_trg1.root.Trigger.events[:]
    trg_evt_trg2 = h5out_trg2.root.Trigger.events[:]
    np.testing.assert_array_equal(trg_evt[0::2], trg_evt_trg1)
    np.testing.assert_array_equal(trg_evt[1], trg_evt_trg2[0])

    # Check trigger type
    trg_type      = h5out     .root.Trigger.trigger[:]
    trg_type_trg1 = h5out_trg1.root.Trigger.trigger[:]
    trg_type_trg2 = h5out_trg2.root.Trigger.trigger[:]
    np.testing.assert_array_equal(trg_type[0::2], trg_type_trg1)
    np.testing.assert_array_equal(trg_type[1], trg_type_trg2[0])

    # Check pmts waveforms
    pmtrwf      = h5out     .root.RD.pmtrwf[:,:,:]
    pmtrwf_trg1 = h5out_trg1.root.RD.pmtrwf[:,:,:]
    pmtrwf_trg2 = h5out_trg2.root.RD.pmtrwf[:,:,:]
    np.testing.assert_array_equal(pmtrwf[0::2,:,:], pmtrwf_trg1)
    np.testing.assert_array_equal(pmtrwf[1,:,:], pmtrwf_trg2[0])

    # Check sipms waveforms
    sipmrwf      = h5out.root.RD.sipmrwf[:,:,:]
    sipmrwf_trg1 = h5out_trg1.root.RD.sipmrwf[:,:,:]
    sipmrwf_trg2 = h5out_trg2.root.RD.sipmrwf[:,:,:]
    np.testing.assert_array_equal(sipmrwf[0::2,:,:], sipmrwf_trg1)
    np.testing.assert_array_equal(sipmrwf[1,:,:], sipmrwf_trg2[0])

    # Close files
    h5out.close()

