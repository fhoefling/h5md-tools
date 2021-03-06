/* C extension for computing the static structure factor
 *
 * Copyright © 2008-2010  Felix Höfling, Peter Colberg
 *
 * This file is part of mdplot.
 *
 * mdplot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Python.h>

#define PY_ARRAY_UNIQUE_SYMBOL Py_Array_API_mdplot_ext
#define NO_IMPORT_ARRAY
#include <numpy/ndarrayobject.h>

// forward declaration
bool is_float_matrix(PyArrayObject *a);

PyObject *_static_structure_factor(PyObject *self, PyObject *args)
{
    PyArrayObject *q, *r;

    if (!PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &q, &PyArray_Type, &r)
            || q == NULL || !is_float_matrix(q)
            || r == NULL || !is_float_matrix(r))  {
        return NULL;
    }

    // Check that objects are of equal 2nd dimension
    if (PyArray_DIM(q, 1) != PyArray_DIM(r, 1)) {
        PyErr_SetString(PyExc_ValueError,
            "wavenumber array and coordinate array must be of equal 2nd dimension.");
        return NULL;
    }
    unsigned dimension = PyArray_DIM(q, 1);
    unsigned nq = PyArray_DIM(q, 0);
    unsigned npart = PyArray_DIM(r, 0);

    // loop over wavevectors
    double result = 0;
// #   pragma omp parallel
    {
    for (unsigned i=0; i < nq; i++) {
        double sin_sum = 0;
        double cos_sum = 0;
        // loop over particles
#       pragma omp parallel for reduction(+:sin_sum,cos_sum)
        for (unsigned j=0; j < npart; j++) {
            // q_r = inner(q, r)
            float q_r = 0;
            for (unsigned k=0; k < dimension; k++) {
                q_r += *(float*)PyArray_GETPTR2(q, i, k) * *(float*)PyArray_GETPTR2(r, j, k);
            }
            // on old platforms/compilers one may prefer sincos(q_r, &s, &c)
            sin_sum += sinf(q_r);  // single precision should be sufficient here
            cos_sum += cosf(q_r);
        }
// #       pragma omp master
        {
        result +=  sin_sum * sin_sum + cos_sum * cos_sum;
        }
    }
    }
    result /= nq * npart;

    return Py_BuildValue("d", result);
}

bool is_float_matrix(PyArrayObject *a)
{   if (PyArray_TYPE(a) != NPY_FLOAT || a->nd != 2)
    {
        PyErr_SetString(PyExc_ValueError,
            "array must be of type Float and 2-dimensional.");
        return false;
    }
    return true;
}

