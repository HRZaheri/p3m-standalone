#ifndef ASSIGNMENT_H
#define ASSIGNMENT_H

__global__ void assign_charges(const float * const pos, const float * const q,
float *mesh, const int m_size, const int cao, const float pos_shift, const
			       float hi);
#endif
