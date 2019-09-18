/*
 * MRT_ROS_Dummy_Quadruped.h
 *
 *  Created on: May 28, 2018
 *      Author: farbod
 */

namespace switched_model {

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
template <size_t JOINT_COORD_SIZE, size_t STATE_DIM, size_t INPUT_DIM>
MRT_ROS_Dummy_Quadruped<JOINT_COORD_SIZE, STATE_DIM, INPUT_DIM>::MRT_ROS_Dummy_Quadruped(
    quadruped_interface_ptr_t ocs2QuadrupedInterfacePtr, scalar_t mrtDesiredFrequency, std::string robotName /*= "robot"*/,
    scalar_t mpcDesiredFrequency /*= -1*/)
    : BASE(mrt_ptr_t(new mrt_t(ocs2QuadrupedInterfacePtr, robotName)), mrtDesiredFrequency, mpcDesiredFrequency,
           &ocs2QuadrupedInterfacePtr->getDynamics(), ocs2QuadrupedInterfacePtr->slqSettings().rolloutSettings_),
      quadrupedXppVisualizer_(std::move(ocs2QuadrupedInterfacePtr), std::move(robotName), true) {}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
template <size_t JOINT_COORD_SIZE, size_t STATE_DIM, size_t INPUT_DIM>
MRT_ROS_Dummy_Quadruped<JOINT_COORD_SIZE, STATE_DIM, INPUT_DIM>::MRT_ROS_Dummy_Quadruped(
    quadruped_interface_ptr_t ocs2QuadrupedInterfacePtr, mrt_ptr_t mrtPtr, scalar_t mrtDesiredFrequency,
    std::string robotName /*= "robot"*/, scalar_t mpcDesiredFrequency /*= -1*/)
    : BASE(mrtPtr, mrtDesiredFrequency, mpcDesiredFrequency, &ocs2QuadrupedInterfacePtr->getDynamics(),
           ocs2QuadrupedInterfacePtr->slqSettings().rolloutSettings_),
      quadrupedXppVisualizer_(std::move(ocs2QuadrupedInterfacePtr), std::move(robotName), true) {}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
template <size_t JOINT_COORD_SIZE, size_t STATE_DIM, size_t INPUT_DIM>
void MRT_ROS_Dummy_Quadruped<JOINT_COORD_SIZE, STATE_DIM, INPUT_DIM>::launchVisualizerNode(int argc, char* argv[]) {
  quadrupedXppVisualizer_.launchVisualizerNode(argc, argv);
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
template <size_t JOINT_COORD_SIZE, size_t STATE_DIM, size_t INPUT_DIM>
void MRT_ROS_Dummy_Quadruped<JOINT_COORD_SIZE, STATE_DIM, INPUT_DIM>::publishVisualizer(const system_observation_t& observation,
                                                                                        const primal_solution_t& primalSolution,
                                                                                        const command_data_t& command) {
  quadrupedXppVisualizer_.publishObservation(observation);
  quadrupedXppVisualizer_.publishDesiredTrajectory(observation.time(), command.mpcCostDesiredTrajectories_);
  quadrupedXppVisualizer_.publishOptimizedStateTrajectory(primalSolution.timeTrajectory_, primalSolution.stateTrajectory_);
}

}  // end of namespace switched_model
