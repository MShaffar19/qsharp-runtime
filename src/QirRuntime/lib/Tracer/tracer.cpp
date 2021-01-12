// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>

#include "tracer.hpp"

using namespace std;

namespace Microsoft
{
namespace Quantum
{
    thread_local std::shared_ptr<CTracer> tracer = nullptr;
    std::shared_ptr<CTracer> CreateTracer()
    {
        tracer = std::make_shared<CTracer>();
        return tracer;
    }

    //------------------------------------------------------------------------------------------------------------------
    // CTracer::CreateNewLayer
    //------------------------------------------------------------------------------------------------------------------
    LayerId CTracer::CreateNewLayer(Duration opDuration)
    {
        // Create a new layer for the operation.
        Time layerStartTime = 0;
        if (!this->metricsByLayer.empty())
        {
            const Layer& lastLayer = this->metricsByLayer.back();
            layerStartTime = lastLayer.startTime + lastLayer.duration;
        }
        this->metricsByLayer.push_back(Layer{max(this->preferredLayerDuration, opDuration), layerStartTime});

        return this->metricsByLayer.size() - 1;
    }

    //------------------------------------------------------------------------------------------------------------------
    // CTracer::FindLayerToInsertOperationInto
    //------------------------------------------------------------------------------------------------------------------
    LayerId CTracer::FindLayerToInsertOperationInto(Qubit q, Duration opDuration) const
    {
        const QubitState& qstate = this->UseQubit(q);

        LayerId layerToInsertInto = INVALID;
        if (qstate.layer != INVALID)
        {
            const Layer& lastUsedIn = this->metricsByLayer[qstate.layer];
            if (qstate.lastUsedTime + opDuration <= lastUsedIn.startTime + lastUsedIn.duration)
            {
                layerToInsertInto = qstate.layer;
            }
            else if (opDuration <= this->preferredLayerDuration && qstate.layer + 1 < this->metricsByLayer.size())
            {
                layerToInsertInto = qstate.layer + 1;
            }
        }
        else if (opDuration <= this->preferredLayerDuration && !this->metricsByLayer.empty())
        {
            // the qubit hasn't been used in any of the layers yet -- add it to the first layer
            layerToInsertInto = 0;
        }

        if (layerToInsertInto != INVALID && this->globalBarrier != INVALID)
        {
            if (this->globalBarrier + 1 == this->metricsByLayer.size())
            {
                layerToInsertInto = INVALID;
            }
            else
            {
                layerToInsertInto = std::max(layerToInsertInto, this->globalBarrier + 1);
            }
        }

        return layerToInsertInto;
    }

    //------------------------------------------------------------------------------------------------------------------
    // CTracer::AddOperationToLayer
    //------------------------------------------------------------------------------------------------------------------
    void CTracer::AddOperationToLayer(OpId id, LayerId layer)
    {
        assert(layer < this->metricsByLayer.size());
        auto inserted = this->metricsByLayer[layer].operations.insert({id, 1});
        if (!inserted.second)
        {
            assert(inserted.first->first == id);
            inserted.first->second += 1;
        }
    }

    //------------------------------------------------------------------------------------------------------------------
    // CTracer::UpdateQubitState
    //------------------------------------------------------------------------------------------------------------------
    void CTracer::UpdateQubitState(Qubit q, LayerId layer, Duration opDuration)
    {
        QubitState& qstate = this->UseQubit(q);
        for (OpId idPending : qstate.pendingZeroOps)
        {
            this->AddOperationToLayer(idPending, layer);
        }

        // Update the qubit state.
        qstate.layer = layer;
        const Time layerStart = this->metricsByLayer[layer].startTime;
        qstate.lastUsedTime = max(layerStart, qstate.lastUsedTime) + opDuration;
        qstate.pendingZeroOps.clear();
    }

    //------------------------------------------------------------------------------------------------------------------
    // CTracer::TraceSingleQubitOp
    //------------------------------------------------------------------------------------------------------------------
    void CTracer::TraceSingleQubitOp(OpId id, Duration opDuration, Qubit target)
    {
        QubitState& qstate = this->UseQubit(target);
        if (opDuration == 0 &&
            (qstate.layer == INVALID || (this->globalBarrier != INVALID && qstate.layer < this->globalBarrier)))
        {
            qstate.pendingZeroOps.push_back(id);
            return;
        }

        // Figure out the layer this operation should go into.
        LayerId layerToInsertInto = this->FindLayerToInsertOperationInto(target, opDuration);
        if (layerToInsertInto == INVALID)
        {
            layerToInsertInto = this->CreateNewLayer(opDuration);
        }

        // Add the operation and the pending zero-duration ones into the layer.
        this->AddOperationToLayer(id, layerToInsertInto);
        this->UpdateQubitState(target, layerToInsertInto, opDuration);
    }

    //------------------------------------------------------------------------------------------------------------------
    // CTracer::TraceControlledSingleQubitOp
    //------------------------------------------------------------------------------------------------------------------
    void CTracer::TraceMultiQubitOp(
        OpId id,
        Duration opDuration,
        int64_t nFirstGroup,
        Qubit* firstGroup,
        int64_t nSecondGroup,
        Qubit* secondGroup)
    {
        assert(nFirstGroup >= 0);
        assert(nSecondGroup > 0);

        // Operations that involve a single qubit can special case duration zero.
        if (nFirstGroup == 0 && nSecondGroup == 1)
        {
            this->TraceSingleQubitOp(id, opDuration, secondGroup[0]);
            return;
        }

        // Special-casing operations of duration zero enables potentially better reuse of qubits, when we'll start
        // optimizing for circuit width. However, tracking _the same_ pending operation across _multiple_ qubits is
        // tricky and not worth the effort, so we don't do it.

        // Figure out the layer this operation should go into.
        LayerId layerToInsertInto = this->FindLayerToInsertOperationInto(secondGroup[0], opDuration);
        for (int64_t i = 1; i < nSecondGroup && layerToInsertInto != INVALID; i++)
        {
            layerToInsertInto =
                max(layerToInsertInto, this->FindLayerToInsertOperationInto(secondGroup[i], opDuration));
        }
        for (int64_t i = 0; i < nFirstGroup && layerToInsertInto != INVALID; i++)
        {
            layerToInsertInto = max(layerToInsertInto, this->FindLayerToInsertOperationInto(firstGroup[i], opDuration));
        }
        if (layerToInsertInto == INVALID)
        {
            layerToInsertInto = this->CreateNewLayer(opDuration);
        }

        // Add the operation into the layer.
        this->AddOperationToLayer(id, layerToInsertInto);

        // Update the state of the involved qubits.
        for (int64_t i = 0; i < nFirstGroup; i++)
        {
            this->UpdateQubitState(firstGroup[i], layerToInsertInto, opDuration);
        }
        for (int64_t i = 0; i < nSecondGroup; i++)
        {
            this->UpdateQubitState(secondGroup[i], layerToInsertInto, opDuration);
        }
    }

    void CTracer::InjectGlobalBarrier(char* name, Duration duration)
    {
        LayerId layer = this->CreateNewLayer(duration);
        this->metricsByLayer[layer].name = name;
        this->globalBarrier = layer;
    }
} // namespace Quantum
} // namespace Microsoft