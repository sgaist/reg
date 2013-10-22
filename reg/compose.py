"""
Compose ClassLookups. ListClassLookup and ChainClassLookup are different
ways to compose ClassLookups together into a single one. CachedClassLookup
is a caching version of ClassLookup.
"""

from .registry import IClassLookup

CACHED_SENTINEL = object()


class ListClassLookup(IClassLookup):
    """A simple list of class lookups functioning as an IClassLookup.

    Go through all items in the list, starting at the beginning and
    try to find the component. If found in a lookup, return it right away.
    """

    def __init__(self, lookups):
        self.lookups = lookups

    def get(self, func, classes):
        for lookup in self.lookups:
            result = lookup.get(func, classes)
            if result is not None:
                return result
        return None

    def all(self, func, classes):
        for lookup in self.lookups:
            for component in lookup.all(func, classes):
                if component is not None:
                    yield component


class ChainClassLookup(IClassLookup):
    """Chain a class lookup on top of another class lookup.

    Look in the supplied IClassLookup object first, and if not found, look
    in the next IClassLookup object. This way multiple IClassLookup objects
    can be chained together.
    """

    def __init__(self, lookup, next):
        self.lookup = lookup
        self.next = next

    def get(self, func, classes):
        result = self.lookup.get(func, classes)
        if result is not None:
            return result
        return self.next.get(func, classes)

    def all(self, func, classes):
        for component in self.lookup.all(func, classes):
            yield component
        for component in self.next.all(func, classes):
            yield component


class CachedClassLookup(IClassLookup):
    def __init__(self, class_lookup):
        self.class_lookup = class_lookup
        self._cache = {}
        self._all_cache = {}

    def get(self, func, classes):
        classes = tuple(classes)
        component = self._cache.get((func, classes), CACHED_SENTINEL)
        if component is not CACHED_SENTINEL:
            return component
        component = self.class_lookup.get(func, classes)
        self._cache[(func, classes)] = component
        return component

    def all(self, func, classes):
        classes = tuple(classes)
        result = self._all_cache.get((func, classes))
        if result is not None:
            return result
        result = list(self.class_lookup.all(func, classes))
        self._all_cache[(func, classes)] = result
        return result