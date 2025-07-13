#ifndef LIBMOSQUITTO_COMMON_TIME_H
#define LIBMOSQUITTO_COMMON_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

/* Function: mosquitto_time_init
 *
 * Initialises the time source to use the best source available at run time.
 */
libmosqcommon_EXPORT void mosquitto_time_init(void);

/* Function: mosquitto_time
 *
 * Returns an indication of the current time in seconds. The exact type of
 * value varies depending on the platform in use, but in most cases will be a
 * monotonically increasing value that does not relate to the real clock time.
 *
 * Returns:
 *    Indication of the current time, in seconds
 */
libmosqcommon_EXPORT time_t mosquitto_time(void);

/* Function: mosquitto_time_ns
 *
 * Returns the current clock time in seconds and nanoseconds. The resolution of
 * the nanosecond value varies depending on the platform in use.
 *
 * The value returned may be decrease as well as increase in response to system
 * clock changes.
 *
 * Parameters:
 *    s - the output pointer for the number of seconds
 *    ns - the output pointer for the number of nanoseconds
 */
libmosqcommon_EXPORT void mosquitto_time_ns(time_t *s, long *ns);

/* Function: mosquitto_time_cmp
 *
 * Returns < 0 if the time t1 is smaller (earlier) than t2
 * Returns > 0 if the time t1 is greater (later) than t2
 * Returns == 0 if the time t1 is exactly equal to t2
 */
libmosqcommon_EXPORT long mosquitto_time_cmp(time_t t1_s, long t1_ns, time_t t2_s, long t2_ns);

#ifdef __cplusplus
}
#endif

#endif
