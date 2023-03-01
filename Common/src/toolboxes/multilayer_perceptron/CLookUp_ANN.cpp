/*!
 * \file CLookUp_ANN.cpp
 * \brief Implementation of the multi-layer perceptron class to be
 *      used for look-up operations.
 * \author E.C.Bunschoten
 * \version 7.5.1 "Blackbird"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2023, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */
#include "../../../include/toolboxes/multilayer_perceptron/CLookUp_ANN.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "../../../include/toolboxes/multilayer_perceptron/CIOMap.hpp"
#include "../../../include/toolboxes/multilayer_perceptron/CReadNeuralNetwork.hpp"

using namespace std;

MLPToolbox::CLookUp_ANN::CLookUp_ANN(const unsigned short n_inputs, const string* input_filenames) {
#ifdef HAVE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
  if (rank == MASTER_NODE) cout << "Generating ANN collection" << endl;

  number_of_variables = n_inputs;

  NeuralNetworks.resize(n_inputs);
  
  if (rank == MASTER_NODE) {
    cout << setfill(' ');
    cout << endl;
    cout << "+------------------------------------------------------------------+\n";
    cout << "|                 Multi-Layer Perceptron (MLP) info                |\n";
    cout << "+------------------------------------------------------------------+" << endl;
  }

  for (auto i_MLP = 0u; i_MLP < n_inputs; i_MLP++) {
    if (rank == MASTER_NODE) cout << "Generating neural network for " << input_filenames[i_MLP] << endl;
    GenerateANN(NeuralNetworks[i_MLP], input_filenames[i_MLP]);
    if (rank == MASTER_NODE) NeuralNetworks[i_MLP].DisplayNetwork();
  }
}

vector<pair<size_t, size_t>> MLPToolbox::CLookUp_ANN::FindVariableIndices(size_t i_ANN,
                                                                          su2vector<string> variable_names,
                                                                          bool input) const {
  /*--- Find loaded MLPs that have the same input variable names as the variables listed in variable_names ---*/

  vector<pair<size_t, size_t>> variable_indices;
  auto nVar = input ? NeuralNetworks[i_ANN].GetnInputs() : NeuralNetworks[i_ANN].GetnOutputs();

  for (auto iVar = 0u; iVar < nVar; iVar++) {
    for (auto jVar = 0u; jVar < variable_names.size(); jVar++) {
      string ANN_varname = input ? NeuralNetworks[i_ANN].GetInputName(iVar) : NeuralNetworks[i_ANN].GetOutputName(iVar);
      if (variable_names[jVar].compare(ANN_varname) == 0) {
        variable_indices.push_back(make_pair(jVar, iVar));
      }
    }
  }
  return variable_indices;
}

unsigned long MLPToolbox::CLookUp_ANN::PredictANN(CIOMap* input_output_map, su2vector<su2double>& inputs,
                                                  su2vector<su2double*>& outputs) {
  /*--- Evaluate MLP based on target input and output variables ---*/
  bool within_range,              // Within MLP training set range.
      MLP_was_evaluated = false;  // MLP was evaluated within training set range.

  /* If queries lie outside the training data set, the nearest MLP will be evaluated through extrapolation. */
  su2double distance_to_query = 1e20;  // Overall smallest distance between training data set middle and query.
  size_t i_ANN_nearest = 0,            // Index of nearest MLP.
      i_map_nearest = 0;               // Index of nearest iomap index.

  for (auto i_map = 0u; i_map < input_output_map->GetNMLPs(); i_map++) {
    within_range = true;
    auto i_ANN = input_output_map->GetMLPIndex(i_map);
    auto ANN_inputs = input_output_map->GetMLPInputs(i_map, inputs);

    su2double distance_to_query_i = 0;
    for (auto i_input = 0u; i_input < ANN_inputs.size(); i_input++) {
      auto ANN_input_limits = NeuralNetworks[i_ANN].GetInputNorm(i_input);

      /* Check if query input lies outside MLP training range */
      if ((ANN_inputs[i_input] < ANN_input_limits.first) || (ANN_inputs[i_input] > ANN_input_limits.second)) {
        within_range = false;
      }

      /* Calculate distance between MLP training range center point and query */
      su2double middle = 0.5 * (ANN_input_limits.second + ANN_input_limits.first);
      distance_to_query_i +=
          pow((ANN_inputs[i_input] - middle) / (ANN_input_limits.second - ANN_input_limits.first), 2);
    }

    /* Evaluate MLP when query inputs lie within training data range */
    if (within_range) {
      NeuralNetworks[i_ANN].Predict(ANN_inputs);
      MLP_was_evaluated = true;
      for (auto i = 0u; i < input_output_map->GetNMappedOutputs(i_map); i++) {
        *outputs[input_output_map->GetOutputIndex(i_map, i)] =
            NeuralNetworks[i_ANN].GetANNOutput(input_output_map->GetMLPOutputIndex(i_map, i));
      }
    }

    /* Update minimum distance to query */
    if (sqrt(distance_to_query_i) < distance_to_query) {
      i_ANN_nearest = i_ANN;
      distance_to_query = distance_to_query_i;
      i_map_nearest = i_map;
    }
  }
  /* Evaluate nearest MLP in case no query data within range is found */
  if (!MLP_was_evaluated) {
    auto ANN_inputs = input_output_map->GetMLPInputs(i_map_nearest, inputs);
    NeuralNetworks[i_ANN_nearest].Predict(ANN_inputs);
    for (auto i = 0u; i < input_output_map->GetNMappedOutputs(i_map_nearest); i++) {
      *outputs[input_output_map->GetOutputIndex(i_map_nearest, i)] =
          NeuralNetworks[i_ANN_nearest].GetANNOutput(input_output_map->GetMLPOutputIndex(i_map_nearest, i));
    }
  }

  /* Return 1 if query data lies outside the range of any of the loaded MLPs */
  return MLP_was_evaluated ? 0 : 1;
}

void MLPToolbox::CLookUp_ANN::GenerateANN(CNeuralNetwork& ANN, string fileName) {
  /*--- Generate MLP architecture based on information in MLP input file ---*/

  /* Read MLP input file */
  CReadNeuralNetwork Reader = CReadNeuralNetwork(fileName);

  /* Read MLP input file */
  Reader.ReadMLPFile();

  /* Generate basic layer architectures */
  ANN.DefineInputLayer(Reader.GetNInputs());
  ANN.SizeInputs(Reader.GetNInputs());
  for (auto iInput = 0u; iInput < Reader.GetNInputs(); iInput++) {
    ANN.SetInputName(iInput, Reader.GetInputName(iInput));
  }
  for (auto iLayer = 1u; iLayer < Reader.GetNlayers() - 1; iLayer++) {
    ANN.PushHiddenLayer(Reader.GetNneurons(iLayer));
  }
  ANN.DefineOutputLayer(Reader.GetNOutputs());
  for (auto iOutput = 0u; iOutput < Reader.GetNOutputs(); iOutput++) {
    ANN.SetOutputName(iOutput, Reader.GetOutputName(iOutput));
  }

  /* Size weights of each layer */
  ANN.SizeWeights();

  /* Define weights and activation functions */
  ANN.SizeActivationFunctions(ANN.GetNWeightLayers() + 1);
  for (auto i_layer = 0u; i_layer < ANN.GetNWeightLayers(); i_layer++) {
    ANN.SetActivationFunction(i_layer, Reader.GetActivationFunction(i_layer));
    for (auto i_neuron = 0u; i_neuron < ANN.GetNNeurons(i_layer); i_neuron++) {
      for (auto j_neuron = 0u; j_neuron < ANN.GetNNeurons(i_layer + 1); j_neuron++) {
        ANN.SetWeight(i_layer, i_neuron, j_neuron, Reader.GetWeight(i_layer, i_neuron, j_neuron));
      }
    }
  }
  ANN.SetActivationFunction(ANN.GetNWeightLayers(), Reader.GetActivationFunction(ANN.GetNWeightLayers()));

  /* Set neuron biases */
  for (auto i_layer = 0u; i_layer < ANN.GetNWeightLayers() + 1; i_layer++) {
    for (auto i_neuron = 0u; i_neuron < ANN.GetNNeurons(i_layer); i_neuron++) {
      ANN.SetBias(i_layer, i_neuron, Reader.GetBias(i_layer, i_neuron));
    }
  }

  /* Define input and output layer normalization values */
  for (auto iInput = 0u; iInput < Reader.GetNInputs(); iInput++) {
    ANN.SetInputNorm(iInput, Reader.GetInputNorm(iInput).first, Reader.GetInputNorm(iInput).second);
  }
  for (auto iOutput = 0u; iOutput < Reader.GetNOutputs(); iOutput++) {
    ANN.SetOutputNorm(iOutput, Reader.GetOutputNorm(iOutput).first, Reader.GetOutputNorm(iOutput).second);
  }
}

bool MLPToolbox::CLookUp_ANN::CheckUseOfInputs(su2vector<string>& input_names, CIOMap* input_output_map) const {
  /*--- Check wether all input variables are in the loaded MLPs ---*/
  vector<string> missing_inputs;
  bool inputs_are_present{true};
  for (auto iInput = 0u; iInput < input_names.size(); iInput++) {
    bool found_input = false;
    for (auto i_map = 0u; i_map < input_output_map->GetNMLPs(); i_map++) {
      auto input_map = input_output_map->GetInputMapping(i_map);
      for (auto jInput = 0u; jInput < input_map.size(); jInput++) {
        if (input_map[jInput].first == iInput) {
          found_input = true;
        }
      }
    }
    if (!found_input) {
      missing_inputs.push_back(input_names[iInput]);
      inputs_are_present = false;
    };
  }
  if (missing_inputs.size() > 0) {
    string message{"Inputs "};
    for (size_t iVar = 0; iVar < missing_inputs.size(); iVar++) message += missing_inputs[iVar] + " ";
    SU2_MPI::Error(message + "are not present in any loaded ANN.", CURRENT_FUNCTION);
  }
  return inputs_are_present;
}

bool MLPToolbox::CLookUp_ANN::CheckUseOfOutputs(su2vector<string>& output_names, CIOMap* input_output_map) const {
  /*--- Check wether all output variables are in the loaded MLPs ---*/

  vector<string> missing_outputs;
  bool outputs_are_present{true};
  /* Looping over the target outputs */
  for (auto iOutput = 0u; iOutput < output_names.size(); iOutput++) {
    bool found_output{false};

    /* Looping over all the selected ANNs */
    for (auto i_map = 0u; i_map < input_output_map->GetNMLPs(); i_map++) {
      auto output_map = input_output_map->GetOutputMapping(i_map);

      /* Looping over the outputs of the output map of the current ANN */
      for (auto jOutput = 0u; jOutput < output_map.size(); jOutput++) {
        if (output_map[jOutput].first == iOutput) found_output = true;
      }
    }
    if (!found_output) {
      missing_outputs.push_back(output_names[iOutput]);
      outputs_are_present = false;
    };
  }
  if (missing_outputs.size() > 0) {
    string message{"Outputs "};
    for (size_t iVar = 0; iVar < missing_outputs.size(); iVar++) message += missing_outputs[iVar] + " ";
    SU2_MPI::Error(message + "are not present in any loaded ANN.", CURRENT_FUNCTION);
  }
  return outputs_are_present;
}