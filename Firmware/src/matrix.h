#ifndef __MATRIX_H__
#define __MATRIX_H__

typedef struct {
	double** data;			// Contents of the matrix
	const uint8_t rows;	// Dimensions
	const uint8_t cols;
} Matrix_t;

/*	Initialize matrix */
const Matrix_t * init_matrix(const Matrix_t * m, double* raw_data);

void set_matrix(const Matrix_t *, ...);
void set_matrix_const(const Matrix_t *, const double *);
void set_identity_matrix(const Matrix_t *);
void copy_matrix(const Matrix_t * src, const Matrix_t * dst);
void add_matrix(const Matrix_t * a, const Matrix_t * b, const Matrix_t * c);
void subtract_matrix(const Matrix_t * a, const Matrix_t * b, const Matrix_t * c);
void subtract_from_identity_matrix(const Matrix_t * a);

/* Multiply matrices a and b and put the result in c. */
void multiply_matrix(const Matrix_t * a, const Matrix_t * b, const Matrix_t * c);

/* Multiply matrix a by b-transpose and put the result in c. */
void multiply_by_transpose_matrix(const Matrix_t * a, const Matrix_t * b, const Matrix_t * c);

/* Transpose input and put the result in output. */
void transpose_matrix(const Matrix_t * src, const Matrix_t * dst);

/* Whether two matrices are approximately equal. */
int equal_matrix(const Matrix_t * a, const Matrix_t * b, double tolerance);

/* Multiply a matrix by a scalar. */
void scale_matrix(const Matrix_t * m, double scalar);

/* Swap rows r1 and r2 of a matrix.
   This is one of the three "elementary row operations". */
void swap_rows(const Matrix_t * m, int r1, int r2);

/* Multiply row r of a matrix by a scalar.
   This is one of the three "elementary row operations". */
void scale_row(const Matrix_t * m, int r, double scalar);

/* Add a multiple of row r2 to row r1.
   Also known as a "shear" operation.
   This is one of the three "elementary row operations". */
void shear_row(const Matrix_t * m, int r1, int r2, double scalar);

/* Invert a square matrix.
   Returns whether the matrix is invertible.
   input is mutated as well by this routine. */
int destructive_invert_matrix(const Matrix_t * input, const Matrix_t * output);

#endif
