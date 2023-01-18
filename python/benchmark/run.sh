#!/bin/bash

## Create and activate a new Python virtualenv, or activate an already existing one
#venv () {
#	ENVDIR="venv"
#	if [ -d env ]; then
#		ENVDIR="env"
#	fi
#
#	if [ ! -d "$ENVDIR/" ]; then
#		if command -v python &> /dev/null
#		then
#			PYTHON="python"
#			PIP="pip"
#		else
#			PYTHON="python3"
#			PIP="pip3"
#		fi
#
#
#		echo "Creating $ENVDIR/ ..."
#		$PYTHON -m venv $ENVDIR
#
#		echo "Upgrading pip..."
#		source $ENVDIR/bin/activate
#		$PIP install --upgrade pip
#	fi
#
#	echo "Activating env..."
#	source $ENVDIR/bin/activate
#}
#
#venv

# install dependencies
#pip install -r requirements.txt

# run
python bench_fmm.py --benchmark_out_format=json --benchmark_out=../../benchmark_plots/benchmark_output/python-output.json