//
// Created by mar on 17.02.17.
//
#include "test-adjacency-matrix.h"
#include <gflags/gflags.h>
#include <gtest/gtest.h>


using test_adjacency_matrix::MatrixAndExpValue;
namespace test_adjacency_matrix
{

	static std::vector<MatrixAndExpValue> cases;
	void init_cases()
	{
		std::vector<std::string> temp = {"a", "b", "c", "d", "e"};
		cases.emplace_back(temp, true);
		cases[0].test_matrix("a", "a") = 10;
		cases[0].test_matrix("a", "e") = 50;
		cases[0].test_matrix("b", "b") = 20;
		cases[0].test_matrix("c", "c") = 30;
		cases[0].test_matrix("d", "d") = 40;
		cases[0].test_matrix("e", "e") = 50;
	}
}

int main(int argc, char **argv)
{

	test_adjacency_matrix::init_cases();


	::testing::InitGoogleTest(&argc, argv);

//	if (::testing::FLAGS_gtest_filter == "*" && !::testing::FLAGS_gtest_list_tests)
//	{
//		std::cout << "Usage:\n"
//				"--gtest_filter=*UT to run unit tests\n"
//				"--gtest_filter=*HT to run hardware tests\n"
//				"--gtest_filter=*LT to run stress tests\n\n";
//
//		::testing::FLAGS_gtest_filter = "";
//	}

	google::ParseCommandLineFlags(&argc, &argv, true);

	return RUN_ALL_TESTS();
//	std::vector<std::string> b = {"a", "b", "c", "d", "e"};
//	AdjacencyMatrix matrix;
//	matrix.init(b);
//	matrix.cout_debug_print();
//	matrix("a", "a") = 10;
//	matrix("a", "e") = 50;
//	matrix("b", "b") = 20;
//	matrix("c", "c") = 30;
//	matrix("d", "d") = 40;
//	matrix("e", "e") = 50;
//
//	std::cout << (++matrix("a", "a")) << std::endl;
//	std::string result = matrix.result_to_string(false);
//	std::cout << result << std::endl;
//
////	matrix.swap_columns(0, 4);
//	matrix.sort();
//	result = matrix.result_to_string(false);
//	std::cout << "sort:" <<"\n" << result << std::endl;
//
////	matrix.sort_rows();
//////	matrix.swap_columns(0, 5);  // Проверка ассерта
////	result = matrix.result_to_string();
////	std::cout << result << std::endl;
////	matrix("a", "b") = 10;
////	matrix.sort_columns();
//////	matrix.swap_columns(0, 5);  // Проверка ассерта
////	result = matrix.result_to_string();
////	std::cout << result << std::endl;
////	matrix.cout_debug_print();
//	std::cout << matrix.find_y_index("b") << std::endl;
//	std::cout << matrix.find_x_index("b") << std::endl;
//
//
//	std::cout << matrix.get_col_as_string("e") << std::endl;
//	std::cout << matrix.get_row_as_string("b") << std::endl;
//
////	std::cout << matrix.find_x_index("f") << std::endl; // -1, нет такой вершины
//
//	AdjacencyMatrix matrix2;
//	matrix2.init(b);
//	matrix2("a", "a") = 10;
//	matrix2("a", "e") = 50;
//	matrix2("b", "b") = 20;
//	matrix2("c", "c") = 30;
//	matrix2("d", "d") = 40;
//	matrix2("e", "e") = 50;
//	matrix2.cout_debug_print();
//	matrix.cout_debug_print();
//	matrix += matrix2;
//	matrix.cout_debug_print();
//	matrix2.cout_debug_print();
//	std::cout << (matrix==matrix2) << std::endl;
//
//	return 0;
}

class MatrixAdjacencyTest : public testing::TestWithParam<int>
{
public:
	/**
	 * @brief Run all test cases
	 * @param cases [in]

	 */
	void run_case(int case_idx)
	{
		MatrixAndExpValue &test_case = test_adjacency_matrix::cases[case_idx];
		std::vector<std::string> b = {"a", "b", "c", "d", "e"};
		AdjacencyMatrix matrix(b);
//		matrix.cout_debug_print();
		matrix("a", "a") = 10;
		matrix("a", "e") = 50;
		matrix("b", "b") = 20;
		matrix("c", "c") = 30;
		matrix("d", "d") = 40;
		matrix("e", "e") = 50;
		++matrix("a", "a");
		++test_case.test_matrix("a", "a");
		AdjacencyMatrix matrix2;

		matrix2 += matrix;
		std::string result_without_norm = "\ta\tb\tc\td\te\t\n"
				"a\t11.00\t0.00\t0.00\t0.00\t50.00\t\n"
				"b\t0.00\t20.00\t0.00\t0.00\t0.00\t\n"
				"c\t0.00\t0.00\t30.00\t0.00\t0.00\t\n"
				"d\t0.00\t0.00\t0.00\t40.00\t0.00\t\n"
				"e\t0.00\t0.00\t0.00\t0.00\t50.00\t\n";
		std::string result_norm = "\ta\tb\tc\td\te\t\n"
				"a\t18.03\t0.00\t0.00\t0.00\t81.97\t\n"
				"b\t0.00\t100.00\t0.00\t0.00\t0.00\t\n"
				"c\t0.00\t0.00\t100.00\t0.00\t0.00\t\n"
				"d\t0.00\t0.00\t0.00\t100.00\t0.00\t\n"
				"e\t0.00\t0.00\t0.00\t0.00\t100.00\t\n";

		EXPECT_TRUE(result_without_norm == test_case.test_matrix.result_to_string(false));

		EXPECT_TRUE(result_norm == test_case.test_matrix.result_to_string());
		EXPECT_FALSE(test_case.test_matrix.result_to_string(false) ==
					 test_case.test_matrix.result_to_string());
		EXPECT_TRUE(matrix2 == test_case.test_matrix);
		EXPECT_TRUE(matrix == test_case.test_matrix);
		EXPECT_TRUE(test_case.test_matrix.find_y_index("b") != -1);
		EXPECT_TRUE(test_case.test_matrix.find_x_index("b") != -1);
		EXPECT_TRUE(test_case.test_matrix.find_y_index("j") == -1);
		EXPECT_TRUE(test_case.test_matrix.find_x_index("j") == -1);

	}

};

TEST_P(MatrixAdjacencyTest, TestOfRegexEqForPatterns)
{
	run_case(GetParam());
}

INSTANTIATE_TEST_CASE_P(
		MatrixAdjacencyTest, MatrixAdjacencyTest,
		testing::Range(0, int(test_adjacency_matrix::cases.size()) ));
