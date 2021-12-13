# CSV single-include
This directory contains ``csv.hpp``, which contains the whole
source code as a single-include, header-only library

## Building
The header files are combined to a single file using [quom](https://github.com/Viatorus/quom).
1. Install quom: ``pip install quom``
2. Combine the files: ``python -m quom include/csv.hpp single_include/csv.hpp``
