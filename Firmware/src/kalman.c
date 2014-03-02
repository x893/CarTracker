/* Kalman filters. */
#include "hardware.h"
#include "kalman.h"


#if ( USE_AVERAGE == 1 )

typedef struct {
	double lat;
	double lon;
} AveragePos_t;

AveragePos_t AverageData[MAX_AVERAGE_COUNT];

void FilterAverageInit(GpsContext_t * gc)
{
	gc->AverageIndex = 0;
	gc->AverageCount = 0;
}

/*******************************************************************************
*
* Function Name	:	FilterAverageUpdate
*
* Description	:	Add position to array
*
*******************************************************************************/
void FilterAverageUpdate(GpsContext_t * gc, double lat, double lon, uint32_t time)
{
	AveragePos_t *data = &AverageData[gc->AverageIndex];

	if (time == 0 || gc->AverageTime == 0 || (gc->AverageTime + (10) < time))
	{
		gc->AverageIndex = 0;
		gc->AverageCount = 0;
	}

	if (gc->AverageCount < MAX_AVERAGE_COUNT)
		gc->AverageCount++;

	if (++(gc->AverageIndex) >= MAX_AVERAGE_COUNT)
		gc->AverageIndex = 0;
	data->lat = lat;
	data->lon = lon;
	gc->AverageTime = time;
}

uint8_t FilterAverageGet(GpsContext_t * gc, double *lat, double *lon)
{
	if (gc->AverageCount > 0)
	{
		uint8_t idx = gc->AverageIndex;
		uint8_t cnt = gc->AverageCount;
		double avg_lat = 0.0, avg_lon = 0.0;
		double n_sum;

		if (cnt > gc->AverageMaxCount)
			cnt = gc->AverageMaxCount;
		n_sum = (double)cnt;

		while (cnt != 0)
		{
			if (idx == 0)
				idx = MAX_AVERAGE_COUNT;
			--idx;

			avg_lat += AverageData[idx].lat;
			avg_lon += AverageData[idx].lon;

			--cnt;
		}
		if (n_sum == 1.0)
		{
			*lat = avg_lat;
			*lon = avg_lon;
		}
		else
		{
			*lat = avg_lat / n_sum;
			*lon = avg_lon / n_sum;
		}
		return 1;
	}
	else
	{
		gc->AverageTime = 0;
		*lat = 0.0;
		*lon = 0.0;
	}
	return 0;
}

#endif


/* Refer to http://en.wikipedia.org/wiki/Kalman_filter for
   mathematical details. The naming scheme is that variables get names
   that make sense, and are commented with their analog in
   the Wikipedia mathematical notation.
   This Kalman filter implementation does not support controlled input.
   (Like knowing which way the steering wheel in a car is turned and
   using that to inform the model.)
   Vectors are handled as n-by-1 matrices.
   TODO: comment on the dimension of the matrices
*/

/* Runs one timestep of prediction + estimation.

   Before each time step of running this, set f.observation to be the
   next time step's observation.

   Before the first step, define the model by setting:
   f.state_transition
   f.observation_model
   f.process_noise_covariance
   f.observation_noise_covariance

   It is also advisable to initialize with reasonable guesses for
   f.state_estimate
   f.estimate_covariance
*/

#if ( USE_KALMAN == 1 )

/*
#define CONCATDOT(a,b)	a##.##b
#define CONCAT2(a,b)	a##b
#define CONCAT3(a,b,c)	CONCAT2(a##b,c)

#define KALMAN_M_DEFINE(name,rows,cols)		\
	Matrix_t   CONCAT2(kf_,name);			\
	double   CONCAT3(mc_,name,[rows*cols]);	\
	double * CONCAT3(mr_,name,[rows])

#define KALMAN_M_INIT(name,rows,cols)		\
	CONCATDOT(kf,name) = init_matrix(		\
					CONCAT2(&kf_,name),		\
					rows,					\
					cols,					\
					CONCAT3(&mc_,name,[0]),	\
					CONCAT3(&mr_,name,[0])	\
					)
*/

// KALMAN_M_DEFINE(state_transition, KALMAN_STATE, KALMAN_STATE);
double   mc_state_transition [KALMAN_STATE * KALMAN_STATE];
double * mr_state_transition [KALMAN_STATE];
const Matrix_t   kf_state_transition = {
	mr_state_transition,
	KALMAN_STATE,
	KALMAN_STATE
};

// KALMAN_M_DEFINE(observation_model, KALMAN_OBSERVATION, KALMAN_STATE);
double   mc_observation_model [KALMAN_OBSERVATION * KALMAN_STATE];
double * mr_observation_model [KALMAN_OBSERVATION];
const Matrix_t   kf_observation_model = {
	mr_observation_model,
	KALMAN_OBSERVATION,
	KALMAN_STATE
};

// KALMAN_M_DEFINE(process_noise_covariance, KALMAN_STATE, KALMAN_STATE);
double   mc_process_noise_covariance [KALMAN_STATE * KALMAN_STATE];
double * mr_process_noise_covariance [KALMAN_STATE];
const Matrix_t   kf_process_noise_covariance = {
	mr_process_noise_covariance,
	KALMAN_STATE,
	KALMAN_STATE
};

// KALMAN_M_DEFINE(observation, KALMAN_OBSERVATION, 1);
double   mc_observation [KALMAN_OBSERVATION * 1];
double * mr_observation [KALMAN_OBSERVATION];
const Matrix_t   kf_observation = {
	mr_observation,
	KALMAN_OBSERVATION,
	1
};

// KALMAN_M_DEFINE(predicted_state, KALMAN_STATE, 1);
double   mc_predicted_state [KALMAN_STATE * 1];
double * mr_predicted_state [KALMAN_STATE];
const Matrix_t   kf_predicted_state = {
	mr_predicted_state,
	KALMAN_STATE,
	1
};

// KALMAN_M_DEFINE(innovation, KALMAN_OBSERVATION,	1);
double   mc_innovation [KALMAN_OBSERVATION * 1];
double * mr_innovation [KALMAN_OBSERVATION];
const Matrix_t   kf_innovation = {
	mr_innovation,
	KALMAN_OBSERVATION,
	1
};

// KALMAN_M_DEFINE(innovation_covariance, KALMAN_OBSERVATION, KALMAN_OBSERVATION);
double   mc_innovation_covariance [KALMAN_OBSERVATION * KALMAN_OBSERVATION];
double * mr_innovation_covariance [KALMAN_OBSERVATION];
const Matrix_t   kf_innovation_covariance = {
	mr_innovation_covariance,
	KALMAN_OBSERVATION,
	KALMAN_OBSERVATION
};

// KALMAN_M_DEFINE(optimal_gain, KALMAN_STATE, KALMAN_OBSERVATION);
double   mc_optimal_gain [KALMAN_STATE * KALMAN_OBSERVATION];
double * mr_optimal_gain [KALMAN_STATE];
const Matrix_t   kf_optimal_gain = {
	mr_optimal_gain,
	KALMAN_STATE,
	KALMAN_OBSERVATION
};

// KALMAN_M_DEFINE(state_estimate, KALMAN_STATE, 1);
double   mc_state_estimate [KALMAN_STATE * 1];
double * mr_state_estimate [KALMAN_STATE];
const Matrix_t   kf_state_estimate = {
	mr_state_estimate,
	KALMAN_STATE,
	1
};

// KALMAN_M_DEFINE(estimate_covariance, KALMAN_STATE, KALMAN_STATE);
double   mc_estimate_covariance [KALMAN_STATE * KALMAN_STATE];
double * mr_estimate_covariance [KALMAN_STATE];
const Matrix_t   kf_estimate_covariance = {
	mr_estimate_covariance,
	KALMAN_STATE,
	KALMAN_STATE
};

// KALMAN_M_DEFINE(vertical_scratch,			KALMAN_STATE, KALMAN_OBSERVATION);
double   mc_vertical_scratch [KALMAN_STATE * KALMAN_OBSERVATION];
double * mr_vertical_scratch [KALMAN_STATE];
const Matrix_t   kf_vertical_scratch = {
	mr_vertical_scratch,
	KALMAN_STATE,
	KALMAN_OBSERVATION
};

// KALMAN_M_DEFINE(small_square_scratch,		KALMAN_OBSERVATION, KALMAN_OBSERVATION);
double   mc_small_square_scratch [KALMAN_OBSERVATION * KALMAN_OBSERVATION];
double * mr_small_square_scratch [KALMAN_OBSERVATION];
const Matrix_t   kf_small_square_scratch = {
	mr_small_square_scratch,
	KALMAN_OBSERVATION,
	KALMAN_OBSERVATION
};

// KALMAN_M_DEFINE(big_square_scratch,			KALMAN_STATE, KALMAN_STATE);
double   mc_big_square_scratch [KALMAN_STATE * KALMAN_STATE];
double * mr_big_square_scratch [KALMAN_STATE];
const Matrix_t   kf_big_square_scratch = {
	mr_big_square_scratch,
	KALMAN_STATE,
	KALMAN_STATE
};

// KALMAN_M_DEFINE(observation_noise_covariance,	KALMAN_OBSERVATION, KALMAN_OBSERVATION);
double   mc_observation_noise_covariance [KALMAN_OBSERVATION * KALMAN_OBSERVATION];
double * mr_observation_noise_covariance [KALMAN_OBSERVATION];
const Matrix_t   kf_observation_noise_covariance = {
	mr_observation_noise_covariance,
	KALMAN_OBSERVATION,
	KALMAN_OBSERVATION
};

// KALMAN_M_DEFINE(predicted_estimate_covariance,	KALMAN_STATE, KALMAN_STATE);
double   mc_predicted_estimate_covariance [KALMAN_STATE * KALMAN_STATE];
double * mr_predicted_estimate_covariance [KALMAN_STATE];
const Matrix_t   kf_predicted_estimate_covariance = {
	mr_predicted_estimate_covariance,
	KALMAN_STATE,
	KALMAN_STATE
};

// KALMAN_M_DEFINE(inverse_innovation_covariance,	KALMAN_OBSERVATION, KALMAN_OBSERVATION);
double   mc_inverse_innovation_covariance [KALMAN_OBSERVATION * KALMAN_OBSERVATION];
double * mr_inverse_innovation_covariance [KALMAN_OBSERVATION];
const Matrix_t   kf_inverse_innovation_covariance = {
	mr_inverse_innovation_covariance,
	KALMAN_OBSERVATION,
	KALMAN_OBSERVATION
};


const KalmanFilter_t kf = {
	KALMAN_STATE,
	KALMAN_OBSERVATION,
	&kf_state_transition,
	&kf_observation_model,
	&kf_process_noise_covariance,
	&kf_observation_noise_covariance,
	&kf_observation,
	&kf_predicted_state,
	&kf_predicted_estimate_covariance,
	&kf_innovation,
	&kf_innovation_covariance,
	&kf_inverse_innovation_covariance,
	&kf_optimal_gain,
	&kf_state_estimate,
	&kf_estimate_covariance,
	&kf_vertical_scratch,
	&kf_small_square_scratch,
	&kf_big_square_scratch
};


/*******************************************************************************
* Function Name :	set_seconds_per_timestep
* Description   :	The position units are in thousandths of latitude and longitude.
*					The velocity units are in thousandths of position units per second.
*
*					So if there is one second per timestep, a velocity of 1 will change
*					the lat or long by 1 after a million timesteps.
*
*					Thus a typical position is hundreds of thousands of units.
*					A typical velocity is maybe ten.
*
*******************************************************************************/
__STATIC_INLINE void set_seconds_per_timestep(double seconds_per_timestep)
{
	/*
		unit_scaler accounts for the relation between position and
		velocity units
	*/
	kf_state_transition.data[0][2] = 0.001 * seconds_per_timestep;
	kf_state_transition.data[1][3] = 0.001 * seconds_per_timestep;
}

/*******************************************************************************
* Function Name :	FilterKalmanInit
* Description   :	Initialize Kalman filter
*******************************************************************************/

#define KALMAN_NOISE	1.0

const double i_process_noise_covariance[] = {
	0.000001,	0.0,		0.0, 0.0,
	0.0,		0.000001,	0.0, 0.0,
	0.0,		0.0,		1.0, 0.0,
	0.0,		0.0,		0.0, 1.0
};
const double i_observation_noise_covariance[] = {
	0.000001 * KALMAN_NOISE,		0.0,
			0.0,			0.000001 * KALMAN_NOISE
};
const double i_observation_model[] = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0
};
const double i_state_estimate[] = {
	0.0, 0.0, 0.0, 0.0
};

void FilterKalmanInit(void)
{
	/*	The state model has four dimensions:
		x, y, x', y'
		Each time step we can only observe position, not velocity, so the
		observation vector has only two dimensions.
	*/

	init_matrix(kf.state_transition, mc_state_transition);
	init_matrix(kf.observation_model, mc_observation_model);
	init_matrix (kf.process_noise_covariance, mc_process_noise_covariance);
	init_matrix(kf.observation_noise_covariance, mc_observation_noise_covariance);
	init_matrix(kf.observation, mc_observation);
	init_matrix(kf.predicted_state, mc_predicted_state);
	init_matrix(kf.predicted_estimate_covariance, mc_predicted_estimate_covariance);
	init_matrix(kf.innovation, mc_innovation);
	init_matrix(kf.innovation_covariance, mc_innovation_covariance);
	init_matrix(kf.inverse_innovation_covariance, mc_inverse_innovation_covariance);
	init_matrix(kf.optimal_gain, mc_optimal_gain);
	init_matrix(kf.state_estimate, mc_state_estimate);
	init_matrix(kf.estimate_covariance, mc_estimate_covariance);
	init_matrix(kf.vertical_scratch, mc_vertical_scratch);
	init_matrix(kf.small_square_scratch, mc_small_square_scratch);
	init_matrix(kf.big_square_scratch, mc_big_square_scratch);

	/*	Assuming the axes are rectilinear does not work well at the
		poles, but it has the bonus that we don't need to convert between
		lat/long and more rectangular coordinates. The slight inaccuracy
		of our physics model is not too important.
	*/
	set_identity_matrix(kf.state_transition);
	set_seconds_per_timestep(1.0);

	/* We observe (x, y) in each time step */
	set_matrix_const(kf.observation_model, i_observation_model);

	/* Noise in the world. */
	set_matrix_const(kf.process_noise_covariance, i_process_noise_covariance);

	/* Noise in our observation */
	set_matrix_const( kf.observation_noise_covariance, i_observation_noise_covariance);

	/* The start position is totally unknown, so give a high variance */
	set_matrix_const(kf.state_estimate, i_state_estimate);
	set_identity_matrix(kf.estimate_covariance);

	scale_matrix(kf.estimate_covariance, (1000.0 * 1000.0 * 1000.0 * 1000.0));
}

/*******************************************************************************
* Function Name :	FilterKalmanUpdate
* Description   :	
*******************************************************************************/
void FilterKalmanUpdate(double lat, double lon, uint32_t time)
{
	set_seconds_per_timestep((double)time);
	set_matrix(kf.observation, lat * 1000.0, lon * 1000.0);

	// kf.timestep++;

	/* Predict the state */
	multiply_matrix(
		kf.state_transition,
		kf.state_estimate,
		kf.predicted_state
	);

	/* Predict the state estimate covariance */
	multiply_matrix(
		kf.state_transition,
		kf.estimate_covariance,
		kf.big_square_scratch
	);
	multiply_by_transpose_matrix(
		kf.big_square_scratch,
		kf.state_transition,
		kf.predicted_estimate_covariance
	);
	add_matrix(
		kf.predicted_estimate_covariance,
		kf.process_noise_covariance,
		kf.predicted_estimate_covariance
	);

	// Estimate
	/* Calculate innovation */
	multiply_matrix(
		kf.observation_model,
		kf.predicted_state,
		kf.innovation
	);
	subtract_matrix(
		kf.observation,
		kf.innovation,
		kf.innovation
	);

	/* Calculate innovation covariance */
	multiply_by_transpose_matrix(
		kf.predicted_estimate_covariance,
		kf.observation_model,
		kf.vertical_scratch
	);
	multiply_matrix(
		kf.observation_model,
		kf.vertical_scratch,
		kf.innovation_covariance
	);
	add_matrix(
		kf.innovation_covariance,
		kf.observation_noise_covariance,
		kf.innovation_covariance
	);

	/*	Invert the innovation covariance.
		Note: this destroys the innovation covariance.
		TODO: handle inversion failure intelligently.
	*/
	destructive_invert_matrix(
		kf.innovation_covariance,
		kf.inverse_innovation_covariance
	);
  
	/*	Calculate the optimal Kalman gain.
		Note we still have a useful partial product in vertical scratch
		from the innovation covariance.
	*/
	multiply_matrix(
		kf.vertical_scratch,
		kf.inverse_innovation_covariance,
		kf.optimal_gain
	);

	/* Estimate the state */
	multiply_matrix(
		kf.optimal_gain,
		kf.innovation,
		kf.state_estimate
	);
	add_matrix(
		kf.state_estimate,
		kf.predicted_state,
		kf.state_estimate
	);

	/* Estimate the state covariance */
	multiply_matrix(
		kf.optimal_gain,
		kf.observation_model,
		kf.big_square_scratch
	);
	subtract_from_identity_matrix(
		kf.big_square_scratch
	);
	multiply_matrix(
		kf.big_square_scratch,
		kf.predicted_estimate_covariance,
		kf.estimate_covariance
	);
}

/*******************************************************************************
* Function Name :	FilterKalmanGet
* Description   :	
*******************************************************************************/
void FilterKalmanGet(double *lat, double *lon)
{
	*lat = kf.state_estimate->data[0][0] / 1000.0;
	*lon = kf.state_estimate->data[1][0] / 1000.0;
}

/*
static const double PI = 3.14159265;
static const double EARTH_RADIUS_IN_MILES = 3963.1676;

void get_velocity(double* delta_lat, double* delta_lon)
{
	*delta_lat = kf.state_estimate->data[2][0] / (1000.0 * 1000.0);
	*delta_lon = kf.state_estimate->data[3][0] / (1000.0 * 1000.0);
}
*/

/*	See
	http://www.movable-type.co.uk/scripts/latlong.html
	for formulas
*/
/*
double get_bearing(KalmanFilter* f)
{
	double lat, lon, delta_lat, delta_lon, x, y, to_radians, lat1, bearing;

	FilterKalmanGet(f, &lat, &lon);
	get_velocity(f, &delta_lat, &delta_lon);

	// Convert to radians
	to_radians = PI / 180.0;
	lat *= to_radians;
	lon *= to_radians;
	delta_lat *= to_radians;
	delta_lon *= to_radians;
  
	// Do math
	lat1 = lat - delta_lat;
	y = sin(delta_lon) * cos(lat);
	x = cos(lat1) * sin(lat) - sin(lat1) * cos(lat) * cos(delta_lon);
	bearing = atan2(y, x);

	// Convert to degrees
	bearing = bearing / to_radians;

	while (bearing >= 360.0)
		bearing -= 360.0;

	while (bearing < 0.0)
		bearing += 360.0;

	return bearing;
}

double calculate_mph(double lat, double lon, double delta_lat, double delta_lon)
{
	//	First, let's calculate a unit-independent measurement - the radii
	//	of the earth traveled in each second.
	//	(Presumably this will be a very small number.)
  
	// Convert to radians
	double to_radians, lat1,sin_half_dlat, sin_half_dlon, a, radians_per_second, miles_per_second, miles_per_hour;
  
	to_radians = PI / 180.0;
	lat *= to_radians;
	lon *= to_radians;
	delta_lat *= to_radians;
	delta_lon *= to_radians;

	// Haversine formula
	lat1 = lat - delta_lat;
	sin_half_dlat = sin(delta_lat / 2.0);
	sin_half_dlon = sin(delta_lon / 2.0);
	a = sin_half_dlat * sin_half_dlat + cos(lat1) * cos(lat) * sin_half_dlon * sin_half_dlon;
	radians_per_second = 2 * atan2(1000.0 * sqrt(a), 1000.0 * sqrt(1.0 - a));
  
	// Convert units
	miles_per_second = radians_per_second * EARTH_RADIUS_IN_MILES;
	miles_per_hour = miles_per_second * 60.0 * 60.0;
	return miles_per_hour;
}

double get_mph(KalmanFilter* f)
{
	double lat, lon, delta_lat, delta_lon;
	FilterKalmanGet(f, &lat, &lon);
	get_velocity(f, &delta_lat, &delta_lon);
	return calculate_mph(lat, lon, delta_lat, delta_lon);
}
*/
#endif
