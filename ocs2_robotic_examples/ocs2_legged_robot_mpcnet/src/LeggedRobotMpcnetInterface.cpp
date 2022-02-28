#include "ocs2_legged_robot_mpcnet/LeggedRobotMpcnetInterface.h"

#include <ros/package.h>

#include <ocs2_mpc/MPC_DDP.h>
#include <ocs2_mpcnet/control/MpcnetOnnxController.h>
#include <ocs2_oc/rollout/TimeTriggeredRollout.h>
#include <ocs2_oc/synchronized_module/ReferenceManager.h>
#include <ocs2_raisim_core/RaisimRollout.h>
#include <ocs2_raisim_core/RaisimRolloutSettings.h>

#include "ocs2_legged_robot_mpcnet/LeggedRobotMpcnetDefinition.h"

namespace ocs2 {
namespace legged_robot {

LeggedRobotMpcnetInterface::LeggedRobotMpcnetInterface(size_t nDataGenerationThreads, size_t nPolicyEvaluationThreads, bool raisim) {
  // create ONNX environment
  auto onnxEnvironmentPtr = createOnnxEnvironment();
  // paths to files
  std::string taskFile = ros::package::getPath("ocs2_legged_robot") + "/config/mpc/task.info";
  std::string urdfFile = ros::package::getPath("ocs2_robotic_assets") + "/resources/anymal_c/urdf/anymal.urdf";
  std::string referenceFile = ros::package::getPath("ocs2_legged_robot") + "/config/command/reference.info";
  std::string raisimFile = ros::package::getPath("ocs2_legged_robot_raisim") + "/config/raisim.info";
  std::string resourcePath = ros::package::getPath("ocs2_robotic_assets") + "/resources/anymal_c/meshes";
  // set up MPC-Net rollout manager for data generation and policy evaluation
  std::vector<std::unique_ptr<MPC_BASE>> mpcPtrs;
  std::vector<std::unique_ptr<MpcnetControllerBase>> mpcnetPtrs;
  std::vector<std::unique_ptr<RolloutBase>> rolloutPtrs;
  std::vector<std::shared_ptr<MpcnetDefinitionBase>> mpcnetDefinitionPtrs;
  std::vector<std::shared_ptr<ReferenceManagerInterface>> referenceManagerPtrs;
  mpcPtrs.reserve(nDataGenerationThreads + nPolicyEvaluationThreads);
  mpcnetPtrs.reserve(nDataGenerationThreads + nPolicyEvaluationThreads);
  rolloutPtrs.reserve(nDataGenerationThreads + nPolicyEvaluationThreads);
  mpcnetDefinitionPtrs.reserve(nDataGenerationThreads + nPolicyEvaluationThreads);
  referenceManagerPtrs.reserve(nDataGenerationThreads + nPolicyEvaluationThreads);
  for (int i = 0; i < (nDataGenerationThreads + nPolicyEvaluationThreads); i++) {
    leggedRobotInterfacePtrs_.push_back(std::unique_ptr<LeggedRobotInterface>(new LeggedRobotInterface(taskFile, urdfFile, referenceFile)));
    std::shared_ptr<MpcnetDefinitionBase> mpcnetDefinitionPtr(
        new LeggedRobotMpcnetDefinition(leggedRobotInterfacePtrs_[i]->getInitialState()));
    mpcPtrs.push_back(getMpc(*leggedRobotInterfacePtrs_[i]));
    mpcnetPtrs.push_back(std::unique_ptr<MpcnetControllerBase>(
        new MpcnetOnnxController(mpcnetDefinitionPtr, leggedRobotInterfacePtrs_[i]->getReferenceManagerPtr(), onnxEnvironmentPtr)));
    if (raisim) {
      RaisimRolloutSettings raisimRolloutSettings(raisimFile, "rollout");
      raisimRolloutSettings.portNumber_ += i;
      leggedRobotRaisimConversionsPtrs_.push_back(std::unique_ptr<LeggedRobotRaisimConversions>(new LeggedRobotRaisimConversions(
          leggedRobotInterfacePtrs_[i]->getPinocchioInterface(), leggedRobotInterfacePtrs_[i]->getCentroidalModelInfo(),
          leggedRobotInterfacePtrs_[i]->getInitialState())));
      leggedRobotRaisimConversionsPtrs_[i]->loadSettings(raisimFile, "rollout", true);
      rolloutPtrs.push_back(std::unique_ptr<RolloutBase>(new RaisimRollout(
          urdfFile, resourcePath,
          [&, i](const vector_t& state, const vector_t& input) {
            return leggedRobotRaisimConversionsPtrs_[i]->stateToRaisimGenCoordGenVel(state, input);
          },
          [&, i](const Eigen::VectorXd& q, const Eigen::VectorXd& dq) {
            return leggedRobotRaisimConversionsPtrs_[i]->raisimGenCoordGenVelToState(q, dq);
          },
          [&, i](double time, const vector_t& input, const vector_t& state, const Eigen::VectorXd& q, const Eigen::VectorXd& dq) {
            return leggedRobotRaisimConversionsPtrs_[i]->inputToRaisimGeneralizedForce(time, input, state, q, dq);
          },
          nullptr, raisimRolloutSettings,
          [&, i](double time, const vector_t& input, const vector_t& state, const Eigen::VectorXd& q, const Eigen::VectorXd& dq) {
            return leggedRobotRaisimConversionsPtrs_[i]->inputToRaisimPdTargets(time, input, state, q, dq);
          })));
    } else {
      rolloutPtrs.push_back(std::unique_ptr<RolloutBase>(leggedRobotInterfacePtrs_[i]->getRollout().clone()));
    }
    mpcnetDefinitionPtrs.push_back(mpcnetDefinitionPtr);
    referenceManagerPtrs.push_back(leggedRobotInterfacePtrs_[i]->getReferenceManagerPtr());
  }
  mpcnetRolloutManagerPtr_.reset(new MpcnetRolloutManager(nDataGenerationThreads, nPolicyEvaluationThreads, std::move(mpcPtrs),
                                                          std::move(mpcnetPtrs), std::move(rolloutPtrs), mpcnetDefinitionPtrs,
                                                          referenceManagerPtrs));
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
std::unique_ptr<MPC_BASE> LeggedRobotMpcnetInterface::getMpc(LeggedRobotInterface& leggedRobotInterface) {
  std::unique_ptr<MPC_BASE> mpcPtr(new MPC_DDP(leggedRobotInterface.mpcSettings(), leggedRobotInterface.ddpSettings(),
                                               leggedRobotInterface.getRollout(), leggedRobotInterface.getOptimalControlProblem(),
                                               leggedRobotInterface.getInitializer()));
  mpcPtr->getSolverPtr()->setReferenceManager(leggedRobotInterface.getReferenceManagerPtr());
  return mpcPtr;
}

}  // namespace legged_robot
}  // namespace ocs2
