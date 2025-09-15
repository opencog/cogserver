
HOWTO run these tests by hand:
------------------------------

You may need to set up the PYTHON path:
```
export PYTHONPATH=${PROJECT_BINARY_DIR}/opencog/cython
```

For example:
```
export PYTHONPATH=build/opencog/cython
```

You maye also need to specify the library path:
```
export LD_LIBRARY_PATH=build/opencog/cython
```

Then, from the project root directory:
```
nosetests3 -vs tests/cython/
```
