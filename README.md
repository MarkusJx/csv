# csv
A C++ header-only [RFC 4180](https://tools.ietf.org/html/rfc4180) compliant CSV (Comma-Separated Values) parser/writer.

## Examples
### Instantiation
#### Basic creation
```c++
markusjx::csv csv;
```

#### Create using string literals
It is also possible to create a csv object using a string literal.
To create a new row, insert a line break.
```c++
markusjx::csv csv = "abc;def;123\nghi"__csv;
```

#### Create using a std::initializer_list
You can also initialize a csv object using a ``std::initializer_list``:
```c++
markusjx::csv csv = {
    {"abc", "def", 123, true}, // This will be the first row
    {"ghi", 456, false, "kl"}  // THis will be the second row
};
```

#### Create using a std::vector
You could also use a ``std::vector<markusjx::csv_row>`` to initialize a csv object:
```c++
std::vector<markusjx::csv_row> data;
data.emplace_back({"abc", "def", 123});
data.push_back(markusjx::csv_row({"ghi", 456, false, "kl"}));

markusjx::csv csv = data;
```

### Writing to the csv object
You can write to the last line of the csv object
using the ``<<`` operator:
```c++
csv << "some" << "text";
```

End a line to start writing on a new one like this:
```c++
csv << markusjx::csv::endl;
// or
csv << std::endl;
// or
csv.endline();
```

#### Appending a row
You can append data to the current row using a ``std::initializer_list``:
```c++
csv << {"abc", 123};
```
If you want to write to a new row, end the last line first.

#### Writing to a row anywhere
You can also write to any row, using the ``[]`` operator.
If the requested row does not exist, it will be created.
```c++
// Write to the 10th row, which may or may not exist
csv[10] << "some" << "data";

// Set the data of the 11th row, which may or may not exist
csv[11] = {"some", "data", 123};
```

Please note that line breaks cannot be inserted into rows.
Line breaks may only be written to cells.

**Any write operations using ``operator<<`` are also possible using the ``push()`` function.**

#### Writing to a row
It is also possible to write data to a specific cell.
If the cell (and row) do(es) not exist, it will be created.
```c++
// Set some data
csv[5][4] = 42;

// Add data
csv[7][8] += 5;

// ...to a string
csv[8][9] += "string";
```

### Accessing the data
You can handle the csv object as an two-dimensional array,
therefore the data can be accesses using the ``[]`` operator:
```c++
// Get all cells in the first row:
const auto row_0 = csv[0];

// Get the first cell in the first row:
const auto cell_0_0 = csv[0][0];
// or
const auto cell_0_0 = row_0[0];
```

Another way is to use the ``markusjx::csv::at(size_t)``
function to access a row or cell, just like accessing
data in a ``std::vector``:
```c++
const auto row_0 = csv.at(0);
const auto cell_0_0 = csv.at(0).at(0);
```

#### Getting the actual data
In order to access the stored data, you can use the provided ``operator U()`` functions:
```
// The stored data is an integer:
int i = csv[0][0];

// It is a double
double d = csv[0][0];

// Or maybe a std::string
std::string s = csv[0][0];

// Or a bool
bool b = csv[0][0];
```

Please note that conversions to number values may throw a
``markusjx::exceptions::conversion_error`` if the value can't
be converted to a number. If the value is a floating point integer
and you are converting to an integer, the conversion is done as usual,
no exception will be thrown and the integer is returned.
Additionally, conversions to the types ``char`` and ``bool`` may
throw an exception if the stored value can't be converted.

To view supported data types, take a look at the [supported data types](#supported-data-types).

You may also get a character in the string value of a cell:
```c++
// Get the length of the string value:
size_t len = csv[0][0].length();

// Get the first character in the value
// of the first cell in the first row:
char c = csv[0][0][0];
```

### Working with data
#### Increasing, decreasing and writing integer values
Increase an integer value:
```c++
// Increase by any value
csv[0][0] += 42;
// or
csv[0][0] = csv[0][0] + csv[0][1];

// Increase by one
csv[0][0]++;
// Pre-increment
++csv[0][0];

// Plain addition
int result = csv[0][0] + csv[0][1];
// or using integers
int alt = csv[0][0] + 25;
```

Decrease an integer value:
```c++
// Decrease by any value
csv[0][0] -= 42;
// or
csv[0][0] = csv[0][0] - csv[0][1];

// Decrease by one
csv[0][0]--;
// Pre-decrement
--csv[0][0];

// Plain subtraction
int result = csv[0][0] - csv[0][1];
// or using integers
int alt = csv[0][0] - 25;
```

Multiplying values:
```c++
// Multiply and assign any value
csv[0][0] *= 42;
// or
csv[0][0] = csv[0][0] * csv[0][1];

// Plain multiplication
int result = csv[0][0] * csv[0][1];
// or using integers
int alt = csv[0][0] * 25;
```

Division:
```c++
// Dividy by and assign any value
csv[0][0] /= 42;
// or
csv[0][0] = csv[0][0] / csv[0][1];

// Plain division
int result = csv[0][0] / csv[0][1];
// or using integers
int alt = csv[0][0] / 25;
```

##### Type conversion rules:
If any of the provided values is a floating point integer,
all values involved in a operation will be converted to ``long double``.
If rows containing integers are divided by each other and no
floating point integers are involved with the operation,
a plain integer value is returned. If default data types,
such as ``int``, ``long`` or ``double`` are involved in a
mathematical operation, the compiler's default conversion rules apply.

#### String operations
All string operators are also available with string values, such as:
```c++
// Append to a string and assign the value:
csv[0][0] += "some string";

// Append to a string:
std::string data = csv[0][0] + "some data";

// Append a value from another cell
csv[0][0] *= csv[0][1];
// or
csv[0][0] = csv[0][0] + csv[0][1];
```

**Since all values are stored as strings, integer
values will be converted to string values when working with strings.**
Example:
```c++
// csv[0][0] is an integer
csv[0][0] = 42;

// Append a string
csv[0][0] += "abc";
// The result will be a string
```

#### Iterating over all rows
To iterate over all rows you can either use indices:
```c++
for (size_t i = 0; i < csv.size(); i++) {
    // Get the row at index i
    const auto row = csv[i];
}
```

or you could use iterators:
```c++
for (const auto &row : csv) {
    // Do something with that info
}
```

#### Iterating over all cells
Again, you can iterate over all cells either using indices:
```c++
for (size_t i = 0; i < csv.size(); i++) {
    for (size_t j = 0; j < csv[i].size(); j++) {
        // Get the cell at index j
        const auto cell = csv[i][j];
    }
}
```

or using iterators:
```c++
for (const auto &row : csv) {
    for (const auto &cell : row) {
        // Do something with the cell
    }
}
```

#### Deleting rows
Delete rows using ``markusjx::basic_csv::remove(size_t)``:
```c++
// Delete the first row
csv.remove(0);
```

#### Deleting cells
Delete cells from a row using ``markusjx::csv_row::remove(size_t)``:
```c++
// Delete the first cell in the first row
csv[0].remove(0);
```

#### Clearing the csv object
To remove all values from a csv object use ``markusjx::basic_csv::clear()``:
```c++
csv.clear();
// csv is empty now
```

#### Clearing a csv row
To remove all cells from a csv row use ``markusjx::csv_row::clear()``:
```c++
// Clear the first row in the csv object
csv[0].clear();
// csv[0] is empty now
```

#### Remove all empty cells/rows
Remove all empty cells at the end of a row:
```c++
auto &row = csv[0];
row.strip();
```

Remove all empty cells at the end of all rows and remove
all empty rows at the end of the csv object:
```c++
csv.strip();
```

### Comparisons
You can compare cells with any other cells or raw values.
#### Number comparisons
```c++
// Greater than:
bool g = csv[0][0] > csv[0][1];
// or using an integer
bool g_i = csv[0][0] > 25;

// Greater than or equal to:
bool ge = csv[0][0] >= csv[0][1];
// or using an integer
bool ge_i = csv[0][0] >= 25;

// Less than:
bool l = csv[0][0] < csv[0][1];
// or using an integer
bool l_i = csv[0][0] < 25;

// Less than or equal to:
bool le = csv[0][0] <= csv[0][1];
// or using an integer
bool le_i = csv[0][0] <= 25;

// Equal to:
bool e = csv[0][0]== csv[0][1];
// or using an integer
bool e_i = csv[0][0] == 25;

// Not equal to:
bool ne = csv[0][0] != csv[0][1];
// or using an integer
bool ne_i = csv[0][0] != 25;
```

When comparing cell values, the same rules
as with mathematical operations apply to integers.

**If two values are compared and one of them is not a number,
both values will be compared as strings.**

### Type checks
You may also get the type of a cell value:
```c++
// Check if the value is a boolean
bool is_bool = csv[0][0].isBoolean();

// Check if the value is a number
// (any number floating point, integer...):
bool is_num = csv[0][0].isNumber();

// Check if the value is a floating point integer:
bool is_float = csv[0][0].isFloatingPoint();

// Check if the value is a decimal integer
// (no floating point):
bool is_dec = csv[0][0].isDecimal();

// Check if the value is a char:
bool is_char = csv[0][0].isChar();
```

### Get the csv string
In order to get the csv string, just use the ``to_string()`` function:
```c++
std::string csv_str = csv.to_string();
```

### Stream operations
Writing the csv object as a string to a stream:
```c++
// Write to stdout:
std::cout << csv;

// Write to a file:
std::ofstream ofs("file.csv");
ofs << csv;
```

Reading a csv object from a stream:
```c++
// Using a file stream:
std::ifstream ifs("file.csv");

markusjx::csv csv;
ifs >> csv;
```
This will read the whole file to a string and parse that value.

### Wide string operations
The whole arsenal of functions is also available for wide chars using ``std::wstring``.
**If any non-wide string values are added to the object, they will be converted to a wide string value.
Please note that these values can't be converted back to non-wide strings.**

#### Instantiation
Plain csv object:
```c++
markusjx::w_csv wcsv;
```

From string:
```c++
// (Almost) the same as with non-wide chars
markusjx::w_csv wcsv = L"abc;def;123"__csv;

// Using parse...
markusjx::w_csv wcsv = markusjx::w_csv::parse(L"abc;def;123");

// Using parse while supplying a non-wide string
markusjx::w_csv wcsv = markusjx::w_csv::parse("abc;def;123");
```

Using ``std::initializer_list``:
```c++
markusjx::w_csv wcsv = {
  {"abc", L"def", 123}
};
```

#### Writing data to the object
Using ``operator<<`` or ``push()``:
```c++
wcsv << "abc" << L"def" << std::string("some") << std::wstring(L"data") << 1234;
```

Using ``operator[]`` or ``at()``:
```c++
wcsv[0][0] = "abc";
wcsv[0][1] = L"def";

wcsv[0] = {"abd", L"def", 123};
```

Lines may still be ended using ``std::endl`` or ``static markusjx::csv::endl`` or ``markusjx::csv::endline()``.

#### Accessing data
The data can be accessed using the same methods as when using non-wide strings.
Only the access to strings is different as std::strings can't be read from ``markusjx::w_csv`` objects:
```c++
// Will work:
std::wstring data = wcsv[0][0];

// Won't work (compile-time error):
std::string data = wcsv[0][0];
```

### Working with csv files
To reduce the memory footprint of your application, you can use the ``csv_file`` class to work with csv files.
For working with wide strings use the ``w_csv_file`` class.

#### Instantiation
Read a file called ``file.csv``:
```c++
csv_file file("file.csv");
```

#### Reading/Writing data
Read and write operations are equal to those in ``markusjx::basic_csv``:
```c++
// Using operator<<
file << "abc" << 123 << false;

// Using operator[]
file[1][0] = 456;

// Reading is also the same
int read = file[1][0];

// Delete a row
file.remove(0);

// End lines either using
file << markusjx::csv_file::endl;
// or
file << markusjx::csv::endl;
// or
file << std::endl;
//or
file.endline();
```

Iterate over the file:
```c++
// Using indices
for (uint64_t i = 0; i < file.size(); i++) {
    const auto row = file[i];
}

// Using iterators
for (const auto row : file) {
    // ...
}
```
Please note that the const iterator does not return
references to the data but copies of the rows.

Write all unwritten changes to the actual file:
```c++
file.flush();
```

#### Conversion between ``basic_csv_file`` and ``basic_csv``
##### Reading a ``basic_csv_file`` to a ``basic_csv`` object
This will call ``basic_csv_file::flush()`` which will
write the whole contents of the data cache to the file
before reading everything into the csv file. The csv
object may or may not contain data, the read rows
will be appended to the already existing rows.
No already existing rows will be modified.
```c++
markusjx::csv_file file("file.csv");
markusjx::csv csv;

// Read all data into the csv object
file >> csv;
```

##### Writing a ``basic_csv`` object to a ``basic_csv_file``
When writing to a file, the file may or may not already
contain data. If it does already contain data, the rows
of the csv object will be appended to the list of already
existing rows in the file. Once the operation is complete,
all data will be immediately written to the file, ignoring
the maximum cache size. After the operation, the cache will
be empty.
```c++
markusjx::csv_file file("file.csv");
markusjx::csv csv;

// Write some data to the csv object...

// Write the csv object to the file
file << csv;

// Or: get the csv object
csv = file.get_basic_csv();
```

#### Implementation notes
* ``markusjx::basic_csv_file`` is **not** RFC 4180 compliant, writing new lines to cells is prohibited
* All values written are first written to a cache whose max size can be set using the
  second argument in the ``markusjx::basic_csv_file`` constructor
* The csv files are also available as utf-16 using ``w_csv_file``
* ``strip()`` is not available for ``markusjx::basic_csv_file``

## Supported data types
The supported data types are all types that are
convertible from and to string,
which are the following:
* ``int``
* ``long int``
* ``unsigned long int``
* ``long long int``
* ``unsigned long long int``
* ``double``
* ``long double``
* ``float``
* ``bool``
* ``const char *`` (write-only, not readable)
* ``std::string`` (when using utf-8/ASCII strings, write-only when using utf-16 strings)
* ``char`` (when using utf-8/ASCII strings, write-only when using utf-16 strings)
* ``const wchar_t *`` (write-only, not readable, only available when using utf-16 strings)
* ``std::wstring`` (when using utf-16 strings)
* ``wchar_t`` (when using utf-16 strings)

**Note: Any value can be converted to a string value**

## General implementation notes
* A cell may contain a new line or a separator.
  In this case, the whole cell value will be wrapped in double quotes (RFC 4180 section 2.6).
* A cell may contain double quotes.
  In this case, any double quotes will be escaped using another double quote (RFC 4180 section 2.6).
* The default separator is ``';'`` (default for european excel versions). See below for further information.

## Changing the separator
If you want to change the default separator (``';'``) for the default template instantiations
(``markusjx::csv``, ``markusjx::w_csv``, ``markusjx::csv_file`` and ``markusjx::w_csv_file``),
set the ``MARKUSJX_CSV_SEPARATOR`` macro to your preferred separator before including ``csv.hpp``:
```c++
// Set separator to ','
// (default for US versions of Excel)
#define MARKUSJX_CSV_SEPARATOR ','

#include <csv.hpp>
```

Another option is to define your own template instantiations:
```c++
// Define a custom csv implementation using
// ASCII strings and ',' as a separator
using custom_csv = markusjx::basic_csv<std::string, ','>;

// Define a custom csv file type
using custom_csv_file = markusjx::basic_csv_file<std::string, ','>;
```

## License
This project is licensed under the [MIT License](LICENSE).
