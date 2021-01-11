// Copyright (c) // NOLINT{} Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>

#include "CoreTypes.hpp"
#include "qirTypes.hpp"
#include "tracer.hpp"

namespace Microsoft
{
namespace Quantum
{
    extern thread_local std::shared_ptr<CTracer> tracer;
}
} // namespace Microsoft

using namespace Microsoft::Quantum;
extern "C"
{
    void quantum__trc__on_operation_start(int64_t id) // NOLINT
    {
    }
    void quantum__trc__on_operation_end(int64_t id) // NOLINT
    {
    }

    void quantum__trc__swap(Qubit q1, Qubit q2) // NOLINT
    {
    }

    void quantum__trc__global_barrier(const char* name) // NOLINT
    {
    }

    void quantum__trc__single_qubit_op(int32_t id, int32_t duration, Qubit target) // NOLINT
    {
        tracer->TraceSingleQubitOp(id, duration, target);
    }
    void quantum__trc__single_qubit_op_ctl(int32_t id, int32_t duration, QirArray* ctls, Qubit target) // NOLINT
    {
        tracer->TraceMultiQubitOp(id, duration, ctls->count, reinterpret_cast<Qubit*>(ctls->buffer), 1, &target);
    }
    void quantum__trc__multi_qubit_op(int32_t id, int32_t duration, QirArray* targets) // NOLINT
    {
        tracer->TraceMultiQubitOp(id, duration, 0, nullptr, targets->count, reinterpret_cast<Qubit*>(targets->buffer));
    }
    void quantum__trc__multi_qubit_op_ctl(int32_t id, int32_t duration, QirArray* ctls, QirArray* targets) // NOLINT
    {
        tracer->TraceMultiQubitOp(
            id, duration, ctls->count, reinterpret_cast<Qubit*>(ctls->buffer), targets->count,
            reinterpret_cast<Qubit*>(targets->buffer));
    }
}