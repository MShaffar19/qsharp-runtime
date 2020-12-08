﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;
using Microsoft.Quantum.Simulation.Core;

namespace Microsoft.Quantum.Simulation.Simulators
{
    public partial class ToffoliSimulator
    {
        /// <summary>
        /// The implementation of the operation.
        /// For the Toffoli simulator, the implementation flips the target qubit
        /// if the rotation is effectively an X gate.
        /// </summary>
        public void R_Body(Pauli pauli, double angle, Qubit target)
        {
            if (target == null) return;

            this.CheckQubit(target, "target");

            var (isX, safe) = CheckRotation(pauli, angle / 2.0);
            if (isX)
            {
                this.DoX(target);
            }
        }

        /// <summary>
        /// The implementation of the adjoint specialization of the operation.
        /// For the Toffoli simulator *only*, this operation is self-adjoint.
        /// </summary>
        public void R_AdjointBody(Pauli pauli, double angle, Qubit target) => R_Body(pauli, angle, target);

        /// <summary>
        /// The implementation of the controlled specialization of the operation.
        /// For the Toffoli simulator, the implementation flips the target qubit
        /// if the rotation is effectively an X gate and all of the control qubits
        /// are in the One state.
        /// </summary>
        public void R_ControlledBody(IQArray<Qubit> controls, Pauli pauli, double angle, Qubit target)
        {
            if (target == null) return;

            this.CheckControlQubits(controls, target);

            var (isX, safe) = CheckRotation(pauli, angle / 2.0);
            if (!safe)
            {
                throw new InvalidOperationException($"The Toffoli simulator can only perform controlled rotations of multiples of 2*pi.");
            }
            // We never need to do anything since only the identity is safe to control
        }

        /// <summary>
        /// The implementation of the controlled adjoint specialization of the operation.
        /// For the Toffoli simulator *only*, the controlled specialization is self-adjoint.
        /// </summary>
        public void R_ControlledAdjointBody(IQArray<Qubit> controls, Pauli pauli, double angle, Qubit target) => R_ControlledBody(controls, pauli, angle, target);
    }
}
