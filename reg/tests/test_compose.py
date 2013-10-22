from reg.registry import ClassRegistry as Registry
from reg.interface import Interface
from reg.compose import (
    ListClassLookup, ChainClassLookup, CachedClassLookup)


def target():
    pass


def test_list_class_lookup():
    reg1 = Registry()
    reg2 = Registry()

    reg2.register(target, (), 'reg2 component')

    lookup = ListClassLookup([reg1, reg2])
    assert lookup.get(target, ()) == 'reg2 component'

    reg1.register(target, (), 'reg1 component')
    assert lookup.get(target, ()) == 'reg1 component'


def test_list_class_lookup_all():
    reg1 = Registry()
    reg2 = Registry()

    reg1.register(target, (), 'reg1')
    reg2.register(target, (), 'reg2')

    lookup = ListClassLookup([reg1, reg2])
    assert list(lookup.all(target, ())) == ['reg1', 'reg2']


def test_chain_class_lookup():
    reg1 = Registry()
    reg2 = Registry()

    reg2.register(target, (), 'reg2 component')

    lookup = ChainClassLookup(reg1, reg2)
    assert lookup.get(target, ()) == 'reg2 component'

    reg1.register(target, (), 'reg1 component')
    assert lookup.get(target, ()) == 'reg1 component'


def test_chain_class_lookup_all():
    reg1 = Registry()
    reg2 = Registry()

    reg1.register(target, (), 'reg1')
    reg2.register(target, (), 'reg2')

    lookup = ChainClassLookup(reg1, reg2)
    assert list(lookup.all(target, ())) == ['reg1', 'reg2']


def test_cached_class_lookup():
    reg = Registry()

    reg.register(target, (), 'reg component')

    cached = CachedClassLookup(reg)

    assert cached.get(target, ()) == 'reg component'

    # we change the registration
    reg.register(target, (), 'reg component changed')

    # the cache won't know
    assert cached.get(target, ()) == 'reg component'


def test_cached_class_lookup_all():
    reg = Registry()

    reg.register(target, (), 'reg component')

    cached = CachedClassLookup(reg)

    assert list(cached.all(target, ())) == ['reg component']

    # we change the registration
    reg.register(target, (), 'reg component changed')

    # the cache won't know
    assert list(cached.all(target, ())) == ['reg component']