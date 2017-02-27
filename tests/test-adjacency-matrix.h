//
// Created by mar on 22.02.17.
//

#ifndef TEST_ADJACENCY_MATRIX_H
#define TEST_ADJACENCY_MATRIX_H

#include "common/adjacency-matrix.h"
#include <string>


namespace test_adjacency_matrix
{

	struct MatrixAndExpValue
	{
		MatrixAndExpValue(const std::vector<std::string> vec, bool _exp_value):
				test_matrix(vec),
				expected_value(_exp_value){};
		AdjacencyMatrix test_matrix;
		bool expected_value;
	};
	struct TestAdjacencyMatrix
	{

	};


	void init_cases();

}

#endif //AIFIL_UTILS_TEST_ADJACENCY_MATRIX_H
