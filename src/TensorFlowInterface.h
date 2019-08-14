/*
Copyright © 2012 Clint Bellanger
Copyright © 2013-2014 Justin Jacobs

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

#ifndef TENSOR_FLOW_INTERFACE_H
#define TENSOR_FLOW_INTERFACE_H

#include "CommonIncludes.h"
#include "Utils.h"

#include <tensorflow/c/c_api.h>

// static const int TENSOR_IN_LENGTH = 108;
// static const int TENSOR_OUT_LENGTH = 2;
static const int TENSOR_IN_LENGTH = 113;
static const int TENSOR_OUT_LENGTH = 1;

static const int NUM_DIMS = 2;

class TensorFlowInterface {
private:
	const char* saved_model_path;

	TF_Status* status;
	TF_Graph* graph;
	TF_Session* session;

	TF_Tensor* tensor_in;
	TF_Tensor * tensor_out;

	static void tensor_free_none(void * data, size_t len, void* arg);
protected:
public:
	TensorFlowInterface();
	~TensorFlowInterface();

	float * predict(std::array<float, TENSOR_IN_LENGTH> game_data);
};

#endif // TENSOR_FLOW_INTERFACE_H
