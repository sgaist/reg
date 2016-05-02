#include <Python.h>

#if PY_MAJOR_VERSION >= 3
#define IS_PY3
#endif

PyCodeObject* get_code(PyObject* callable_obj, PyObject** method_obj_out,
                       int* init_no_args) {
  PyObject* func_obj;
  PyObject* method_obj = NULL;

  if (PyFunction_Check(callable_obj)) {
    /* function */
    func_obj = callable_obj;
  } else if (PyMethod_Check(callable_obj)) {
    /* method */
    func_obj = PyMethod_Function(callable_obj);
  } else if (PyType_Check(callable_obj)) {
    /* new style class */
    method_obj = PyObject_GetAttrString(callable_obj, "__init__");
#ifndef IS_PY3
    /* in Python 2 __init__ is a method */
    if (PyMethod_Check(method_obj)) {
      func_obj = PyMethod_Function(method_obj);
    } else {
      /* something else found, not method, so no __init__ */
      func_obj = NULL;
      *init_no_args = 1;
    }
#else
    /* in Python 3 __init__ is a function */
    if (PyFunction_Check(method_obj)) {
      func_obj = method_obj;
    } else {
      /* something else found, not function, so no __init__ */
      func_obj = NULL;
      *init_no_args = 1;
    }
#endif

#ifndef IS_PY3
    /* old style classes only exist in Python 2 */
  } else if (PyClass_Check(callable_obj)) {
    /* old style class */
    method_obj = PyObject_GetAttrString(callable_obj, "__init__");
    if (method_obj != NULL) {
      func_obj = PyMethod_Function(method_obj);
    } else {
      PyErr_SetString(PyExc_TypeError,
                    "old-style classes without __init__ not supported");
      return NULL;
    }
#endif
  } else if (PyCFunction_Check(callable_obj)) {
    /* function implemented in C extension */
    PyErr_SetString(PyExc_TypeError,
                    "functions implemented in C are not supported");
    return NULL;
  } else {
    /* new or old style class instance */
    method_obj = PyObject_GetAttrString(callable_obj, "__call__");
    if (method_obj != NULL) {
      func_obj = PyMethod_Function(method_obj);
    } else {
#ifndef IS_PY3
      /* old style classes only exist in Python 2 */
      if (PyInstance_Check(callable_obj)) {
        PyErr_SetString(PyExc_AttributeError,
                        "Instance has no __call__ method");
      } else {
#endif
        PyErr_SetString(PyExc_TypeError,
                        "Instance is not callable");
#ifndef IS_PY3
      }
#endif
      return NULL;
    }
  }

  // output method parameter
  *method_obj_out = method_obj;

  if (func_obj == NULL) {
    return NULL;
  }
  /* we can determine the arguments now */
  return (PyCodeObject*)PyFunction_GetCode(func_obj);
}

static PyObject*
mapply(PyObject *self, PyObject *args, PyObject* kwargs)
{
  PyObject* callable_obj;
  PyObject* remaining_args;
  PyObject* result;
  PyCodeObject* co;
  PyObject* method_obj = NULL;
  PyObject* new_kwargs = NULL;
  int i, init_no_args = 0;

  if (PyTuple_GET_SIZE(args) < 1) {
    PyErr_SetString(PyExc_TypeError,
                    "mapply() takes at one parameter");
    return NULL;
  }

  callable_obj = PyTuple_GET_ITEM(args, 0);
  remaining_args = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));

  co = get_code(callable_obj, &method_obj, &init_no_args);

  if (init_no_args) {
    /* use new_kwargs that is NULL, as init wants no args */
    goto final;
  }

  /* if we got no keyword parameters anyway,
     or if the target function takes keyword parameters */
  if (kwargs == NULL ||
      (co != NULL && co->co_flags & CO_VARKEYWORDS)) {
    /* we reuse the existing kwargs, but incref so we can decref symmetrically
       later */
    new_kwargs = kwargs;
    Py_XINCREF(new_kwargs);
    goto final;
  }

  /* an error was raised in get_code */
  if (co == NULL) {
    return NULL;
  }

  /* create a new keyword argument dict with only the desired args */
  new_kwargs = PyDict_New();

  for (i = 0; i < co->co_argcount; i++) {
    PyObject* name = PyTuple_GET_ITEM(co->co_varnames, i);
    PyObject* value = PyDict_GetItem(kwargs, name);
    if (value != NULL) {
      PyDict_SetItem(new_kwargs, name, value);
    }
  }

 final:
  /* call the underlying function */
  result = PyObject_Call(callable_obj, remaining_args, new_kwargs);

  /* cleanup */
  Py_DECREF(remaining_args);
  Py_XDECREF(new_kwargs); /* can be null */
  Py_XDECREF(method_obj); /* can be null */
  return result;
}

static PyObject*
lookup_mapply(PyObject *self, PyObject *args, PyObject* kwargs)
{
  PyObject* callable_obj;
  PyObject* lookup_obj;
  PyObject* remaining_args;
  PyObject* result;
  PyCodeObject* co;
  PyObject* method_obj = NULL;
  int i, has_lookup = 0, cleanup_dict = 0, init_no_args = 0;

  if (PyTuple_GET_SIZE(args) < 2) {
    PyErr_SetString(PyExc_TypeError,
                    "lookup_mapply() takes at least two parameters");
    return NULL;
  }

  callable_obj = PyTuple_GET_ITEM(args, 0);
  lookup_obj = PyTuple_GET_ITEM(args, 1);
  remaining_args = PyTuple_GetSlice(args, 2, PyTuple_GET_SIZE(args));

  co = get_code(callable_obj, &method_obj, &init_no_args);

  if (init_no_args) {
    /* init wants no args, so don't send any */
    kwargs = NULL;
    goto final;
  }

  if (co != NULL && co->co_flags & CO_VARKEYWORDS) {
    goto final;
  }

  /* an error was raised in get_code */
  if (co == NULL) {
    return NULL;
  }

  PyObject* lookup_str = PyUnicode_FromString("lookup");

  for (i = 0; i < co->co_argcount; i++) {
    PyObject* name = PyTuple_GET_ITEM(co->co_varnames, i);
    if (PyObject_RichCompareBool(lookup_str, name, Py_EQ)) {
      has_lookup = 1;
      break;
    }
  }
  if (has_lookup) {
    if (kwargs == NULL) {
      kwargs = PyDict_New();
      cleanup_dict = 1;
    }
    PyDict_SetItem(kwargs, lookup_str, lookup_obj);
  }
 final:
  result = PyObject_Call(callable_obj, remaining_args, kwargs);

  Py_DECREF(remaining_args);
  if (cleanup_dict) {
    Py_DECREF(kwargs);
  }
  Py_XDECREF(method_obj);
  return result;
}

static PyMethodDef FastMapplyMethods[] = {
  {"lookup_mapply", (PyCFunction)lookup_mapply, METH_VARARGS | METH_KEYWORDS,
   "apply with optional lookup parameter"},
  {"mapply", (PyCFunction)mapply, METH_VARARGS | METH_KEYWORDS,
   "mapply"},

  {NULL, NULL, 0, NULL}
};

#ifdef IS_PY3
static struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "fastmapply",
  NULL,
  0,
  FastMapplyMethods,
  NULL,
  NULL,
  NULL,
  NULL
};
#endif

PyMODINIT_FUNC
#ifndef IS_PY3
initfastmapply(void)
#else
  PyInit_fastmapply(void)
#endif
{
#ifndef IS_PY3
  (void) Py_InitModule("fastmapply", FastMapplyMethods);
#else
  return PyModule_Create(&moduledef);
#endif
}