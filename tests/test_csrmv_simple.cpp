#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <boost/program_options.hpp>

#include "resources/clsparse_environment.h"
#include "resources/csr_matrix_environment.h"
#include "resources/matrix_utils.h"

cl_command_queue ClSparseEnvironment::queue = NULL;
cl_context ClSparseEnvironment::context = NULL;

namespace po = boost::program_options;


template <typename T>
class TestCSRMV : public ::testing::Test
{
    using CSRE = CSREnvironment;
    using CLSE = ClSparseEnvironment;

public:

    void SetUp()
    {
        //TODO:: take the values from cmdline;
        alpha = T(CSRE::alpha);
        beta = T(CSRE::beta);

        x = std::vector<T>(CSRE::n_cols);
        y = std::vector<T>(CSRE::n_rows);

        std::fill(x.begin(), x.end(), T(1));
        std::fill(x.begin(), x.end(), T(0));

        cl_int status;
        gx = clCreateBuffer(CLSE::context,
                            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                            x.size() * sizeof(T), x.data(), &status);

        ASSERT_EQ(CL_SUCCESS, status); //is it wise to use this here?

        gy = clCreateBuffer(CLSE::context,
                            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                            y.size() * sizeof(T), y.data(), &status);

        ASSERT_EQ(CL_SUCCESS, status);

        galpha = clCreateBuffer(CLSE::context,
                                CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                sizeof(T), &alpha, &status);

        ASSERT_EQ(CL_SUCCESS, status);

        gbeta = clCreateBuffer(CLSE::context,
                               CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                               sizeof(T), &beta, &status);

        ASSERT_EQ(CL_SUCCESS, status);

        generateReference(x, alpha, y, beta);


    }


private:
    void generateReference (const std::vector<float>& x,
                            const float alpha,
                            std::vector<float>& y,
                            const float beta)
    {
            csrmv(CSRE::n_rows, CSRE::n_cols, CSRE::n_vals,
                CSRE::row_offsets, CSRE::col_indices, CSRE::f_values,
                  x, alpha, y, beta);
    }

    void generateReference (const std::vector<double>& x,
                            const double alpha,
                            std::vector<double>& y,
                            const double beta)
    {
            csrmv(CSRE::n_rows, CSRE::n_cols, CSRE::n_vals,
                CSRE::row_offsets, CSRE::col_indices, CSRE::d_values,
                  x, alpha, y, beta);
    }

    cl_mem gx;
    cl_mem gy;
    std::vector<T> x;
    std::vector<T> y;

    T alpha;
    T beta;

    cl_mem galpha;
    cl_mem gbeta;


};

typedef ::testing::Types<float, double> TYPES;
TYPED_TEST_CASE(TestCSRMV, TYPES);

TYPED_TEST(TestCSRMV, multiply)
{

}


int main (int argc, char* argv[])
{
    using CLSE = ClSparseEnvironment;
    using CSRE = CSREnvironment;
    //pass path to matrix as an argument, We can switch to boost po later

    std::string path;
    double alpha;
    double beta;

    po::options_description desc("Allowed options");

    desc.add_options()
            ("help,h", "Produce this message.")
            ("path,p", po::value(&path)->required(), "Path to matrix in mtx format.")
            ("alpha,a", po::value(&alpha)->default_value(1.0),
             "Alpha parameter for eq: \n\ty = alpha * M * x + beta * y")
            ("beta,b", po::value(&beta)->default_value(0.0),
             "Beta parameter for eq: \n\ty = alpha * M * x + beta * y");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (po::error& error)
    {
        std::cerr << "Parsing command line options..." << std::endl;
        std::cerr << "Error: " << error.what() << std::endl;
        std::cerr << desc << std::endl;
        return false;
    }



    ::testing::InitGoogleTest(&argc, argv);
    //order does matter!
    ::testing::AddGlobalTestEnvironment( new CLSE());
    ::testing::AddGlobalTestEnvironment( new CSRE(path, alpha, beta,
                                                  CLSE::queue, CLSE::context));
    return RUN_ALL_TESTS();
}