/* Matrix math. */
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "matrix.h"

const Matrix_t * init_matrix(const Matrix_t * m, double* raw_data)
{
	uint8_t i;
	double** data_rows = m->data;
	
	for (i = 0; i < m->rows; ++i)
	{
		data_rows[i] = raw_data;
		raw_data += m->cols;
	}
	return m;
}

/*******************************************************************************
*
* Function Name	:	set_matrix
*
* Description	:	Set values of a matrix from variable list
*
*******************************************************************************/
void set_matrix(const Matrix_t * m, ...)
{
	uint8_t i,j;
	va_list ap;
	va_start(ap, m);
	
	for (i = 0; i < m->rows; ++i)
	{
		for (j = 0; j < m->cols; ++j)
		{
			m->data[i][j] = va_arg(ap, double);
		}
	}
	va_end(ap);
}

/*******************************************************************************
*
* Function Name	:	set_matrix_const
*
* Description	:	Set values of a matrix from array
*
*******************************************************************************/
void set_matrix_const(const Matrix_t * m, const double * data)
{
	uint8_t i,j;
	for (i = 0; i < m->rows; ++i)
	{
		for (j = 0; j < m->cols; ++j)
		{
			m->data[i][j] = *data++;
		}
	}
}

/*******************************************************************************
*
* Function Name	:	set_identity_matrix
*
* Description	:	Turn m into an identity (diagonal) matrix.
*
*******************************************************************************/
void set_identity_matrix(const Matrix_t * m)
{
	uint8_t i,j;
	for (i = 0; i < m->rows; ++i)
	{
		for (j = 0; j < m->cols; ++j)
		{
			if (i == j)
				m->data[i][j] = 1.0;
			else
				m->data[i][j] = 0.0;
		}
	}
}

/*******************************************************************************
*
* Function Name	:	copy_matrix
*
* Description	:	Copy a matrix.
*
*******************************************************************************/
void copy_matrix(const Matrix_t * src, const Matrix_t * dst)
{
	int i,j;
	for (i = 0; i < src->rows; ++i)
	{
		for (j = 0; j < src->cols; ++j)
		{
			dst->data[i][j] = src->data[i][j];
		}
	}
}

/*******************************************************************************
*
* Function Name	:	add_matrix
*
* Description	:	Add matrices a and b and put the result in c.
*
*******************************************************************************/
void add_matrix(const Matrix_t * a, const Matrix_t * b, const Matrix_t * c)
{
	uint8_t i,j;
	for (i = 0; i < a->rows; ++i)
		for (j = 0; j < a->cols; ++j)
			c->data[i][j] = a->data[i][j] + b->data[i][j];
}

/*******************************************************************************
*
* Function Name	:	subtract_matrix
*
* Description	:	Subtract matrices a and b and put the result in c.
*
*******************************************************************************/
void subtract_matrix(const Matrix_t * a, const Matrix_t * b, const Matrix_t * c)
{
	uint8_t i,j;
	for (i = 0; i < a->rows; ++i)
		for (j = 0; j < a->cols; ++j)
			c->data[i][j] = a->data[i][j] - b->data[i][j];
}

/*******************************************************************************
*
* Function Name	:	subtract_from_identity_matrix
*
* Description	:	Subtract from the identity matrix in place.
*
*******************************************************************************/
void subtract_from_identity_matrix(const Matrix_t * a)
{
	uint8_t i,j;
	for (i = 0; i < a->rows; ++i)
	{
		for (j = 0; j < a->cols; ++j)
		{
			if (i == j)
				a->data[i][j] = 1.0 - a->data[i][j];
			else
				a->data[i][j] = 0.0 - a->data[i][j];
		}
	}
}

void multiply_matrix(const Matrix_t * a, const Matrix_t * b, const Matrix_t * c)
{
	uint8_t i,j,k;
	for (i = 0; i < c->rows; ++i)
		for (j = 0; j < c->cols; ++j)
		{
			/* Calculate element c.data[i][j] via a dot product of one row of a
			with one column of b */
			c->data[i][j] = 0.0;
			for (k = 0; k < a->cols; ++k)
				c->data[i][j] += a->data[i][k] * b->data[k][j];
		}
}

/*
	This is multiplying a by b-tranpose so it is like multiply_matrix
	but references to b reverse rows and cols.
 */
void multiply_by_transpose_matrix(const Matrix_t * a, const Matrix_t * b, const Matrix_t * c)
{
	uint8_t i,j,k;
	for (i = 0; i < c->rows; ++i)
		for (j = 0; j < c->cols; ++j)
		{
			/* Calculate element c.data[i][j] via a dot product of one row of a
			with one row of b */
			c->data[i][j] = 0.0;
			for (k = 0; k < a->cols; ++k)
				c->data[i][j] += a->data[i][k] * b->data[j][k];
		}
}

void transpose_matrix(const Matrix_t * src, const Matrix_t * dst)
{
	uint8_t i,j;
	for (i = 0; i < src->rows; ++i)
		for (j = 0; j < src->cols; ++j)
			dst->data[j][i] = src->data[i][j];
}

int equal_matrix(const Matrix_t * a, const Matrix_t * b, double tolerance)
{
	uint8_t i,j;
	for (i = 0; i < a->rows; ++i)
		for (j = 0; j < a->cols; ++j)
			if (abs(a->data[i][j] - b->data[i][j]) > tolerance)
				return 0;
	return 1;
}

void scale_matrix(const Matrix_t * m, double scalar)
{
	uint8_t i,j;
	for (i = 0; i < m->rows; ++i)
		for (j = 0; j < m->cols; ++j)
			m->data[i][j] *= scalar;
}

void swap_rows(const Matrix_t * m, int r1, int r2)
{
	double* tmp = m->data[r1];
	m->data[r1] = m->data[r2];
	m->data[r2] = tmp;
}

void scale_row(const Matrix_t * m, int r, double scalar)
{
	uint8_t i;
	for (i = 0; i < m->cols; ++i)
		m->data[r][i] *= scalar;
}

/* Add scalar * row r2 to row r1. */
void shear_row(const Matrix_t * m, int r1, int r2, double scalar)
{
	uint8_t i;
	for (i = 0; i < m->cols; ++i)
		m->data[r1][i] += scalar * m->data[r2][i];
}

/* Uses Gauss-Jordan elimination.

   The elimination procedure works by applying elementary row
   operations to our input matrix until the input matrix is reduced to
   the identity matrix.
   Simultaneously, we apply the same elementary row operations to a
   separate identity matrix to produce the inverse matrix.
   If this makes no sense, read wikipedia on Gauss-Jordan elimination.
   
   This is not the fastest way to invert matrices, so this is quite
   possibly the bottleneck. */
int destructive_invert_matrix(const Matrix_t * input, const Matrix_t * output)
{
	uint8_t i,j,r;
	double scalar, shear_needed;
	set_identity_matrix(output);

	/*
		Convert input to the identity matrix via elementary row operations.
		The ith pass through this loop turns the element at i,i to a 1
		and turns all other elements in column i to a 0.
	*/
	for (i = 0; i < input->rows; ++i)
	{
		if (input->data[i][i] == 0.0)
		{
			/* We must swap rows to get a nonzero diagonal element. */
			for (r = i + 1; r < input->rows; ++r)
			{
				if (input->data[r][i] != 0.0)
					break;
			}
			if (r == input->rows)
				/* Every remaining element in this column is zero, so this
				matrix cannot be inverted. */
				return 0;
			swap_rows(input, i, r);
			swap_rows(output, i, r);
		}

		/*
			Scale this row to ensure a 1 along the diagonal.
			We might need to worry about overflow from a huge scalar here. */
		scalar = 1.0 / input->data[i][i];
		scale_row(input, i, scalar);
		scale_row(output, i, scalar);

		/* Zero out the other elements in this column. */
		for (j = 0; j < input->rows; ++j)
		{
			if (i == j)
				continue;
			shear_needed = -input->data[j][i];
			shear_row(input, j, i, shear_needed);
			shear_row(output, j, i, shear_needed);
		}
	}
	return 1;
}
