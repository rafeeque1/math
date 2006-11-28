// test_negative_binomial.cpp

// Copyright Paul A. Bristow 2006.
// Copyright John Maddock 2006.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Tests for Negative Binomial Distribution.

#define BOOST_MATH_THROW_ON_DOMAIN_ERROR
// Note BOOST_MATH_THROW_ON_OVERFLOW_ERROR is NOT defined - see below.
// Note that there defines must be placed BEFORE #includes.
// Nor are these defined - several tests underflow by design.
//#define BOOST_MATH_THROW_ON_UNDERFLOW_ERROR
//#define BOOST_MATH_THROW_ON_DENORM_ERROR

#ifdef _MSC_VER
#  pragma warning(disable: 4127) // conditional expression is constant.
#  pragma warning(disable: 4100) // unreferenced formal parameter.
#  pragma warning(disable: 4512) // assignment operator could not be generated.
#endif

#include <boost/math/distributions/negative_binomial.hpp> // for negative_binomial_distribution
using boost::math::negative_binomial_distribution;

#include <boost/math/special_functions/gamma.hpp>
  using boost::math::lgamma;  // log gamma

#include <boost/math/concepts/real_concept.hpp> // for real_concept
using ::boost::math::concepts::real_concept;

#include <boost/test/included/test_exec_monitor.hpp> // for test_main
#include <boost/test/floating_point_comparison.hpp> // for BOOST_CHECK_CLOSE

#include <iostream>
using std::cout;
using std::endl;
using std::setprecision;
using std::showpoint;
#include <limits>
using std::numeric_limits;

// Need all functions working to use this test_spot!

template <class RealType>
void test_spot( // Test a single spot value against 'known good' values.
               RealType N,    // Number of successes.
               RealType k,    // Number of failures.
               RealType p,    // Probability of success_fraction.
               RealType P,    // CDF probability.
               RealType Q,    // Complement of CDF.
               RealType tol)  // Test tolerance.
{
  boost::math::negative_binomial_distribution<RealType> bn(N, p);
   BOOST_CHECK_EQUAL(N, bn.successes());
   BOOST_CHECK_EQUAL(p, bn.success_fraction());
   BOOST_CHECK_CLOSE(
     cdf(bn, k), P, tol);

  if((P < 0.99) && (Q < 0.99))
  {
    // We can only check this if P is not too close to 1,
    // so that we can guarantee that Q is free of error:
    //
    BOOST_CHECK_CLOSE(
      cdf(complement(bn, k)), Q, tol);
    if(k != 0)
    {
      BOOST_CHECK_CLOSE(
        quantile(bn, P), k, tol);
    }
    else
    {
      // Just check quantile is very small:
      if((std::numeric_limits<RealType>::max_exponent <= std::numeric_limits<double>::max_exponent)
        && (boost::is_floating_point<RealType>::value))
      {
        // Limit where this is checked: if exponent range is very large we may
        // run out of iterations in our root finding algorithm.
        BOOST_CHECK(quantile(bn, P) < boost::math::tools::epsilon<RealType>() * 10);
      }
    }
    if(k != 0)
    {
      BOOST_CHECK_CLOSE(
        quantile(complement(bn, Q)), k, tol);
    }
    else
    {
      // Just check quantile is very small:
      if((std::numeric_limits<RealType>::max_exponent <= std::numeric_limits<double>::max_exponent)
        && (boost::is_floating_point<RealType>::value))
      {
        // Limit where this is checked: if exponent range is very large we may
        // run out of iterations in our root finding algorithm.
        BOOST_CHECK(quantile(complement(bn, Q)) < boost::math::tools::epsilon<RealType>() * 10);
      }
    }
    // estimate success ratio:
    BOOST_CHECK_CLOSE(
      negative_binomial_distribution<RealType>::estimate_lower_bound_on_p(
      N+k, N, P),
      p, tol);
    // Note we bump up the sample size here, purely for the sake of the test,
    // internally the function has to adjust the sample size so that we get
    // the right upper bound, our test undoes this, so we can verify the result.
    BOOST_CHECK_CLOSE(
      negative_binomial_distribution<RealType>::estimate_upper_bound_on_p(
      N+k+1, N, Q),
      p, tol);

    if(Q < P)
    {
       //
       // We check two things here, that the upper and lower bounds
       // are the right way around, and that they do actually bracket
       // the naive estimate of p = successes / (sample size)
       //
      BOOST_CHECK(
        negative_binomial_distribution<RealType>::estimate_lower_bound_on_p(
        N+k, N, Q)
        <=
        negative_binomial_distribution<RealType>::estimate_upper_bound_on_p(
        N+k, N, Q)
        );
      BOOST_CHECK(
        negative_binomial_distribution<RealType>::estimate_lower_bound_on_p(
        N+k, N, Q)
        <=
        N / (N+k)
        );
      BOOST_CHECK(
        N / (N+k)
        <=
        negative_binomial_distribution<RealType>::estimate_upper_bound_on_p(
        N+k, N, Q)
        );
    }
    else
    {
       // As above but when P is small.
      BOOST_CHECK(
        negative_binomial_distribution<RealType>::estimate_lower_bound_on_p(
        N+k, N, P)
        <=
        negative_binomial_distribution<RealType>::estimate_upper_bound_on_p(
        N+k, N, P)
        );
      BOOST_CHECK(
        negative_binomial_distribution<RealType>::estimate_lower_bound_on_p(
        N+k, N, P)
        <=
        N / (N+k)
        );
      BOOST_CHECK(
        N / (N+k)
        <=
        negative_binomial_distribution<RealType>::estimate_upper_bound_on_p(
        N+k, N, P)
        );
    }

    // Estimate sample size:
    BOOST_CHECK_CLOSE(
      negative_binomial_distribution<RealType>::estimate_number_of_trials(
      k, p, P),
      N+k, tol);
    BOOST_CHECK_CLOSE(
      negative_binomial_distribution<RealType>::estimate_number_of_trials(
      boost::math::complement(k, p, Q)),
      N+k, tol);

    // Double check consistency of CDF and PDF by computing the finite sum:
    RealType sum = 0;
    for(unsigned i = 0; i <= k; ++i)
    {
      sum += pdf(bn, RealType(i));
    }
    BOOST_CHECK_CLOSE(sum, P, tol);

    // Complement is not possible since sum is to infinity.
  } //
} // test_spot

template <class RealType> // Any floating-point type RealType.
void test_spots(RealType)
{
  // Basic sanity checks, test data is to double precision only
  // so set tolerance to 1000 eps expressed as a percent, or
  // 1000 eps of type double expressed as a percent, whichever
  // is the larger.

  RealType tolerance = (std::max)
    (boost::math::tools::epsilon<RealType>(),
    static_cast<RealType>(std::numeric_limits<double>::epsilon()));
  tolerance *= 100 * 1000;

  cout << "Tolerance = " << tolerance << "%." << endl;

  RealType tol1eps = boost::math::tools::epsilon<RealType>() * 2; // Very tight, suit exact values.
  RealType tol2eps = boost::math::tools::epsilon<RealType>() * 2; // Tight, suit exact values.
  RealType tol5eps = boost::math::tools::epsilon<RealType>() * 5; // Wider 5 epsilon.
  cout << "Tolerance 5 eps = " << tol5eps << "%." << endl;

  // Sources of spot test values:

  // MathCAD defines pbinom(k, r, p)
  // returns pr(X , k) when random variable X has the binomial distribution with parameters r and p.
  // 0 <= k
  // r > 0
  // 0 <= p <= 1
  // P = pbinom(30, 500, 0.05) = 0.869147702104609

  using boost::math::negative_binomial_distribution;
  using  ::boost::math::negative_binomial;
  using  ::boost::math::cdf;
  using  ::boost::math::pdf;

  // Test negative binomial using cdf spot values from MathCAD cdf = pnbinom(k, r, p).
  // These test quantiles and complements as well.

  test_spot(  // pnbinom(1,2,0.5) = 0.5
  static_cast<RealType>(2),   // successes r
  static_cast<RealType>(1),   // Number of failures, k
  static_cast<RealType>(0.5), // Probability of success as fraction, p
  static_cast<RealType>(0.5), // Probability of result (CDF), P
  static_cast<RealType>(0.5),  // complement CCDF Q = 1 - P
  tolerance);

  test_spot( // pbinom(0, 2, 0.25)
  static_cast<RealType>(2),    // successes r
  static_cast<RealType>(0),    // Number of failures, k
  static_cast<RealType>(0.25),
  static_cast<RealType>(0.0625),                    // Probability of result (CDF), P
  static_cast<RealType>(0.9375),                    // Q = 1 - P
  tolerance);

  test_spot(  // pbinom(48,8,0.25)
  static_cast<RealType>(8),     // successes r
  static_cast<RealType>(48),    // Number of failures, k
  static_cast<RealType>(0.25),                    // Probability of success, p
  static_cast<RealType>(9.826582228110670E-1),     // Probability of result (CDF), P
  static_cast<RealType>(1 - 9.826582228110670E-1),   // Q = 1 - P
  tolerance);

  test_spot(  // pbinom(2,5,0.4)
  static_cast<RealType>(5),     // successes r
  static_cast<RealType>(2),     // Number of failures, k
  static_cast<RealType>(0.4),                    // Probability of success, p
  static_cast<RealType>(9.625600000000020E-2),     // Probability of result (CDF), P
  static_cast<RealType>(1 - 9.625600000000020E-2),   // Q = 1 - P
  tolerance);

  test_spot(  // pbinom(10,100,0.9)
  static_cast<RealType>(100),     // successes r
  static_cast<RealType>(10),     // Number of failures, k
  static_cast<RealType>(0.9),                    // Probability of success, p
  static_cast<RealType>(4.535522887695670E-1),     // Probability of result (CDF), P
  static_cast<RealType>(1 - 4.535522887695670E-1),   // Q = 1 - P
  tolerance);

  test_spot(  // pbinom(1,100,0.991)
  static_cast<RealType>(100),     // successes r
  static_cast<RealType>(1),     // Number of failures, k
  static_cast<RealType>(0.991),                    // Probability of success, p
  static_cast<RealType>(7.693413044217000E-1),     // Probability of result (CDF), P
  static_cast<RealType>(1 - 7.693413044217000E-1),   // Q = 1 - P
  tolerance);

  test_spot(  // pbinom(10,100,0.991)
  static_cast<RealType>(100),     // successes r
  static_cast<RealType>(10),     // Number of failures, k
  static_cast<RealType>(0.991),                    // Probability of success, p
  static_cast<RealType>(9.999999940939000E-1),     // Probability of result (CDF), P
  static_cast<RealType>(1 - 9.999999940939000E-1),   // Q = 1 - P
  tolerance);

if(std::numeric_limits<RealType>::is_specialized)
{ // An extreme value test that takes 3 minutes using the real concept type
  // for which numeric_limits<RealType>::is_specialized == false, deliberately
  // and for which there is no Lanczos approximation defined (also deliberately)
  // giving a very slow computation, but with acceptable accuracy.
  // A possible enhancement might be to use a normal approximation for
  // extreme values, but this is not implemented.
  test_spot(  // pbinom(100000,100,0.001)
  static_cast<RealType>(100),     // successes r
  static_cast<RealType>(100000),     // Number of failures, k
  static_cast<RealType>(0.001),                    // Probability of success, p
  static_cast<RealType>(5.173047534260320E-1),     // Probability of result (CDF), P
  static_cast<RealType>(1 - 5.173047534260320E-1),   // Q = 1 - P
  tolerance*1000); // *1000 is OK 0.51730475350664229  versus

  // functions.wolfram.com
  //   for I[0.001](100, 100000+1) gives:
  // Wolfram       0.517304753506834882009032744488738352004003696396461766326713
  // JM nonLanczos 0.51730475350664229 differs at the 13th decimal digit.
  // MathCAD       0.51730475342603199 differs at 10th decimal digit.
}

/* */
  // End of single spot tests using RealType

  // Tests on cdf:
// MathCAD pbinom k, r, p) == failures, successes,

  BOOST_CHECK_CLOSE(cdf(
    negative_binomial_distribution<RealType>(static_cast<RealType>(2), static_cast<RealType>(0.5)), // successes = 2,prob 0.25
    static_cast<RealType>(0) ), // k = 0
    static_cast<RealType>(0.25), // probability 1/4
    tolerance);

  BOOST_CHECK_CLOSE(cdf(complement(
    negative_binomial_distribution<RealType>(static_cast<RealType>(2), static_cast<RealType>(0.5)), // successes = 2,prob 0.25
    static_cast<RealType>(0) )), // k = 0
    static_cast<RealType>(0.75), // probability 3/4
    tolerance);

  // Tests on PDF:
  BOOST_CHECK_CLOSE(
  pdf(negative_binomial_distribution<RealType>(static_cast<RealType>(2), static_cast<RealType>(0.5)),
  static_cast<RealType>(0) ),  // k = 0.
  static_cast<RealType>(0.25), // 0
  tolerance);

  BOOST_CHECK_CLOSE(
  pdf(negative_binomial_distribution<RealType>(static_cast<RealType>(4), static_cast<RealType>(0.5)),
  static_cast<RealType>(0)),  // k = 0.
  static_cast<RealType>(0.0625), // exact 1/16
  tolerance);

  BOOST_CHECK_CLOSE(
  pdf(negative_binomial_distribution<RealType>(static_cast<RealType>(20), static_cast<RealType>(0.25)),
  static_cast<RealType>(0)),  // k = 0
  static_cast<RealType>(9.094947017729270E-13), // pbinom(0,20,0.25) = 9.094947017729270E-13
  tolerance);

  BOOST_CHECK_CLOSE(
  pdf(negative_binomial_distribution<RealType>(static_cast<RealType>(20), static_cast<RealType>(0.2)),
  static_cast<RealType>(0)),  // k = 0
  static_cast<RealType>(1.0485760000000003e-014), // MathCAD 1.048576000000000E-14
  tolerance);

  BOOST_CHECK_CLOSE(
  pdf(negative_binomial_distribution<RealType>(static_cast<RealType>(10), static_cast<RealType>(0.1)),
  static_cast<RealType>(0)),  // k = 0.
  static_cast<RealType>(1e-10), // MathCAD says zero, but suffers cancellation error?
  tolerance);

  BOOST_CHECK_CLOSE(
  pdf(negative_binomial_distribution<RealType>(static_cast<RealType>(20), static_cast<RealType>(0.1)),
  static_cast<RealType>(0)),  // k = 0.
  static_cast<RealType>(1e-20), // MathCAD says zero, but suffers cancellation error?
  tolerance);


  BOOST_CHECK_CLOSE( // .
  pdf(negative_binomial_distribution<RealType>(static_cast<RealType>(20), static_cast<RealType>(0.9)),
  static_cast<RealType>(0)),  // k.
  static_cast<RealType>(1.215766545905690E-1), // k=20  p = 0.9
  tolerance);

  // Test on cdf

  BOOST_CHECK_CLOSE( // k = 1.
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(20), static_cast<RealType>(0.25)),
  static_cast<RealType>(1)),  // k =1.
  static_cast<RealType>(1.455191522836700E-11),
  tolerance);

  BOOST_CHECK_SMALL( // Check within an epsilon with CHECK_SMALL
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(20), static_cast<RealType>(0.25)),
  static_cast<RealType>(1)) -
  static_cast<RealType>(1.455191522836700E-11),
  static_cast<RealType>(tol1eps) );

  // Some exact (probably - judging by trailing zeros) values.
  BOOST_CHECK_CLOSE(
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(0)),  // k.
  static_cast<RealType>(1.525878906250000E-5),
  tolerance);

  BOOST_CHECK_CLOSE(
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(0)),  // k.
  static_cast<RealType>(1.525878906250000E-5),
  tolerance);

  BOOST_CHECK_SMALL(
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(0)) -
  static_cast<RealType>(1.525878906250000E-5),
  tol2eps );

  BOOST_CHECK_CLOSE( // k = 1.
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(1)),  // k.
  static_cast<RealType>(1.068115234375010E-4),
  tolerance);

  BOOST_CHECK_CLOSE( // k = 2.
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(2)),  // k.
  static_cast<RealType>(4.158020019531300E-4),
  tolerance);

  BOOST_CHECK_CLOSE( // k = 3.
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(3)),  // k.bristow
  static_cast<RealType>(1.188278198242200E-3),
  tolerance);

  BOOST_CHECK_CLOSE( // k = 4.
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(4)),  // k.
  static_cast<RealType>(2.781510353088410E-3),
  tolerance);

  BOOST_CHECK_CLOSE( // k = 5.
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(5)),  // k.
  static_cast<RealType>(5.649328231811500E-3),
  tolerance);

  BOOST_CHECK_CLOSE( // k = 6.
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(6)),  // k.
  static_cast<RealType>(1.030953228473680E-2),
  tolerance);

  BOOST_CHECK_CLOSE( // k = 7.
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(7)),  // k.
  static_cast<RealType>(1.729983836412430E-2),
  tolerance);

  BOOST_CHECK_CLOSE( // k = 8.
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(8)),  // k = n.
  static_cast<RealType>(2.712995628826370E-2),
  tolerance);

  BOOST_CHECK_CLOSE( //
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(48)),  // k
  static_cast<RealType>(9.826582228110670E-1),
  tolerance);

  BOOST_CHECK_CLOSE( //
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(64)),  // k
  static_cast<RealType>(9.990295004935590E-1),
  tolerance);

  BOOST_CHECK_CLOSE( //
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(5), static_cast<RealType>(0.4)),
  static_cast<RealType>(26)),  // k
  static_cast<RealType>(9.989686246611190E-1),
  tolerance);

  BOOST_CHECK_CLOSE( //
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(5), static_cast<RealType>(0.4)),
  static_cast<RealType>(2)),  // k failures
  static_cast<RealType>(9.625600000000020E-2),
  tolerance);

  BOOST_CHECK_CLOSE( //
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(50), static_cast<RealType>(0.9)),
  static_cast<RealType>(20)),  // k
  static_cast<RealType>(9.999970854144170E-1),
  tolerance);

  BOOST_CHECK_CLOSE( //
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(500), static_cast<RealType>(0.7)),
  static_cast<RealType>(200)),  // k
  static_cast<RealType>(2.172846379930550E-1),
  tolerance* 2);

  BOOST_CHECK_CLOSE( //
  cdf(negative_binomial_distribution<RealType>(static_cast<RealType>(50), static_cast<RealType>(0.7)),
  static_cast<RealType>(20)),  // k
  static_cast<RealType>(4.550203671301790E-1),
  tolerance);

  negative_binomial_distribution<RealType> dist(static_cast<RealType>(8), static_cast<RealType>(0.25));
  using namespace std; // ADL of std names.
  // mean:
  BOOST_CHECK_CLOSE(
    mean(dist)
    , static_cast<RealType>(8 * ( 1 - 0.25) /0.25), tol5eps);
  // variance:
  BOOST_CHECK_CLOSE(
    variance(dist)
    , static_cast<RealType>(8 * (1 - 0.25) / (0.25 * 0.25)), tol5eps);
  // std deviation:
  BOOST_CHECK_CLOSE(
    standard_deviation(dist), // 9.79795897113271239270
    static_cast<RealType>(9.797958971132712392789136298823565567864), // using functions.wolfram.com
    //                              9.79795897113271152534  == sqrt(8 * (1 - 0.25) / (0.25 * 0.25)))
    tol5eps * 100);

  BOOST_CHECK_CLOSE(
    skewness(dist), //
    static_cast<RealType>(0.71443450831176036),
    // using http://mathworld.wolfram.com/skewness.html
    tol5eps * 100);

  BOOST_CHECK_CLOSE(
    kurtosis_excess(dist), //
    static_cast<RealType>(0.7604166666666666666666666666666666667), // using Wikipedia Kurtosis(excess) formula
    tol5eps * 100);

  BOOST_CHECK_CLOSE(
    kurtosis(dist), // true 
    static_cast<RealType>(3.76041666666666666666666666666666666667), // 
    tol5eps * 100);

  // hazard:
  RealType x = static_cast<RealType>(0.125);
  BOOST_CHECK_CLOSE(
  hazard(dist, x)
  , pdf(dist, x) / cdf(complement(dist, x)), tol5eps);
  // cumulative hazard:
  BOOST_CHECK_CLOSE(
  chf(dist, x)
  , -log(cdf(complement(dist, x))), tol5eps);
  // coefficient_of_variation:
  BOOST_CHECK_CLOSE(
  coefficient_of_variation(dist)
  , standard_deviation(dist) / mean(dist), tol5eps);

  // Special cases for PDF:
  BOOST_CHECK_EQUAL(
  pdf(
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0)), //
  static_cast<RealType>(0)),
  static_cast<RealType>(0) );

  BOOST_CHECK_EQUAL(
  pdf(
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0)),
  static_cast<RealType>(0.0001)),
  static_cast<RealType>(0) );

  BOOST_CHECK_EQUAL(
  pdf(
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(1)),
  static_cast<RealType>(0.001)),
  static_cast<RealType>(0) );

  BOOST_CHECK_EQUAL(
  pdf(
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(1)),
  static_cast<RealType>(8)),
  static_cast<RealType>(0) );

  BOOST_CHECK_SMALL(
  pdf(
   negative_binomial_distribution<RealType>(static_cast<RealType>(2), static_cast<RealType>(0.25)),
  static_cast<RealType>(0))-
  static_cast<RealType>(0.0625),
  boost::math::tools::epsilon<RealType>() ); // Expect exact, but not quite.
  // numeric_limits<RealType>::epsilon()); // Not suitable for real concept!

  // Quantile boundary cases checks:
  BOOST_CHECK_EQUAL(
  quantile(  // zero P < cdf(0) so should be exactly zero.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(0)),
  static_cast<RealType>(0));

  BOOST_CHECK_EQUAL(
  quantile(  // min P < cdf(0) so should be exactly zero.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(boost::math::tools::min_value<RealType>())),
  static_cast<RealType>(0));

  BOOST_CHECK_CLOSE_FRACTION(
  quantile(  // Small P < cdf(0) so should be near zero.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(boost::math::tools::epsilon<RealType>())), // 
  static_cast<RealType>(0),
    tol5eps);

  BOOST_CHECK_CLOSE(
  quantile(  // Small P < cdf(0) so should be exactly zero.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(0.0001)),
  static_cast<RealType>(0.95854156929288470),
    tol5eps * 100);

  //BOOST_CHECK(  // Fails with overflow for real_concept
  //quantile(  // Small P near 1 so k failures should be big.
  //negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  //static_cast<RealType>(1 - boost::math::tools::epsilon<RealType>())) <=
  //static_cast<RealType>(189.56999032670058)  // 106.462769 for float
  //);

if(std::numeric_limits<RealType>::has_infinity)
{ // BOOST_CHECK tests for infinity using std::numeric_limits<>::infinity()
  // Note that infinity is not implemented for real_concept, so these tests
  // are only done for types, like built-in float, double.. that have infinity.
  // Note that these assume that  BOOST_MATH_THROW_ON_OVERFLOW_ERROR is NOT defined.
  // #define BOOST_MATH_THROW_ON_OVERFLOW_ERROR would give a throw here.
  // #define BOOST_MATH_THROW_ON_DOMAIN_ERROR IS defined, so the throw path
  // of error handling is tested below with BOOST_CHECK_THROW tests.

  BOOST_CHECK(
  quantile(  // At P == 1 so k failures should be infinite.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(1)) ==
  //static_cast<RealType>(boost::math::tools::infinity<RealType>())
  static_cast<RealType>(std::numeric_limits<RealType>::infinity()) );

  BOOST_CHECK_EQUAL(
  quantile(  // At 1 == P  so should be infinite.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(1)), //
  std::numeric_limits<RealType>::infinity() );

  BOOST_CHECK_EQUAL(
  quantile(complement(  // Q zero 1 so P == 1 < cdf(0) so should be exactly infinity.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(0))),
  std::numeric_limits<RealType>::infinity() );

 } // test for infinity using std::numeric_limits<>::infinity()
else
{ // real_concept case, so check it throws

  BOOST_CHECK_THROW(
  quantile(  // At P == 1 so k failures should be infinite.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(1)),
  std::overflow_error );

  BOOST_CHECK_THROW(
  quantile(complement(  // Q zero 1 so P == 1 < cdf(0) so should be exactly infinity.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(0))),
  std::overflow_error);

}
  BOOST_CHECK( // Should work for built-in and real_concept.
  quantile(complement(  // Q very near to 1 so P nearly 1  < so should be large > 384.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(boost::math::tools::min_value<RealType>())))
   >= static_cast<RealType>(384) );

  BOOST_CHECK_EQUAL(
  quantile(  //  P ==  0 < cdf(0) so should be zero.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(0)),
  static_cast<RealType>(0));

  // Quantile Complement boundary cases:

  BOOST_CHECK_EQUAL(
  quantile(complement(  // Q = 1 so P = 0 < cdf(0) so should be exactly zero.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(1))),
  static_cast<RealType>(0)
  );

  BOOST_CHECK_EQUAL(
  quantile(complement(  // Q very near 1 so P == epsilon < cdf(0) so should be exactly zero.
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(1 - boost::math::tools::epsilon<RealType>()))),
  static_cast<RealType>(0)
  );


  // Check that duff arguments throw domain_error:
  BOOST_CHECK_THROW(
  pdf( // Negative successes!
  negative_binomial_distribution<RealType>(static_cast<RealType>(-1), static_cast<RealType>(0.25)),
  static_cast<RealType>(0)), std::domain_error
  );
  BOOST_CHECK_THROW(
  pdf( // Negative success_fraction!
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(-0.25)),
  static_cast<RealType>(0)), std::domain_error
  );
  BOOST_CHECK_THROW(
  pdf( // Success_fraction > 1!
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(1.25)),
  static_cast<RealType>(0)),
  std::domain_error
  );
  BOOST_CHECK_THROW(
  pdf( // Negative k argument !
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(-1)),
  std::domain_error
  );
  //BOOST_CHECK_THROW(
  //pdf( // Unlike binomial there is NO limit on k (failures)
  //negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  //static_cast<RealType>(9)), std::domain_error
  //);
  BOOST_CHECK_THROW(
  cdf(  // Negative k argument !
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(0.25)),
  static_cast<RealType>(-1)),
  std::domain_error
  );
  BOOST_CHECK_THROW(
  cdf( // Negative success_fraction!
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(-0.25)),
  static_cast<RealType>(0)), std::domain_error
  );
  BOOST_CHECK_THROW(
  cdf( // Success_fraction > 1!
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(1.25)),
  static_cast<RealType>(0)), std::domain_error
  );
  BOOST_CHECK_THROW(
  quantile(  // Negative success_fraction!
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(-0.25)),
  static_cast<RealType>(0)), std::domain_error
  );
  BOOST_CHECK_THROW(
  quantile( // Success_fraction > 1!
  negative_binomial_distribution<RealType>(static_cast<RealType>(8), static_cast<RealType>(1.25)),
  static_cast<RealType>(0)), std::domain_error
  );
  // End of check throwing 'duff' out-of-domain values.

  return;

} // template <class RealType> void test_spots(RealType) // Any floating-point type RealType.

int test_main(int, char* [])
{
  test_spots(0.0); // Test double.
  // Check that can generate negative_binomial distribution using the two convenience methods:
  using namespace boost::math;
	negative_binomial mynb1(2., 0.5); // Using typedef - default type is double.
	negative_binomial_distribution<> myf2(2., 0.5); // Using default RealType double.

  // Basic sanity-check spot values.
#ifdef BOOST_MATH_THROW_ON_DOMAIN_ERROR
  cout << "BOOST_MATH_THROW_ON_DOMAIN_ERROR" << " is defined to throw on domain error." << endl;
#else
  cout << "BOOST_MATH_THROW_ON_DOMAIN_ERROR" << " is NOT defined, so NO throw on domain error." << endl;
#endif

  // This is a visual sanity check that everything is OK:
  cout << setprecision(17) << showpoint << endl;
  // Show double max_digits10 precision, including trailing zeros.

  // Test some simple double only examples.
  negative_binomial_distribution<double> my8dist(8., 0.25);
  // 8 successes (r), 0.25 success fraction = 35% or 1 in 4 successes.
  // Note: double values (matching the distribution definition) avoid the need for any casting.

  //RealType estimate_lower_bound_on_p(RealType failures, RealType successes, RealType probability);
  //RealType estimate_upper_bound_on_p(RealType failures, RealType successes, RealType probability);
  //RealType estimate_number_of_trials(RealType failures, RealType successes, RealType probability);
  //RealType estimate_number_of_trials(RealType k, RealType p, RealType probability);
  //cout << my8dist.estimate_lower_bound_on_p(1, 8, 0.5) << endl;

  BOOST_CHECK_EQUAL(my8dist.successes(), static_cast<double>(8));
  BOOST_CHECK_EQUAL(my8dist.success_fraction(), static_cast<double>(1./4.)); // Exact.

  double tol = boost::math::tools::epsilon<double>() * 100 * 10;
  // * 100 for %, so tol is 10 epsilon.
  BOOST_CHECK_SMALL(cdf(my8dist, 2.), 4.1580200195313E-4);
  BOOST_CHECK_SMALL(cdf(my8dist, 8.), 0.027129956288264);
  BOOST_CHECK_CLOSE(cdf(my8dist, 16.), 0.233795830683125, tol);

  BOOST_CHECK_CLOSE(pdf(
  negative_binomial_distribution<double>(2., 0.5),
  1.),
  static_cast<double>(0.25),
  tol);

  BOOST_CHECK_CLOSE(cdf(complement(
    negative_binomial_distribution<double>(2., 0.5), //
    1.)), // k
    static_cast<double>(0.5), // half
    tol);

  BOOST_CHECK_CLOSE(cdf(
    negative_binomial_distribution<double>(2., 0.5),
    1.),
    static_cast<double>(0.5),
    tol);

  BOOST_CHECK_CLOSE(
  quantile(
  negative_binomial_distribution<double>(2., 0.5),
  0.5),
  1., //
  tol);

  BOOST_CHECK_CLOSE(
  cdf(
  negative_binomial_distribution<double>(8., 0.25),
  16.),
  0.233795830683125, //
  tol);

  BOOST_CHECK_CLOSE(
  quantile(
  negative_binomial_distribution<double>(8., 0.25),
  0.233795830683125),
  16., //
  tol);

  BOOST_CHECK_EQUAL( // Special cases probability == 0 and p == 1
  quantile(
  negative_binomial_distribution<double>(8., 0.25),
  1.), // Special case of requiring certainty,
  std::numeric_limits<double>::infinity() // which would require infinite trials.
  );

  BOOST_CHECK_EQUAL(
  quantile(
  negative_binomial_distribution<double>(8., 0.25),
  0.), // Special case of requiring NO certainty,
  0 // which would not require any trials.
  );

  BOOST_CHECK_EQUAL( // Special cases probability == 0 and p == 1
  quantile(complement(
  negative_binomial_distribution<double>(8., 0.25),
  1.)), // Special case of not requiring any certainty,
  0 // which would require any (zero) trials.
  );

  //BOOST_CHECK_EQUAL(
  //quantile(complement(
  //negative_binomial_distribution<double>(8., 0.25),
  //std::numeric_limits<double>::min())), // Special case of requiring tiny certainty,
  //8 // which would require more than successes (8) trials.
  //);  // denorm_min needs 2592.5954063179784 trials
      // min requireds 2588.7765527873094 trials.

  BOOST_CHECK_EQUAL(
  quantile(complement(
  negative_binomial_distribution<double>(8., 0.25),
  0.)), // Special case of requiring probability 1 - 0 == 1 == certainty,
  std::numeric_limits<double>::infinity() // which would require infinity trials.
  );


  {
    cout <<" Probability   quantile    expected failures" << endl;
    typedef double RealType;
    cout << "quantile(my8dist, 0) == " << quantile(my8dist, 0) << endl;
    cout << "quantile(my8dist, denorm_min) == " << quantile(my8dist, std::numeric_limits<double>::denorm_min()) << endl;
    cout << "quantile(my8dist, min) == " << quantile(my8dist, std::numeric_limits<double>::min()) << endl;
    cout << "quantile(my8dist, epsilon) == " << quantile(my8dist, std::numeric_limits<double>::epsilon()) << endl;
    cout << "quantile(my8dist, epsilon) == " << quantile(my8dist, 1e-6) << endl;
    for (RealType p = 0.01; p <= 1; p += 0.01) //
    {
      cout << p << ' '<< quantile(my8dist, p) << endl;
    }
    cout << "quantile(my8dist, 1-epsilon) == " << quantile(my8dist, 1 - std::numeric_limits<double>::epsilon()) << endl;
    cout << "quantile(my8dist, 1) == " << quantile(my8dist, 1) << endl;
    cout << "__________"<< endl;
  }

  {
    cout << endl;
    typedef double RealType;
    cout<< "quantile(complement(my8dist, zero)) == "  << quantile(complement(my8dist, 0)) << endl;
    cout<< "quantile(complement(my8dist, denorm_min)) == "  << quantile(complement(my8dist, std::numeric_limits<double>::denorm_min())) << endl;
    cout<< "quantile(complement(my8dist, min)) == "  << quantile(complement(my8dist, std::numeric_limits<double>::min())) << endl;
    cout<< "quantile(complement(my8dist, epsilon)) == "  << quantile(complement(my8dist, std::numeric_limits<double>::epsilon())) << endl;
    for (RealType p = 0.01; p <= 1 ; p += 0.01) //
    {
      cout << p << ' '<< 1 - p << ' ' << quantile(complement(my8dist, p)) << endl;
    }
    cout<< "quantile(complement(my8dist, 1-epsilon)) == "  << quantile(complement(my8dist, 1 - std::numeric_limits<double>::epsilon())) << endl;
    cout<< "quantile(complement(my8dist, 1)) == "  << quantile(complement(my8dist, 1)) << endl;
    cout << endl;
    cout << "__________"<< endl;
  }


   // Compare pdf using the simple formulae:
  //   exp(lgamma(r + k) - lgamma(r) - lgamma(k+1)) * pow(p, r) * pow((1-p), k)
  // with
  //   (p/(r+k) * ibeta_derivative(r, k+1, p) (as used in pdf)
  {
   typedef double RealType;
   RealType r = my8dist.successes();
    RealType p = my8dist.success_fraction();
    for (int i = 0; i <= r * 4; i++) //
    {
      RealType k = static_cast<RealType>(i);
      RealType pmf = exp(lgamma(r + k) - lgamma(r) - lgamma(k+1)) * pow(p, r) * pow((1-p), k);
      BOOST_CHECK_CLOSE(pdf(my8dist, static_cast<RealType>(k)), pmf, tol * 10);
      // 0.0015932321548461931
    // 0.0015932321548461866
  }

  // Double-check consistency of CDF and PDF by computing the finite sum of pdfs:
  RealType sum = 0;
  for(unsigned i = 0; i <= 20; ++i)
  {
    sum += pdf(my8dist, RealType(i));
  }

  cout << setprecision(17) << showpoint << sum <<' '  // 0.40025683281803714
  << cdf(my8dist, static_cast<RealType>(20)) << endl; // 0.40025683281803681
  BOOST_CHECK_CLOSE(sum, cdf(my8dist, static_cast<RealType>(20)), tol);
  }

  // (Parameter value, arbitrarily zero, only communicates the floating point type).
  test_spots(0.0F); // Test float.
  test_spots(0.0); // Test double.
  test_spots(0.0L); // Test long double.
  #if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
    test_spots(boost::math::concepts::real_concept(0.)); // Test real concept.
  #endif

  return 0;
} // int test_main(int, char* [])

/*

------ Build started: Project: test_negative_binomial, Configuration: Debug Win32 ------
Compiling...
test_negative_binomial.cpp
Linking...
Autorun "i:\boost-06-05-03-1300\libs\math\test\Math_test\debug\test_negative_binomial.exe"
Running 1 test case...
Tolerance = 2.22045e-011%.
Tolerance 5 eps = 1.11022e-015%.
BOOST_MATH_THROW_ON_DOMAIN_ERROR is defined to throw on domain error.
 Probability   quantile    expected failures
quantile(my8dist, 0) == 0
quantile(my8dist, denorm_min) == 0
quantile(my8dist, min) == 0
quantile(my8dist, epsilon) == 0
quantile(my8dist, epsilon) == 0
0.01 5.94544
0.02 7.3077
0.03 8.24308
0.04 8.98404
0.05 9.6108
0.06 10.1615
0.07 10.6575
0.08 11.1123
0.09 11.5345
0.1 11.9306
0.11 12.3052
0.12 12.6617
0.13 13.0029
0.14 13.3309
0.15 13.6474
0.16 13.954
0.17 14.2517
0.18 14.5417
0.19 14.8248
0.2 15.1017
0.21 15.3731
0.22 15.6396
0.23 15.9016
0.24 16.1597
0.25 16.4141
0.26 16.6654
0.27 16.9138
0.28 17.1597
0.29 17.4032
0.3 17.6447
0.31 17.8844
0.32 18.1226
0.33 18.3593
0.34 18.5949
0.35 18.8296
0.36 19.0634
0.37 19.2966
0.38 19.5293
0.39 19.7618
0.4 19.994
0.41 20.2263
0.42 20.4587
0.43 20.6915
0.44 20.9246
0.45 21.1584
0.46 21.3928
0.47 21.6282
0.48 21.8645
0.49 22.102
0.5 22.3409
0.51 22.5812
0.52 22.8231
0.53 23.0667
0.54 23.3124
0.55 23.5601
0.56 23.8101
0.57 24.0626
0.58 24.3178
0.59 24.5759
0.6 24.837
0.61 25.1015
0.62 25.3695
0.63 25.6413
0.64 25.9172
0.65 26.1975
0.66 26.4825
0.67 26.7725
0.68 27.0679
0.69 27.3691
0.7 27.6765
0.71 27.9907
0.72 28.312
0.73 28.6411
0.74 28.9787
0.75 29.3253
0.76 29.6819
0.77 30.0492
0.78 30.4283
0.79 30.8203
0.8 31.2264
0.81 31.6482
0.82 32.0873
0.83 32.5456
0.84 33.0256
0.85 33.53
0.86 34.0621
0.87 34.6259
0.88 35.2264
0.89 35.8699
0.9 36.5643
0.91 37.3201
0.92 38.1514
0.93 39.0777
0.94 40.1276
0.95 41.3448
0.96 42.8019
0.97 44.6337
0.98 47.1381
0.99 51.2478
quantile(my8dist, 1-epsilon) == 189.57
quantile(my8dist, 1) == 1.#INF
__________
quantile(complement(my8dist, zero)) == 1.#INF
quantile(complement(my8dist, denorm_min)) == 2592.6
quantile(complement(my8dist, min)) == 2588.78
quantile(complement(my8dist, epsilon)) == 189.57
0.01 0.99 51.2478
0.02 0.98 47.1381
0.03 0.97 44.6337
0.04 0.96 42.8019
0.05 0.95 41.3448
0.06 0.94 40.1276
0.07 0.93 39.0777
0.08 0.92 38.1514
0.09 0.91 37.3201
0.1 0.9 36.5643
0.11 0.89 35.8699
0.12 0.88 35.2264
0.13 0.87 34.6259
0.14 0.86 34.0621
0.15 0.85 33.53
0.16 0.84 33.0256
0.17 0.83 32.5456
0.18 0.82 32.0873
0.19 0.81 31.6482
0.2 0.8 31.2264
0.21 0.79 30.8203
0.22 0.78 30.4283
0.23 0.77 30.0492
0.24 0.76 29.6819
0.25 0.75 29.3253
0.26 0.74 28.9787
0.27 0.73 28.6411
0.28 0.72 28.312
0.29 0.71 27.9907
0.3 0.7 27.6765
0.31 0.69 27.3691
0.32 0.68 27.0679
0.33 0.67 26.7725
0.34 0.66 26.4825
0.35 0.65 26.1975
0.36 0.64 25.9172
0.37 0.63 25.6413
0.38 0.62 25.3695
0.39 0.61 25.1015
0.4 0.6 24.837
0.41 0.59 24.5759
0.42 0.58 24.3178
0.43 0.57 24.0626
0.44 0.56 23.8101
0.45 0.55 23.5601
0.46 0.54 23.3124
0.47 0.53 23.0667
0.48 0.52 22.8231
0.49 0.51 22.5812
0.5 0.5 22.3409
0.51 0.49 22.102
0.52 0.48 21.8645
0.53 0.47 21.6282
0.54 0.46 21.3928
0.55 0.45 21.1584
0.56 0.44 20.9246
0.57 0.43 20.6915
0.58 0.42 20.4587
0.59 0.41 20.2263
0.6 0.4 19.994
0.61 0.39 19.7618
0.62 0.38 19.5293
0.63 0.37 19.2966
0.64 0.36 19.0634
0.65 0.35 18.8296
0.66 0.34 18.5949
0.67 0.33 18.3593
0.68 0.32 18.1226
0.69 0.31 17.8844
0.7 0.3 17.6447
0.71 0.29 17.4032
0.72 0.28 17.1597
0.73 0.27 16.9138
0.74 0.26 16.6654
0.75 0.25 16.4141
0.76 0.24 16.1597
0.77 0.23 15.9016
0.78 0.22 15.6396
0.79 0.21 15.3731
0.8 0.2 15.1017
0.81 0.19 14.8248
0.82 0.18 14.5417
0.83 0.17 14.2517
0.84 0.16 13.954
0.85 0.15 13.6474
0.86 0.14 13.3309
0.87 0.13 13.0029
0.88 0.12 12.6617
0.89 0.11 12.3052
0.9 0.1 11.9306
0.91 0.09 11.5345
0.92 0.08 11.1123
0.93 0.07 10.6575
0.94 0.06 10.1615
0.95 0.05 9.6108
0.96 0.04 8.98404
0.97 0.03 8.24308
0.98 0.02 7.3077
0.99 0.01 5.94544
quantile(complement(my8dist, 1-epsilon)) == 0
quantile(complement(my8dist, 1)) == 0
__________
0.40025683281803698 0.40025683281803687
Tolerance = 0.0119209%.
Tolerance 5 eps = 5.96046e-007%.
Tolerance = 2.22045e-011%.
Tolerance 5 eps = 1.11022e-015%.
Tolerance = 2.22045e-011%.
Tolerance 5 eps = 1.11022e-015%.
Tolerance = 2.22045e-011%.
Tolerance 5 eps = 1.11022e-015%.
*** No errors detected
Build Time 0:08
Build log was saved at "file://i:\boost-06-05-03-1300\libs\math\test\Math_test\test_negative_binomial\Debug\BuildLog.htm"
test_negative_binomial - 0 error(s), 0 warning(s)
========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========



*/
