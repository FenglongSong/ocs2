//
// Created by rgrandia on 06.08.20.
//

#include <ocs2_switched_model_interface/constraint/FrictionConeConstraint.h>

#include <ocs2_switched_model_interface/core/Rotations.h>
#include <ocs2_switched_model_interface/terrain/TerrainPlane.h>

namespace switched_model {

FrictionConeConstraint::FrictionConeConstraint(scalar_t frictionCoefficient, scalar_t regularization, int legNumber, scalar_t gripperForce)
    : BASE(ocs2::ConstraintOrder::Quadratic),
      frictionCoefficient_(frictionCoefficient),
      regularization_(regularization),
      gripperForce_(gripperForce),
      legNumber_(legNumber),
      t_R_w(matrix3_t::Identity()) {}

void FrictionConeConstraint::setSurfaceNormalInWorld(const vector3_t& surfaceNormalInWorld) {
  t_R_w = orientationWorldToTerrainFromSurfaceNormalInWorld(surfaceNormalInWorld);
}

scalar_array_t FrictionConeConstraint::getValue(scalar_t time, const state_vector_t& state, const input_vector_t& input) const {
  const vector3_t eulerXYZ = getOrientation(getComPose(state));
  const vector3_t forcesInBodyFrame = input.template segment<3>(3 * legNumber_);

  const auto localForce = computeLocalForces(eulerXYZ, forcesInBodyFrame);

  scalar_array_t constraintValue;
  constraintValue.emplace_back(coneConstraint(localForce));
  return constraintValue;
}

FrictionConeConstraint::LinearApproximation_t FrictionConeConstraint::getLinearApproximation(scalar_t time, const state_vector_t& state,
                                                                                             const input_vector_t& input) const {
  const vector3_t eulerXYZ = getOrientation(getComPose(state));
  const vector3_t forcesInBodyFrame = input.template segment<3>(3 * legNumber_);

  const auto localForce = computeLocalForces(eulerXYZ, forcesInBodyFrame);
  const auto localForceDerivatives = computeLocalForceDerivatives(eulerXYZ, forcesInBodyFrame);
  const auto coneLocalDerivatives = computeConeLocalDerivatives(localForce);
  const auto coneDerivatives = computeConeConstraintDerivatives(coneLocalDerivatives, localForceDerivatives);

  LinearApproximation_t linearApproximation;
  linearApproximation.constraintValues.emplace_back(coneConstraint(localForce));
  linearApproximation.derivativeState.emplace_back(frictionConeStateDerivative(coneDerivatives));
  linearApproximation.derivativeInput.emplace_back(frictionConeInputDerivative(coneDerivatives));
  return linearApproximation;
}

FrictionConeConstraint::QuadraticApproximation_t FrictionConeConstraint::getQuadraticApproximation(scalar_t time,
                                                                                                   const state_vector_t& state,
                                                                                                   const input_vector_t& input) const {
  const vector3_t eulerXYZ = getOrientation(getComPose(state));
  const vector3_t forcesInBodyFrame = input.template segment<3>(3 * legNumber_);

  const auto localForce = computeLocalForces(eulerXYZ, forcesInBodyFrame);
  const auto localForceDerivatives = computeLocalForceDerivatives(eulerXYZ, forcesInBodyFrame);
  const auto coneLocalDerivatives = computeConeLocalDerivatives(localForce);
  const auto coneDerivatives = computeConeConstraintDerivatives(coneLocalDerivatives, localForceDerivatives);

  QuadraticApproximation_t quadraticApproximation;
  quadraticApproximation.constraintValues.emplace_back(coneConstraint(localForce));
  quadraticApproximation.derivativeState.emplace_back(frictionConeStateDerivative(coneDerivatives));
  quadraticApproximation.derivativeInput.emplace_back(frictionConeInputDerivative(coneDerivatives));
  quadraticApproximation.secondDerivativesState.emplace_back(frictionConeSecondDerivativeState(coneDerivatives));
  quadraticApproximation.secondDerivativesInput.emplace_back(frictionConeSecondDerivativeInput(coneDerivatives));
  quadraticApproximation.derivativesInputState.emplace_back(frictionConeDerivativesInputState(coneDerivatives));
  return quadraticApproximation;
}
FrictionConeConstraint::LocalForceDerivatives FrictionConeConstraint::computeLocalForceDerivatives(
    const vector3_t& eulerXYZ, const vector3_t& forcesInBodyFrame) const {
  LocalForceDerivatives localForceDerivatives{};
  localForceDerivatives.dF_deuler = t_R_w * rotationBaseToOriginJacobian(eulerXYZ, forcesInBodyFrame);
  localForceDerivatives.dF_du = t_R_w * rotationMatrixBaseToOrigin(eulerXYZ);
  return localForceDerivatives;
}

FrictionConeConstraint::LocalForces FrictionConeConstraint::computeLocalForces(const vector3_t& eulerXYZ,
                                                                               const vector3_t& forcesInBodyFrame) const {
  matrix3_t t_R_b = t_R_w * rotationMatrixBaseToOrigin(eulerXYZ);
  return t_R_b * forcesInBodyFrame;
}

FrictionConeConstraint::ConeLocalDerivatives FrictionConeConstraint::computeConeLocalDerivatives(const LocalForces& localForces) const {
  const auto F_tangent = localForces.x() * localForces.x() + localForces.y() * localForces.y() + regularization_;
  const auto F_norm = sqrt(F_tangent);
  const auto F_norm32 = F_tangent * F_norm;

  ConeLocalDerivatives coneDerivatives{};
  coneDerivatives.dCone_dF(0) = -localForces.x() / F_norm;
  coneDerivatives.dCone_dF(1) = -localForces.y() / F_norm;
  coneDerivatives.dCone_dF(2) = frictionCoefficient_;

  coneDerivatives.d2Cone_dF2(0, 0) = -(localForces.y() * localForces.y() + regularization_) / F_norm32;
  coneDerivatives.d2Cone_dF2(0, 1) = localForces.x() * localForces.y() / F_norm32;
  coneDerivatives.d2Cone_dF2(0, 2) = 0.0;
  coneDerivatives.d2Cone_dF2(1, 0) = coneDerivatives.d2Cone_dF2(0, 1);
  coneDerivatives.d2Cone_dF2(1, 1) = -(localForces.x() * localForces.x() + regularization_) / F_norm32;
  coneDerivatives.d2Cone_dF2(1, 2) = 0.0;
  coneDerivatives.d2Cone_dF2(2, 0) = 0.0;
  coneDerivatives.d2Cone_dF2(2, 1) = 0.0;
  coneDerivatives.d2Cone_dF2(2, 2) = 0.0;

  return coneDerivatives;
}

scalar_t FrictionConeConstraint::coneConstraint(const LocalForces& localForces) const {
  const auto F_tangent = localForces.x() * localForces.x() + localForces.y() * localForces.y() + regularization_;
  const auto F_norm = sqrt(F_tangent);

  return frictionCoefficient_ * (localForces.z() + gripperForce_) - F_norm;
}

FrictionConeConstraint::ConeDerivatives FrictionConeConstraint::computeConeConstraintDerivatives(
    const ConeLocalDerivatives& coneLocalDerivatives, const LocalForceDerivatives& localForceDerivatives) const {
  ConeDerivatives coneDerivatives;
  // First order derivatives
  coneDerivatives.dCone_deuler = coneLocalDerivatives.dCone_dF.transpose() * localForceDerivatives.dF_deuler;
  coneDerivatives.dCone_du = coneLocalDerivatives.dCone_dF.transpose() * localForceDerivatives.dF_du;

  // Second order derivatives
  coneDerivatives.d2Cone_du2 = localForceDerivatives.dF_du.transpose() * coneLocalDerivatives.d2Cone_dF2 * localForceDerivatives.dF_du;
  coneDerivatives.d2Cone_deuler2 =
      localForceDerivatives.dF_deuler.transpose() * coneLocalDerivatives.d2Cone_dF2 * localForceDerivatives.dF_deuler;
  coneDerivatives.d2Cone_dudeuler =
      localForceDerivatives.dF_du.transpose() * coneLocalDerivatives.d2Cone_dF2 * localForceDerivatives.dF_deuler;

  return coneDerivatives;
}

FrictionConeConstraint::state_vector_t FrictionConeConstraint::frictionConeStateDerivative(const ConeDerivatives& coneDerivatives) const {
  state_vector_t dhdx;
  dhdx.setZero();
  dhdx.segment<3>(0) = coneDerivatives.dCone_deuler;
  return dhdx;
}

FrictionConeConstraint::input_vector_t FrictionConeConstraint::frictionConeInputDerivative(const ConeDerivatives& coneDerivatives) const {
  input_vector_t dhdu;
  dhdu.setZero();
  dhdu.segment<3>(3 * legNumber_) = coneDerivatives.dCone_du;
  return dhdu;
}

FrictionConeConstraint::input_matrix_t FrictionConeConstraint::frictionConeSecondDerivativeInput(
    const ConeDerivatives& coneDerivatives) const {
  input_matrix_t ddhdudu;
  ddhdudu.setZero();
  ddhdudu.block<3, 3>(3 * legNumber_, 3 * legNumber_) = coneDerivatives.d2Cone_du2;
  return ddhdudu;
}

FrictionConeConstraint::state_matrix_t FrictionConeConstraint::frictionConeSecondDerivativeState(
    const ConeDerivatives& coneDerivatives) const {
  input_matrix_t ddhdxdx;
  ddhdxdx.setZero();
  ddhdxdx.block<3, 3>(0, 0) = coneDerivatives.d2Cone_deuler2;
  return ddhdxdx;
}

FrictionConeConstraint::input_state_matrix_t FrictionConeConstraint::frictionConeDerivativesInputState(
    const ConeDerivatives& coneDerivatives) const {
  input_state_matrix_t ddhdudx;
  ddhdudx.setZero();
  ddhdudx.block<3, 3>(3 * legNumber_, 0) = coneDerivatives.d2Cone_dudeuler;
  return ddhdudx;
}

}  // namespace switched_model
