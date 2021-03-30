/******************************************************************************
Copyright (c) 2020, Farbod Farshidian. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#pragma once

#include <ocs2_mpc/MPC_BASE.h>

#include <ocs2_sqp/MultipleShootingSolver.h>

namespace ocs2 {
class MultipleShootingMpc : public MPC_BASE {
 public:
  MultipleShootingMpc(mpc::Settings mpcSettings, multiple_shooting::Settings settings, const SystemDynamicsBase* systemDynamicsPtr,
                      const CostFunctionBase* costFunctionPtr, const ConstraintBase* constraintPtr = nullptr,
                      const CostFunctionBase* terminalCostPtr = nullptr,
                      const SystemOperatingTrajectoriesBase* operatingTrajectoriesPtr = nullptr)
      : MPC_BASE(std::move(mpcSettings)) {
    solverPtr_.reset(new MultipleShootingSolver(std::move(settings), systemDynamicsPtr, costFunctionPtr, constraintPtr, terminalCostPtr,
                                                operatingTrajectoriesPtr));
  };

  ~MultipleShootingMpc() override = default;

  MultipleShootingSolver* getSolverPtr() override { return solverPtr_.get(); }
  const MultipleShootingSolver* getSolverPtr() const override { return solverPtr_.get(); }

 protected:
  void calculateController(scalar_t initTime, const vector_t& initState, scalar_t finalTime) override {
    scalar_array_t partitioningTimes = {0.0};
    solverPtr_->run(initTime, initState, finalTime, partitioningTimes);
  }

 private:
  std::unique_ptr<MultipleShootingSolver> solverPtr_;
};
}  // namespace ocs2