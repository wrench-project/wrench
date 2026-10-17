#ifndef WRENCH_GREEDYSERVERLESSCHEDULER_H
#define WRENCH_GREEDYSERVERLESSCHEDULER_H

#include <wrench/services/compute/serverless/schedulers/ServerlessScheduler.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>


namespace wrench {
    class ServerlessIdleContainerSelectionPolicy;
    class ServerlessInvocationFailingPolicy;
    class ServerlessContainerPrewarmPolicy;
    class ServerlessPlanSelectionPolicy;
    class ServerlessEvictionPolicy;
    class ServerlessNodeSelectionPolicy;
    class ServerlessSchedulingState;
    class ServerlessNodeActivationPolicy;
    class ServerlessInvocationOrderingPolicy;


    /**
     * @brief Configurable serverless scheduler with greedy invocation placement
     *        and image-preparation planning.
     *
     * @details
     * This scheduler combines replaceable policies within a fixed planning
     * algorithm. Policies control invocation failure, compute-node activation,
     * invocation ordering, node eligibility, resource eviction, plan selection,
     * and container pre-warming. The constructor installs default policies;
     * individual policy categories can be replaced using setPolicy().
     *
     * The algorithm is greedy in how it constructs a plan: invocations are
     * considered sequentially, earlier choices constrain later choices, and
     * accepted choices are not revisited to optimize the batch globally.
     * Replacing a policy changes decisions within this structure, but does not
     * change the ordering of the scheduling phases or the placement passes.
     *
     * @par Planning state
     * Each call to schedule() creates a fresh ServerlessSchedulingState containing
     * a mutable invocation worklist and resource-planning data. The worklist
     * copies shared pointers, not Invocation objects. Filtering and ordering it
     * therefore do not modify the compute service's invocation collection.
     *
     * As invocation plans are selected, the scheduler updates tentative slot,
     * RAM, disk, container-availability, and image-layer bookkeeping. Subsequent
     * decisions use this updated planning state rather than independently
     * consuming resources from the original snapshot. The planning state is not
     * a deep copy of the runtime objects and must not be used to modify those
     * objects in place of returning scheduling decisions.
     *
     * @par Scheduling phases
     * A scheduling round proceeds in the following order:
     *
     * 1. Invoke the invocation-failing policy. Record its failure decisions and
     *    remove the selected invocations from the working invocation list, so
     *    subsequent policies do not plan work for them.
     *
     * 2. Invoke the node-activation policy and record the requested number of
     *    additional compute nodes. Requested activations do not add usable
     *    capacity to the current planning state; the compute service handles
     *    activation and its completion separately.
     *
     * 3. Invoke the invocation-ordering policy to reorder the remaining
     *    invocation worklist in place.
     *
     * 4. Consider invocations in that order. For each invocation, attempt to
     *    select a compute node and an execution or image-preparation plan.
     *    Record the selected decisions and update the planning state. If no
     *    node is selected, leave the invocation pending and consider the next
     *    invocation. Stop this phase when all planning slots have been claimed.
     *    Invocation ordering is therefore a consideration order, not a strict
     *    head-of-line blocking discipline.
     *
     * 5. Invoke the container-prewarming policy using the post-placement
     *    planning state, and append its requests to the returned decisions.
     *    This gives current invocations and their preparation plans precedence
     *    over new prewarming requests within this scheduling round.
     *
     * 6. Query every installed policy's getNextWakeupDate() and return the
     *    earliest requested date, unless all policies return DBL_MAX.
     *
     * Failure selection, activation, ordering, prewarming, and wakeup collection
     * are outside the invocation-placement loop. An empty invocation worklist or
     * a lack of available slots does not skip these phases. Node selection,
     * eviction, and plan selection are invoked as needed during placement.
     *
     * @par Invocation placement
     * For each invocation, the node-selection policy supplies an eligibility
     * set, not a preference ranking. The scheduler considers only selected nodes
     * that are already activated and have an available planning slot, preserving
     * their order in the planning state's activated-node collection.
     *
     * Candidate nodes are examined in the following sequence of passes:
     *
     * 1. Reuse an idle container associated with the same Function.
     * 2. Plan creation of a new container where all required layers are already
     *    resident on disk and in RAM, with resource reclamation if necessary.
     * 3. Wait for RAM preparation where every required layer is either resident
     *    in RAM or already on its way there.
     * 4. Plan the missing RAM loads where all required layers are on disk,
     *    with resource reclamation if necessary.
     * 5. Wait for disk preparation where every required layer is either on disk
     *    or already on its way there.
     * 6. Plan the missing disk copies, with resource reclamation if necessary.
     *
     * The first pass that selects a node determines the invocation's plan.
     * The warm-reuse and wait-for-preparation passes select an idle container
     * based on an idle-container-selection policy.
     * The new-container, RAM-load, and disk-copy passes generate candidate plans
     * and delegate their selection to the plan-selection policy. That policy
     * compares alternatives within those passes; it does not override the pass
     * ordering or compare every possible action across all readiness levels.
     *
     * When reclamation is necessary, the planner considers idle-container
     * termination, then RAM-layer eviction, then disk-layer eviction. The
     * eviction policy selects victims from the eligible candidates. Layers
     * needed by retained containers, current preparation, or earlier selected
     * plans are excluded as appropriate.
     *
     * @par Tentative reservations and asynchronous preparation
     * Selecting a target node claims one slot in the planning state, even when
     * the invocation cannot yet be dispatched because image preparation is
     * pending. Its required image layers are also protected against subsequent
     * eviction planning on that node.
     *
     * Such a slot claim applies only to this scheduling round. It is not a
     * compute-service reservation or a persistent invocation-to-node binding.
     * A later round constructs a new planning state from the observed system.
     *
     * Requested copies and loads are recorded as in progress, not as completed.
     * Starting preparation therefore does not make an invocation immediately
     * dispatchable. Likewise, a RAM-load or disk-copy plan is not a reservation
     * of every resource that a future container may require. Later rounds
     * reassess placement and resource feasibility after the system changes.
     *
     * @par Prewarming policy contract
     * Prewarming uses the resources left by invocation planning, including its
     * tentative reservations for invocations awaiting image preparation.
     *
     * The prewarming policy is responsible for returning requests that
     * collectively fit the remaining slot, RAM, and disk budgets on activated
     * nodes. All required image layers must already be resident on disk and in
     * RAM in the supplied planning state. Layers merely being copied or loaded
     * do not satisfy this requirement.
     *
     * This phase does not generate additional eviction, copy, or load plans to
     * make a proposed prewarm feasible. The returned requests are appended
     * without another resource-planning pass. Each request denotes creation of
     * one additional container; repeated requests for the same function/node
     * pair denote distinct container creations and must be budgeted accordingly.
     *
     * Prewarming is lower priority only when constructing the current batch.
     * Once started by the service, a prewarm occupies resources and may affect
     * invocations arriving in later scheduling rounds.
     *
     * @note The post-placement planning state is not a complete projected
     * runtime state. In particular, the invocation worklist still includes
     * invocations selected for dispatch, and planned new containers have not
     * been materialized as Container objects. Policies must not interpret the
     * worklist size as the number of invocations left without a dispatch plan,
     * or the container collections as a complete post-execution inventory.
     *
     * @par Wakeup requests
     * Policy wakeup dates are absolute simulated dates. DBL_MAX means that a
     * policy requests no timer-driven scheduling round. Other dates must be
     * finite and strictly in the future. Policies are responsible for maintaining
     * their deadlines, including retiring deadlines that have been handled.
     *
     * Wakeup dates are collected after the scheduling phases, so they reflect
     * policy evaluations performed during this call. If all policies return
     * DBL_MAX, scheduler_wakeup_date is left unset. Wakeups supplement ordinary
     * event-driven scheduling; they do not postpone the decisions returned now
     * or commit operations to be performed unconditionally at a future date.
     *
     * @par Decision execution
     * schedule() constructs a batch of intentions; it does not execute the
     * requested operations or wait for their completion. ServerlessComputeService
     * applies the batch in its own decision-processing order and checks whether
     * individual operations can be performed. The batch is not an atomic
     * transaction, and tentative planning reservations are not service-side
     * guarantees.
     *
     * Policies that require a different orchestration or placement structure
     * can be implemented through another ServerlessScheduler subclass.
     *
     * @see ServerlessScheduler
     * @see ServerlessSchedulingState
     * @see ServerlessSchedulingDecisions
     * @see ServerlessSchedulingPolicy
     */
    class GreedyServerlessScheduler : public ServerlessScheduler {

    public:

        GreedyServerlessScheduler();

        [[nodiscard]] ServerlessNodeActivationPolicy* getNodeActivationPolicy() const;
        [[nodiscard]] ServerlessInvocationOrderingPolicy* getInvocationOrderingPolicy() const;
        [[nodiscard]] ServerlessNodeSelectionPolicy* getNodeSelectionPolicy() const;
        [[nodiscard]] ServerlessEvictionPolicy* getEvictionPolicy() const;
        [[nodiscard]] ServerlessPlanSelectionPolicy* getPlanSelectionPolicy() const;
        [[nodiscard]] ServerlessContainerPrewarmPolicy* getContainerPrewarmPolicy() const;
        [[nodiscard]] ServerlessInvocationFailingPolicy* getInvocationFailingPolicy() const;
        [[nodiscard]] ServerlessIdleContainerSelectionPolicy* getIdleContainerPolicy() const;

         void setPolicy(std::shared_ptr<ServerlessNodeActivationPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessInvocationOrderingPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessNodeSelectionPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessEvictionPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessPlanSelectionPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessContainerPrewarmPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessInvocationFailingPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessIdleContainerSelectionPolicy> policy);

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~GreedyServerlessScheduler() override = default;

        std::shared_ptr<ServerlessSchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
            const ServerlessStateOfTheSystem* state) override;

    protected:
        std::pair<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>
        pickComputeNode(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            const std::shared_ptr<Invocation>& inv);

        [[nodiscard]] std::shared_ptr<ServerlessSchedulingDecisions> makeEvictionPlan(
            const std::shared_ptr<ServerlessComputeNode>& node,
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            sg_size_t needed_ram,
            sg_size_t needed_disk,
            const std::set<std::shared_ptr<ImageLayer>>& invocation_layers_to_protect) const;


        /** @brief The node activation policy **/
        std::shared_ptr<ServerlessNodeActivationPolicy> _node_activation_policy;
	    /** @brief The invocation ordering policy **/
        std::shared_ptr<ServerlessInvocationOrderingPolicy> _invocation_ordering_policy;
        /** @brief The node selection policy **/
        std::shared_ptr<ServerlessNodeSelectionPolicy> _node_selection_policy;
	    /** @brief The eviction policy **/
        std::shared_ptr<ServerlessEvictionPolicy> _eviction_policy;
	    /** @brief The plan selection policy **/
        std::shared_ptr<ServerlessPlanSelectionPolicy> _plan_selection_policy;
        /** @brief The container prewarm policy **/
        std::shared_ptr<ServerlessContainerPrewarmPolicy> _container_prewarm_policy;
        /** @brief The invocation failing policy **/
        std::shared_ptr<ServerlessInvocationFailingPolicy> _invocation_failing_policy;
        /** @brief The idle container selection policy **/
        std::shared_ptr<ServerlessIdleContainerSelectionPolicy> _idle_container_selection_policy;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
} // namespace wrench

#endif //WRENCH_GREEDYSERVERLESSCHEDULER_H
