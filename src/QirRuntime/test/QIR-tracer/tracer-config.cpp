// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// TODO: ideally, this file should be generated by the Q# compiler alongside the qir, using the mappings specified in
// target.qs.

#include <memory>

#include "QuantumApi_I.hpp"
#include "tracer-config.hpp"

namespace TracerUser
{
const std::unordered_map<OpId, std::string> g_operationNames = {{0, "X"}, {1, "CX"}, {2, "MCX"},
                                                                {3, "Y"}, {4, "CY"}, {5, "MCY"} /*etc.*/};
}

namespace Microsoft
{
namespace Quantum
{

    std::unique_ptr<ISimulator> CreateFullstateSimulator()
    {
        throw std::logic_error("Tracer should not instantiate full state simulator");
    }

} // namespace Quantum
} // namespace Microsoft