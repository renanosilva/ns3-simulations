#! /usr/bin/env python3

# A list of C++ examples to run in order to ensure that they remain
# buildable and runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run, do_valgrind_run).
#
# See test.py for more information.
cpp_examples = [
    ("first", "True", "True"),
    ("hello-simulator", "True", "True"),
    ("second", "True", "True"),
    ("third", "True", "True"),
    ("fourth", "True", "True"),
    ("fifth", "True", "True"),
    ("sixth", "True", "True"),
    ("seventh", "True", "True"),
]
 
# A list of Python examples to run in order to ensure that they remain
# runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run).
#
# See test.py for more information.
python_examples = [
    ("first.py", "True"),
]
