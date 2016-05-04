from __future__ import print_function

from timeit import timeit
from reg.fastmapply import lookup_mapply as c_lookup_mapply
from reg.fastmapply import mapply as c_mapply
from reg.mapply import py_lookup_mapply, py_mapply


class Foo(object):
    def __init__(self, a):
        self.a = a


class FooLookup(object):
    def __init__(self, a, lookup):
        self.a = a


def foo(a):
    return a


def bar(a, b, c, d):
    return a, b, c, d


def foo_lookup(a, lookup):
    return a


def bar_lookup(a, b, c, d, lookup):
    return a, b, c, d


class Bar(object):
    def __call__(self, a):
        return a


class BarLookup(object):
    def __call__(self, a, lookup):
        return a


the_thing = foo_lookup


def c_generic():
    c_mapply(the_thing, 1, lookup='lookup')


def c_specialized():
    c_lookup_mapply(the_thing, 'lookup', 1)


def py_generic():
    py_mapply(the_thing, 1, lookup='lookup')


def py_specialized():
    py_lookup_mapply(the_thing, 'lookup', 1)


def direct():
    the_thing(1, lookup='lookup')


def main():
    print("Python version specialized:")
    print(timeit(py_specialized))

    print("Python version generic:")
    print(timeit(py_generic))

    print("C version generic:")
    print(timeit(c_generic))

    print("C version specialized:")
    print(timeit(c_specialized))

    print("Direct call:")
    print(timeit(direct))

if __name__ == '__main__':
    main()
