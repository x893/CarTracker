#ifndef __KALMAN_H__
#define __KALMAN_H__

#if ( USE_AVERAGE == 1)

#include "nmea/time.h"
#include "gps.h"

#define MAX_AVERAGE_COUNT	30

#define KMH2KNOT			1.852

#define AVERAGE_SPEED_5		(5.0 / KMH2KNOT)
#define AVERAGE_SUM_5		MAX_AVERAGE_COUNT

#define AVERAGE_SPEED_10	(10.0 / KMH2KNOT)
#define AVERAGE_SUM_10		20

#define AVERAGE_SPEED_20	(20.0 / KMH2KNOT)
#define AVERAGE_SUM_20		5

#define AVERAGE_SUM_MORE	1

void FilterAverageUpdate(GpsContext_t * gc, double lat, double lon, uint32_t time);
uint8_t  FilterAverageGet(GpsContext_t * gc, double *lat, double *lon);
void FilterAverageInit(GpsContext_t * gc);

#endif


#if ( USE_KALMAN == 1)

#include "matrix.h"

#define KALMAN_STATE		4
#define KALMAN_OBSERVATION	2


typedef struct {
	
	int state_dimension;					/* These parameters define the size of the matrices. */
	int observation_dimension;

	const Matrix_t * state_transition;				/* F_k */
	const Matrix_t * observation_model;				/* H_k */
	const Matrix_t * process_noise_covariance;		/* Q_k */
	const Matrix_t * observation_noise_covariance;	/* R_k */
											/* The observation is modified by the user before every time step. */
	const Matrix_t * observation;						/* z_k */
											/* This group of matrices are updated every time step by the filter. */
	const Matrix_t * predicted_state;					/* x-hat_k|k-1 */
	const Matrix_t * predicted_estimate_covariance;	/* P_k|k-1 */
	const Matrix_t * innovation;						/* y-tilde_k */
	const Matrix_t * innovation_covariance;			/* S_k */
	const Matrix_t * inverse_innovation_covariance;	/* S_k^-1 */
	const Matrix_t * optimal_gain;					/* K_k */
	const Matrix_t * state_estimate;					/* x-hat_k|k */
	const Matrix_t * estimate_covariance;			/* P_k|k */

	/* This group is used for meaningless intermediate calculations */
	const Matrix_t * vertical_scratch;
	const Matrix_t * small_square_scratch;
	const Matrix_t * big_square_scratch;
	
} KalmanFilter_t;

void FilterKalmanInit(void);
void FilterKalmanUpdate(double lat, double lon, uint32_t time);
void FilterKalmanGet(double *lat, double *lon);

#endif
#endif
