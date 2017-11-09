#!/usrbin/env bash

COMMAND=$1
ARGUMENT=$2


function install_and_check {
    install
    run_tests
}

function install {
	install_conda
    make_environment
    set_env
    compile
}

function install_conda {
    # Identifies your operating system and installs the appropriate
    # version of conda. We only consider Linux and OS X at present.

    # Does nothing if conda is already found in your PATH.

    case "$(uname -s)" in

        Darwin)
            export CONDA_OS=MacOSX
            ;;

        Linux)
            export CONDA_OS=Linux
            ;;

        *)
            echo "Installation only supported on Linux and OS X"
            exit 1
            ;;
    esac

    if which conda ; then
        echo Conda already installed. Skipping conda installation.
    else
        echo Installing conda for $CONDA_OS
        if which wget; then
            wget https://repo.continuum.io/miniconda/Miniconda3-4.3.21-${CONDA_OS}-x86_64.sh -O miniconda.sh
        else
            curl https://repo.continuum.io/miniconda/Miniconda3-4.3.21-${CONDA_OS}-x86_64.sh -o miniconda.sh
        fi
        bash miniconda.sh -b -p $HOME/miniconda
        export PATH="$HOME/miniconda/bin:$PATH"
        echo Prepended $HOME/miniconda/bin to your PATH. Please add it to your shell profile for future sessions.
        conda config --set always_yes yes --set changeps1 no
        echo Prepended $HOME/miniconda/bin to your PATH. Please add it to your shell profile for future sessions.
    fi
}

function make_environment {
    YML_FILENAME=environment.yml
	conda env create -f ${YML_FILENAME}
}

function set_env {
	export PATH="$HOME/miniconda/bin:$PATH"
    source activate rawdata
	export RD_DIR=`pwd`/
	export LD_LIBRARY_PATH=$CONDA_PREFIX/lib:$LD_LIBRARY_PATH
}

function run_tests {
    set_env
	./tests && pytest -v testing
}

function compile {
    set_env
    make all
    make tests
}

function compile_and_test {
    set_env
    compile
	run_tests
}

function clean {
	make clean
}

THIS=manage.sh

## Main command dispatcher

case $COMMAND in
    install_and_check)      install_and_check ;;
    install)                install ;;
    make_environment)       make_environment ;;
    run_tests)              run_tests ;;
    compile_and_test)       compile_and_test ;;
    env)                    set_env ;;
    clean)                  clean ;;

    *) echo Unrecognized command: ${COMMAND}
       echo
       echo Usage:
       echo
       echo "source $THIS install_and_check"
       echo "source $THIS install"
       echo "source $THIS run_tests"
       echo "bash   $THIS compile_and_test"
       echo "source $THIS env"
       echo "bash   $THIS clean"
       ;;
esac
