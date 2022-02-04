// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <Python.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "main.h"
#include "config.h"
#include "emulate.h"
#include "helper.h"

struct ctx_t _ctx;
struct ctx_t *ctx;

static PyObject *anergistic_execute(PyObject *self, PyObject *args)
{
	Py_buffer local_store, registers;
	int pc;
	PyObject *breakpoints = NULL;
	PyObject *breakpoints_insns = NULL;

	(void)self;
	if (!PyArg_ParseTuple(args, "y*y*i|OO",
						  &local_store,
						  &registers,
						  &pc,
						  &breakpoints,
						  &breakpoints_insns))
		return NULL;

	// XXX: why is (int) required?
	if ((int)local_store.len != 256 * 1024)
	{
		PyErr_SetString(PyExc_TypeError, "The local storage must be a 256kb string array");
		goto ERROR;
	}

	// XXX: why is (int) required?
	if ((int)registers.len != 128 * 16)
	{
		PyErr_SetString(PyExc_TypeError, "The registers must be a 128*16 string array");
		goto ERROR;
	}

	if (pc < 0 || (pc >= 256 * 1024) || (pc & 3))
	{
		PyErr_SetString(PyExc_TypeError, "PC must be aligned pointer within ls");
		goto ERROR;
	}

	memset(&_ctx, 0, sizeof _ctx);
	ctx = &_ctx;
	ctx->ls = local_store.buf;
	ctx->pc = pc;

	int i;
	for (i = 0; i < 128; ++i)
		byte_to_reg(i, (u8 *)registers.buf + i * 16);

	ctx->paused = 0;
	ctx->trap = 1;

	ctx->pc &= LSLR;

	while (emulate() == 0)
	{
		if (breakpoints)
		{
			PyObject *pc = PyLong_FromLong(ctx->pc);
			if (PySet_Contains(breakpoints, pc))
			{
				Py_DECREF(pc);
				break;
			}
			Py_DECREF(pc);
		}
		if (breakpoints_insns)
		{
			PyObject *pc = PyLong_FromLong(be32(ctx->ls + ctx->pc) >> 21);
			if (PySet_Contains(breakpoints_insns, pc))
			{
				Py_DECREF(pc);
				break;
			}
			Py_DECREF(pc);
		}
		if (PyErr_CheckSignals() || PyErr_Occurred())
		{
			goto ERROR;
		}
	}

	for (i = 0; i < 128; ++i)
		reg_to_byte((u8 *)registers.buf + i * 16, i);

	PyBuffer_Release(&local_store);
	PyBuffer_Release(&registers);

	return PyLong_FromLong(ctx->pc);

ERROR:
	PyBuffer_Release(&local_store);
	PyBuffer_Release(&registers);
	return NULL;
}

void fail(const char *a, ...)
{
	char msg[1024];
	va_list va;

	va_start(va, a);
	vsnprintf(msg, sizeof msg, a, va);
	PyErr_SetString(PyExc_RuntimeError, msg);
}

static PyMethodDef anergistic_methods[] =
	{
		{"execute", anergistic_execute, METH_VARARGS, "execute"},
		{NULL, NULL, 0, NULL}};

static struct PyModuleDef moduledef =
	{
		PyModuleDef_HEAD_INIT,
		"anergistic",
		NULL,
		-1,
		anergistic_methods,
		NULL,
		NULL,
		NULL,
		NULL};

PyObject *PyInit_anergistic(void)
{
	return PyModule_Create(&moduledef);
}
