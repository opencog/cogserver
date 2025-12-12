
Python Shell Tests
------------------

This directory contains tests for the CogServer's Python shell.

### Tests

* `PyEvalUTest.cxxtest` - C++ unit test that starts a CogServer and tests
  the Python shell via network connections (using `nc`).

* `test_cogserver.py` - Python test that exercises the CogServer's Python
  bindings.

### Running tests

From the build directory, run all tests:
```
ctest -R Cython --output-on-failure
ctest -R CogServerPython --output-on-failure
```

Or run individual tests directly:
```
./tests/cython/PyEvalUTest
python3 ../tests/cython/test_cogserver.py
```

### Environment setup

If running tests manually, you may need to set:
```
export PYTHONPATH=build/opencog/cython
export LD_LIBRARY_PATH=build/opencog/cogserver/server:build/opencog/cython/opencog
```
