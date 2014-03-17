#ifndef _PID_HDR_
#define _PID_HDR_

/*
 * This file is a component of the test harness and/or sample solution
 * to the DiskDriver exercise developed in February 2008 for use in
 * assessing the OS3 module in undergraduate Computing Science at the
 * University of Glasgow.
 */

/*
 * a Pid is used to distinguish between the different fake applications
 * it is implemented as an unsigned long, but is guaranteed not to exceed
 * MAX_PID
 */

typedef unsigned long Pid;
#define MAX_PID 10

#endif /* _PID_HDR_ */
