from microscopes.cxx.common.recarray.dataview import numpy_dataview as recarray_numpy_dataview
from microscopes.cxx.common.sparse_ndarray.dataview import numpy_dataview as sparse_ndarray_numpy_dataview
from microscopes.cxx.common.rng import rng

import numpy as np
import numpy.ma as ma

from nose.tools import assert_almost_equals

def test_recarray_numpy_dataview():
    x = np.array([(False, 32.), (True, 943.), (False, -32.)], dtype=[('', bool), ('', float)])
    view = recarray_numpy_dataview(x)
    assert view and view.size() == x.shape[0]
    assert len(view) == x.shape[0]

    for a, b in zip(x, view):
        assert a[0] == b[0]
        assert_almost_equals(a[1], b[1])

    r = rng()
    acc = 0
    for a in view.view(True, r):
        acc += a[0]
    assert acc == sum(e[0] for e in x)

def test_recarray_numpy_dataview_subarray():
    x = np.array([(1, (2., 3.)), (-1, (-3., 54.))], dtype=[('', np.int32), ('', np.float32, (2,))])
    view = recarray_numpy_dataview(x)
    assert view and view.size() == x.shape[0]

    for a, b in zip(x, view):
        assert a[0] == b[0]
        assert a[1].shape == b[1].shape
        for f, g in zip(a[1], b[1]):
            assert_almost_equals(f, g)

def test_recarray_numpy_dataview_masked():
    x = ma.masked_array(
        np.array([(True, False, True, True, True)], dtype=[('', np.bool)]*5),
        mask=[(False, False, True, True, True)])
    view = recarray_numpy_dataview(x)
    assert view and view.size() == x.shape[0]

    for a, b in zip(x, view):
        assert a.mask == b.mask
        for mask, (aval, bval) in zip(a.mask, zip(a, b)):
            if mask:
                continue
            assert aval == bval

def test_sparse_ndarray_numpy_dataview():
    x = np.zeros((2, 3, 4), dtype=np.bool)
    view = sparse_ndarray_numpy_dataview(x)
    assert view
    assert view.shape() == (2, 3, 4)

def test_sparse_ndarray_numpy_dataview_masked():
    x = np.zeros((2, 3, 4), dtype=np.bool)
    x = ma.masked_array(x, mask=np.ones(x.shape))
    view = sparse_ndarray_numpy_dataview(x)
    assert view
    assert view.shape() == (2, 3, 4)
