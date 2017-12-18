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
