#ifndef NOISEMODEL_H_INCLUDED
#define NOISEMODEL_H_INCLUDED

/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file NoiseModel.h
 * @date  Jan 13, 2010
 * @author Richard Roberts
 * @author Frank Dellaert
 */

/**
Sharednoisemodel was deleted.
*/


#include <Eigen/Core>
#include <Eigen/Cholesky>
#include "../base/Matrix.h"
#include "../base/MatCal.h"

#include <iostream>
#include "../gmfconfig.h"

using namespace std;

namespace minisam
{

// Forward declaration
class GaussianNoiseModel;
class DiagonalNoiseModel;
class ConstrainedNoiseModel;
class IsotropicNoiseModel;

//---------------------------------------------------------------------------------------

/**
 * noiseModel::Base is the abstract base class for all noise models.
 *
 * Noise models must implement a 'whiten' function to normalize an error vector,
 * and an 'unwhiten' function to unnormalize an error vector.
 */

/**
 * Gaussian implements the mathematical model
 *  |R*x|^2 = |y|^2 with R'*R=inv(Sigma)
 * where
 *   y = whiten(x) = R*x
 *   x = unwhiten(x) = inv(R)*y
 * as indeed
 *   |y|^2 = y'*y = x'*R'*R*x
 * Various derived classes are available that are more efficient.
 * The named constructors return a shared_ptr because, when the smart flag is
 * true, the underlying object might be a derived class such as Diagonal.
 */
class GaussianNoiseModel
{
protected:
    int dim_;
    bool isConstrained_; // default false
    bool isUnit_;        // default false
    /** Matrix square root of information matrix (R) */
    Eigen::MatrixXd sqrt_information_;

public:
    /**
    * Return R itself, but note that Whiten(H) is cheaper than R*H
    */
    const Eigen::MatrixXd &thisR() const;

public:
    GaussianNoiseModel(int dim = 0);
    GaussianNoiseModel(const Eigen::MatrixXd &R,bool smart = true);

public:

    virtual ~GaussianNoiseModel();

    GaussianNoiseModel &operator=(const GaussianNoiseModel &rObj);

    /// Dimensionality
    int dim() const;
    void setdim(int sdim);
    /// true if a constrained noise model, saves slow/clumsy dynamic casting
    bool isConstrained() const;
    /// true if a unit noise model, saves slow/clumsy dynamic casting
    bool isUnit() const;

    void setR(const Eigen::MatrixXd& R);

    Eigen::MatrixXd information() const;


    virtual Eigen::VectorXd sigmas() const;
    virtual Eigen::VectorXd whiten(const Eigen::VectorXd &v) const;
    /**
    * Multiply a derivative with R (derivative of whiten)
    * Equivalent to whitening each column of the input matrix.
    */
    virtual Eigen::MatrixXd Whiten(const Eigen::MatrixXd &H) const;

    virtual double distance(const Eigen::VectorXd &v) const;

    /**
    * In-place version
    */
    virtual void WhitenInPlace(Eigen::MatrixXd &H) const;

    /**
    * Whiten a system, in place as well
    */
    virtual void WhitenSystem(std::vector<Eigen::MatrixXd> &A,
                              Eigen::VectorXd &b) const;
    virtual void WhitenSystem(Eigen::MatrixXd &A, Eigen::VectorXd &b) const;


    /** in-place whiten, override if can be done more efficiently */
    void whitenInPlace(Eigen::VectorXd &v) const;



}; // Gaussian
GaussianNoiseModel* GaussianNoiseModel_Covariance(
    const Eigen::MatrixXd &covariance, bool smart = true);

//---------------------------------------------------------------------------------------

/**
 * A diagonal noise model implements a diagonal covariance matrix, with the
 * elements of the diagonal specified in a Vector.  This class has no public
 * constructors, instead, use the static constructor functions Sigmas etc...
 */
class DiagonalNoiseModel : public GaussianNoiseModel
{
protected:
    /**
    * Standard deviations (sigmas), their inverse and inverse square
    * (weights/precisions) These are all computed at construction: the idea is to
    * use one shared model where computation is done only once, the common use
    * case in many problems.
    */
    Eigen::VectorXd sigmas_, invsigmas_;//, precisions_;

public:
    /** protected constructor - no initializations */

    int nullmodel; // 0 nonnull,1 null;

    DiagonalNoiseModel();

    DiagonalNoiseModel(const DiagonalNoiseModel &dg);

    /** constructor to allow for disabling initialization of invsigmas */
    DiagonalNoiseModel(const Eigen::VectorXd &sigmas);

    DiagonalNoiseModel &operator=(const DiagonalNoiseModel &rObj);

public:

    virtual ~DiagonalNoiseModel();


    /**
    * A diagonal noise model created by specifying a Vector of variances, i.e.
    * i.e. the diagonal of the covariance matrix.
    * @param variances A vector containing the variances of this noise model
    * @param smart check if can be simplified to derived class
    */
    static DiagonalNoiseModel* Variances(const Eigen::VectorXd &variances,
                                         bool smart = true);
    /**
    * A diagonal noise model created by specifying a Vector of precisions, i.e.
    * i.e. the diagonal of the information matrix, i.e., weights
    */
    static DiagonalNoiseModel* Precisions(const Eigen::VectorXd &precisions,
                                          bool smart = true);

    virtual Eigen::VectorXd sigmas() const;
    virtual Eigen::VectorXd whiten(const Eigen::VectorXd &v) const;
    virtual Eigen::MatrixXd Whiten(const Eigen::MatrixXd &H) const;
    virtual void WhitenInPlace(Eigen::MatrixXd &H) const;
    /**
    * Return standard deviations (sqrt of diagonal)
    */
    virtual double sigma(int i) const;

    /**
    * Return sqrt precisions
    */
    const Eigen::VectorXd invsigmas() const;
    double invsigma(int i) const;

    /**
    * Return precisions
    */
    const Eigen::VectorXd precisions() const;
    double precision(int i) const;


    /**
    * Apply appropriately weighted QR factorization to the system [A b]
    *               Q'  *   [A b]  =  [R d]
    * Dimensions: (r*m) * m*(n+1) = r*(n+1), where r = min(m,n).
    * This routine performs an in-place factorization on Ab.
    * Below-diagonal elements are set to zero by this routine.
    * @param Ab is the m*(n+1) augmented system matrix [A b]
    * @return Empty SharedDiagonal() noise model: R,d are whitened
    */
    virtual DiagonalNoiseModel* QR(Eigen::MatrixXd &Ab) const;

}; // Diagonal

//--------------------------------------------------------------------------------------

/**
 * A Constrained constrained model is a specialization of Diagonal which allows
 * some or all of the sigmas to be zero, forcing the error to be zero there.
 * All other Gaussian models are guaranteed to have a non-singular square-root
 * information matrix, but this class is specifically equipped to deal with
 * singular noise models, specifically: whiten will return zero on those
 * components that have zero sigma *and* zero error, unchanged otherwise.
 *
 * While a hard constraint may seem to be a case in which there is infinite
 * error, we do not ever produce an error value of infinity to allow for
 * constraints to actually be optimized rather than self-destructing if not
 * initialized correctly.
 */
class ConstrainedNoiseModel : public DiagonalNoiseModel
{
protected:
    // Sigmas are contained in the base class
    Eigen::VectorXd mu_; ///< Penalty function weight - needs to be large enough
    ///< to dominate soft constraints

    /**
    * protected constructor takes sigmas.
    * prevents any inf values
    * from appearing in invsigmas or precisions.
    * mu set to large default value (1000.0)
    */
    ConstrainedNoiseModel(
        const Eigen::VectorXd& sigmas);
    /**
    * Constructor that prevents any inf values
    * from appearing in invsigmas or precisions.
    * Allows for specifying mu.
    */
    ConstrainedNoiseModel(const Eigen::VectorXd &mu,
                          const Eigen::VectorXd &sigmas);

public:

    virtual ~ConstrainedNoiseModel();

    ConstrainedNoiseModel &operator=(const ConstrainedNoiseModel &rObj);

    /// Return true if a particular dimension is free or constrained
    bool constrained(int i) const;

    /// Access mu as a vector
    const Eigen::VectorXd mu() const;

    /**
    * A diagonal noise model created by specifying a Vector of
    * standard devations, some of which might be zero
    */
    static ConstrainedNoiseModel* MixedSigmas(const Eigen::VectorXd &mu,
            const Eigen::VectorXd &sigmas);
    /**
    * A diagonal noise model created by specifying a Vector of
    * standard devations, some of which might be zero
    */
    static ConstrainedNoiseModel* MixedSigmas(const Eigen::VectorXd &sigmas);

    /**
    * A diagonal noise model created by specifying a Vector of
    * precisions, some of which might be inf
    */
    static ConstrainedNoiseModel* MixedPrecisions(
        const Eigen::VectorXd &mu, const Eigen::VectorXd &precisions);

    /**
    * The distance function for a constrained noisemodel,
    * for non-constrained versions, uses sigmas, otherwise
    * uses the penalty function with mu
    */
    virtual double distance(const Eigen::VectorXd &v) const;

    /** Fully constrained variations*/

    /** Fully constrained variations */
    static ConstrainedNoiseModel* All(int dim, const Eigen::VectorXd &mu);

    /** Fully constrained variations with a mu parameter */
    static ConstrainedNoiseModel* All(int dim, double mu);

    /// Calculates error vector with weights applied
    virtual Eigen::VectorXd whiten(const Eigen::VectorXd &v) const;

    /// Whitening functions will perform partial whitening on rows
    /// with a non-zero sigma.  Other rows remain untouched.
    virtual Eigen::MatrixXd Whiten(const Eigen::MatrixXd &H) const;
    virtual void WhitenInPlace(Eigen::MatrixXd &H) const;

    /**
    * Apply QR factorization to the system [A b], taking into account constraints
    *               Q'  *   [A b]  =  [R d]
    * Dimensions: (r*m) * m*(n+1) = r*(n+1), where r = min(m,n).
    * This routine performs an in-place factorization on Ab.
    * Below-diagonal elements are set to zero by this routine.
    * @param Ab is the m*(n+1) augmented system matrix [A b]
    * @return diagonal noise model can be all zeros, mixed, or not-constrained
    */
    virtual DiagonalNoiseModel* QR(Eigen::MatrixXd &Ab) const;

    /**
    * Returns a Unit version of a constrained noisemodel in which
    * constrained sigmas remain constrained and the rest are unit scaled
    */
    DiagonalNoiseModel* unit() const;

}; // Constrained

//---------------------------------------------------------------------------------------
static void fixnoisemodel(const Eigen::VectorXd &sigmas,//Eigen::VectorXd &precisions,
                          Eigen::VectorXd &invsigmas);
/**
 * An isotropic noise model corresponds to a scaled diagonal covariance
 * To construct, use one of the static methods
 */

class IsotropicNoiseModel : public DiagonalNoiseModel
{
protected:
    double dinvsigma_;

public:
    IsotropicNoiseModel(int dim, double sigma);

    IsotropicNoiseModel();

    virtual ~IsotropicNoiseModel();

    IsotropicNoiseModel &operator=(const IsotropicNoiseModel &rObj);

    static IsotropicNoiseModel* Variance(int dim, double variance,
                                         bool smart = true);

    virtual double distance(const Eigen::VectorXd &v) const;
    virtual Eigen::VectorXd whiten(const Eigen::VectorXd &v) const;
    virtual Eigen::MatrixXd Whiten(const Eigen::MatrixXd &H) const;
    virtual void WhitenInPlace(Eigen::MatrixXd &H) const;

};

GaussianNoiseModel* NGaussianNoiseModel(const Eigen::MatrixXd &cov);

DiagonalNoiseModel* NDiagonalNoiseModelPrecision(const Eigen::VectorXd& precisions,bool smart=true);
DiagonalNoiseModel* DiagonalNoiseModelSigmas(const Eigen::VectorXd &sigmas,
        bool smart = true);

};
#endif // NOISEMODEL_H_INCLUDED

