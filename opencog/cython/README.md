Cogserver Python Interface
--------------------------

## Requirements ##

* Python3
* Cython 0.14 or later. http://www.cython.org/
* Nosetests3 - for running unit tests.

Both Cython and Nosetests can be installed with easy_install:
```
 sudo easy_install cython nose
```
The bindings are written mostly using Cython, which allows writing
code that looks pythonic but gets compiled to C.  It also makes it
trivial to access both Python objects and C objects without using a
bunch of extra Python library API calls.

Currently the package structure looks like this:
```
 opencog.atomspace
 opencog.atomspace.types
 opencog.cogserver
```

Start the cogserver by saying:
```
 import opencog.cogserver
 something...something (???)
 start_cogserver() (???)
```
Something is wrong with the above documentation. Can an experienced
python user help figure out the stuff above?

## Tutorial ##

The OpenCog wiki contains the Python tutorial:

http://wiki.opencog.org/w/Python
