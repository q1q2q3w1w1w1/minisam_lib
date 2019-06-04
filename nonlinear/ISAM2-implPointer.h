#ifndef ISAM2_IMPLPOINTER_H_INCLUDED
#define ISAM2_IMPLPOINTER_H_INCLUDED


/**
 * @file    ISAM2-impl.h
 * @brief   Incremental update functionality (ISAM2) for BayesTree, with fluid relinearization.
 * @author
 */


#include "../inference/BayesTreePointer.h"
#include "../nonlinear/ISAM2CliquePointer.h"
#include "../nonlinear/NonlinearFactorGraph.h"

/**
  * Add new variables to the ISAM2 system.
  * @param newTheta Initial values for new variables
  * @param theta Current solution to be augmented with new initialization
  * @param delta Current linear delta to be augmented with zeros
  * @param ordering Current ordering to be augmented with new variables
  * @param nodes Current BayesTree::Nodes index to be augmented with slots for new variables
  * @param keyFormatter Formatter for printing nonlinear keys during debugging
  */

void ISAM2ImplAddVariablesPointer(const std::map<int,Eigen::VectorXd>& newTheta,
                                  const std::map<int,Pose3>& newThetaPose,
                                  std::map<int,Eigen::VectorXd>& theta,
                                  std::map<int,Pose3>& ThetaPose,
                                  std::map<int,Eigen::VectorXd>& delta,
                                  std::map<int,Eigen::VectorXd>& deltaNewton, std::map<int,Eigen::VectorXd>& RgProd);
void ISAM2ImplAddVariablesPointer(const std::map<int,Eigen::VectorXd>& newTheta,
                                  const std::map<int,Pose2>& newThetaPose,
                                  std::map<int,Eigen::VectorXd>& theta,
                                  std::map<int,Pose2>& ThetaPose,
                                  std::map<int,Eigen::VectorXd>& delta,
                                  std::map<int,Eigen::VectorXd>& deltaNewton, std::map<int,Eigen::VectorXd>& RgProd);

/// Perform the first part of the bookkeeping updates for adding new factors.  Adds them to the
/// complete list of nonlinear factors, and populates the list of new factor indices, both
/// optionally finding and reusing empty factor slots.
void ISAM2ImplAddFactorsStep1Pointer(NonlinearFactorGraph& newFactors, bool useUnusedSlots,
                                     NonlinearFactorGraph& nonlinearFactors, std::vector<int>& newFactorIndices);

/**
 * Remove variables from the ISAM2 system.
 */

#ifdef GMF_Using_Pose3
void ISAM2ImplRemoveVariablesPointer(std::set<int>& unusedKeys,
                                     std::map<int,Eigen::VectorXd>& theta,std::map<int,Pose3>& thetaPose3, VariableIndex& variableIndex, std::map<int,Eigen::VectorXd>& delta,
                                     std::map<int,Eigen::VectorXd>& deltaNewton,
                                     std::map<int,Eigen::VectorXd>& RgProd, std::set<int>& replacedKeys,
                                     std::map<int,ISAM2CliquePointer*>& nodesbtc, //Base::Nodes& nodes,
                                     std::set<int>& fixedVariables);
#else
void ISAM2ImplRemoveVariablesPointer(std::set<int>& unusedKeys,
                                     std::map<int,Eigen::VectorXd>& theta,std::map<int,Pose2>& thetaPose2, VariableIndex& variableIndex, std::map<int,Eigen::VectorXd>& delta,
                                     std::map<int,Eigen::VectorXd>& deltaNewton,
                                     std::map<int,Eigen::VectorXd>& RgProd, std::set<int>& replacedKeys,
                                     std::map<int,ISAM2CliquePointer*>& nodesbtc, //Base::Nodes& nodes,
                                     std::set<int>& fixedVariables);

#endif

/**
 * Find the set of variables to be relinearized according to relinearizeThreshold.
 * Any variables in the VectorValues delta whose vector magnitude is greater than
 * or equal to relinearizeThreshold are returned.
 * @param delta The linear delta to check against the threshold
 * @param keyFormatter Formatter for printing nonlinear keys during debugging
 * @return The set of variable indices in delta whose magnitude is greater than or
 * equal to relinearizeThreshold
 */
std::set<int> ISAM2ImplCheckRelinearizationFullPointer(const std::map<int,Eigen::VectorXd>& delta,
        double relinearizeThresholdDouble,
        std::map<char,Eigen::VectorXd> *relinearizeThresholdMap);
// const ISAM2Params::RelinearizationThreshold& relinearizeThreshold);

/**
 * Find the set of variables to be relinearized according to relinearizeThreshold.
 * This check is performed recursively, starting at the top of the tree. Once a
 * variable in the tree does not need to be relinearized, no further checks in
 * that branch are performed. This is an approximation of the Full version, designed
 * to save time at the expense of accuracy.
 * @param delta The linear delta to check against the threshold
 * @param keyFormatter Formatter for printing nonlinear keys during debugging
 * @return The set of variable indices in delta whose magnitude is greater than or
 * equal to relinearizeThreshold
 */
std::set<int> ISAM2ImplCheckRelinearizationPartialPointer(const std::vector<ISAM2CliquePointer*>& roots,
        std::map<int,Eigen::VectorXd>& delta,  const  double relinearizeThresholdDouble,
        std::map<char,Eigen::VectorXd> *relinearizeThresholdMap);
// const ISAM2Params::RelinearizationThreshold& relinearizeThreshold);

/**
 * Recursively search this clique and its children for marked keys appearing
 * in the separator, and add the *frontal* keys of any cliques whose
 * separator contains any marked keys to the set \c keys.  The purpose of
 * this is to discover the cliques that need to be redone due to information
 * propagating to them from cliques that directly contain factors being
 * relinearized.
 *
 * The original comment says this finds all variables directly connected to
 * the marked ones by measurements.  Is this true, because it seems like this
 * would also pull in variables indirectly connected through other frontal or
 * separator variables?
 *
 * Alternatively could we trace up towards the root for each variable here?
 */
void ISAM2ImplFindAllPointer(const ISAM2CliquePointer* clique,
                             std::set<int>& keys, const std::set<int>& markedMask);
//void ISAM2ImplFindAllVector(const std::vector<ISAM2CliquePointer*>& nodebtc,
                          //   std::set<int>& keys, const std::set<int>& markedMask);

/**
 * Apply expmap to the given values, but only for indices appearing in
 * \c markedRelinMask.  Values are expmapped in-place.
 * \param [in, out] values The value to expmap in-place
 * \param delta The linear delta with which to expmap
 * \param ordering The ordering
 * \param mask Mask on linear indices, only \c true entries are expmapped
 * \param invalidateIfDebug If this is true, *and* NDEBUG is not defined,
 * expmapped deltas will be set to an invalid value (infinity) to catch bugs
 * where we might expmap something twice, or expmap it but then not
 * recalculate its delta.
 * @param keyFormatter Formatter for printing nonlinear keys during debugging
 */
void ISAM2ImplExpmapMaskedPointer(std::map<int,Eigen::VectorXd>& vectorvalues,std::map<int,Pose3>& Pose3values,
                                  std::map<int,Eigen::VectorXd>& delta,
                                  const std::set<int>& mask, std::map<int,Eigen::VectorXd>* invalidateIfDebug);
void ISAM2ImplExpmapMasked(std::map<int,Eigen::VectorXd>& vectorvalues,std::map<int,Pose3>& Pose3values,
                                  std::map<int,Eigen::VectorXd>& delta,
                                  const std::set<int>& mask, std::map<int,Eigen::VectorXd>* invalidateIfDebug);

void ISAM2ImplExpmapMaskedPointer(std::map<int,Eigen::VectorXd>& vectorvalues,std::map<int,Pose2>& Pose3values,
                                  std::map<int,Eigen::VectorXd>& delta,
                                  const std::set<int>& mask, std::map<int,Eigen::VectorXd>* invalidateIfDebug);
void ISAM2ImplExpmapMasked(std::map<int,Eigen::VectorXd>& vectorvalues,std::map<int,Pose2>& Pose3values,
                                  std::map<int,Eigen::VectorXd>& delta,
                                  const std::set<int>& mask, std::map<int,Eigen::VectorXd>* invalidateIfDebug);
/**
 * Update the Newton's method step point, using wildfire
 */
int ISAM2ImplUpdateGaussNewtonDeltaPointer(std::vector<ISAM2CliquePointer*>* roots,
        const std::set<int>& replacedKeys, std::map<int,Eigen::VectorXd>* delta, double wildfireThreshold);

/**
 * Update the RgProd (R*g) incrementally taking into account which variables
 * have been recalculated in \c replacedKeys.  Only used in Dogleg.
 */
int ISAM2ImplUpdateRgProdPointer(std::vector<ISAM2CliquePointer*>& roots,
                                 const std::set<int>& replacedKeys,
                                 const std::map<int,Eigen::VectorXd>& gradAtZero, std::map<int,Eigen::VectorXd>& RgProd);

/**
 * Compute the gradient-search point.  Only used in Dogleg.
 */
std::map<int,Eigen::VectorXd> ISAM2ImplComputeGradientSearchPointer(std::map<int,Eigen::VectorXd>& gradAtZero,
        const std::map<int,Eigen::VectorXd>& RgProd);

//};

bool internaloptimizeWildfireNodePointer(ISAM2CliquePointer* clique,double threshold,
        std::set<int>& changed, const std::set<int>& replaced, std::map<int,Eigen::VectorXd>* delta, int* count);
/// traits
/// Optimize the BayesTree, starting from the root.
/// @param replaced Needs to contain
/// all variables that are contained in the top of the Bayes tree that has been
/// redone.
/// @param delta The current solution, an offset from the linearization
/// point.
/// @param threshold The maximum change against the PREVIOUS delta for
/// non-replaced variables that can be ignored, ie. the old delta entry is kept
/// and recursive backsubstitution might eventually stop if none of the changed
/// variables are contained in the subtree.
/// @return The number of variables that were solved for

int optimizeWildfireNonRecursivePointer(std::vector<ISAM2CliquePointer*>* roots,
                                        double threshold, const std::set<int>& replaced, std::map<int,Eigen::VectorXd>* delta);

/// calculate the number of non-zero entries for the tree starting at clique (use root for complete matrix)


#endif // ISAM2-IMPLPOINTER_H_INCLUDED