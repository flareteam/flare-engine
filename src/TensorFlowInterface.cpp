/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2012-2016 Justin Jacobs

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

#include <tensorflow/c/c_api.h>
#include "TensorFlowInterface.h"
#include "CommonIncludes.h"
#include "Utils.h"

TensorFlowInterface::TensorFlowInterface()
	: saved_model_path("/Users/lpekelis/flare/flare-ai/models/median_time_to_damage_20180611_003221_saved_model")
{
	logInfo("Hello from TensorFlow C library version %s\n", TF_Version());
	TF_SessionOptions* opts = TF_NewSessionOptions();
	status = TF_NewStatus();
	const char* tags[] = {"serve"};  // tf.saved_model.tag_constants.SERVING
	graph = TF_NewGraph();
	session = TF_LoadSessionFromSavedModel(opts, NULL, saved_model_path, tags, 1, graph, NULL, status);
	TF_DeleteSessionOptions(opts);

	// allocate memory to tensors to avoid mem errors
	int64_t dims_in[NUM_DIMS] = {1, TENSOR_IN_LENGTH};
	int64_t dims_out[NUM_DIMS] = {1, TENSOR_OUT_LENGTH};
	tensor_in = TF_AllocateTensor(TF_FLOAT, dims_in, NUM_DIMS, sizeof(float) * TENSOR_IN_LENGTH);
	tensor_out = TF_AllocateTensor(TF_FLOAT, dims_out, NUM_DIMS, sizeof(float) * TENSOR_OUT_LENGTH);

}

TensorFlowInterface::~TensorFlowInterface() {
	TF_DeleteTensor(tensor_in);
	TF_DeleteTensor(tensor_out);
	TF_CloseSession(session, status);
	TF_DeleteSession(session, status);
	TF_DeleteGraph(graph);
	TF_DeleteStatus(status);
}

// Using stack input data nothing to free
void TensorFlowInterface::tensor_free_none(void * data, size_t len, void* arg) {
}

float * TensorFlowInterface::predict(std::array<float, TENSOR_IN_LENGTH> game_data) {

	for(int i=0;i<game_data.size();i++) {
		printf("game_data[%d]=%f, ", i, game_data[i]);
	}

	// int64_t input_num_values = current_game_data.size();
	// const int num_dims = 2;
	// int64_t dims[num_dims] = {1, input_num_values};
	// int64_t byteCount = sizeof(float) * input_num_values;
	//
	// int64_t output_num_values = 2;
	// int64_t out_dims[num_dims] = {1, output_num_values};
	//
	//
	// printf("Input data length: %d\n", input_num_values);
	//
	// // TF_Tensor* tensor_in = TF_AllocateTensor(TF_FLOAT, dims, num_dims, sizeof(float) * input_num_values);
  // // memcpy(TF_TensorData(tensor_in), game_data, sizeof(float) * input_num_values);
	// TF_Tensor* tensor_in = TF_NewTensor(TF_FLOAT, dims, num_dims, &current_game_data[0], byteCount, tensor_free_none, NULL);
	// printf("Input tensor allocated.\n");
	// //TF_Tensor * tensor_out = TF_NewTensor(TF_FLOAT, out_dims, num_dims, &out_data, sizeof(float) * 2, tensor_free_none, NULL);
	// TF_Tensor * tensor_out = TF_AllocateTensor(TF_FLOAT, out_dims, num_dims, sizeof(float) * output_num_values);
	// printf("Output tensor allocated.\n");

	float* tensor_in_ptr = (float *)TF_TensorData(tensor_in);

	logInfo("TensorFlowInterface: Prediction, input vector is length %d, with size of %d, sizeof float %d",
					game_data.size(), sizeof(game_data[0]), sizeof(float));

	//std::memcpy(tensor_in_ptr, &game_data[0], sizeof(float)*TENSOR_IN_LENGTH);
	std::copy(game_data.begin(), game_data.end(), tensor_in_ptr);
  printf("Input tensor filled.\n");

	// Operations
	TF_Operation * op_in = TF_GraphOperationByName(graph, "InputData/X");
	TF_Operation * op_out = TF_GraphOperationByName(graph, "FullyConnected_4/Softmax");
	printf("Operations set.\n");

	// Session Inputs
	TF_Output input_operations[] = { op_in, 0 };
	TF_Tensor ** input_tensors = {&tensor_in};
	printf("Session inputs.\n");

	// Session Outputs
	TF_Output output_operations[] = { op_out, 0 };
	TF_Tensor ** output_tensors = {&tensor_out};
	printf("Session outputs.\n");

	printf("Starting session run.\n");

	TF_SessionRun(session, NULL,
			// Inputs
			input_operations, input_tensors, 1,
			// Outputs
			output_operations, output_tensors, 1,
			// Target operations
			NULL, 0, NULL,
			status);

	printf("Session Run Status: %d - %s\n", TF_GetCode(status), TF_Message(status) );
	printf("Output Tensor Type: %d\n", TF_TensorType(tensor_out));
	float* outval = (float *)TF_TensorData(tensor_out);
	printf("Output Tensor Value: %.2f\n", (*outval));

	// de-allocate
	TF_DeleteTensor(tensor_in);
	TF_DeleteTensor(tensor_out);

	//return *(outval + 0);
	return outval;
}
