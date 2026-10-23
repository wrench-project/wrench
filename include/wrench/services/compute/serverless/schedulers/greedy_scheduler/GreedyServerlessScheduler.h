#ifndef WRENCH_GREEDYSERVERLESSCHEDULER_H
#define WRENCH_GREEDYSERVERLESSCHEDULER_H

#include <wrench/services/compute/serverless/schedulers/ServerlessScheduler.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>


namespace wrench {
    class ServerlessReusableContainerSelectionPolicy;
    class ServerlessInvocationFailingPolicy;
    class ServerlessContainerPrewarmPolicy;
    class ServerlessPlanSelectionPolicy;
    class ServerlessEvictionPolicy;
    class ServerlessNodeSelectionPolicy;
    class ServerlessSchedulingState;
    class ServerlessNodeActivationPolicy;
    class ServerlessInvocationOrderingPolicy;


     /**
     * @brief Policy-configurable serverless scheduler with greedy placement,
     *        resource reclamation, and image-preparation planning.
     *
     * @details
     * This scheduler combines policies for invocation failure, worker activation,
     * invocation ordering, node eligibility, reusable-container selection,
     * eviction, plan selection, and prewarming. Default implementations are
     * installed by the constructor and can be replaced with setPolicy().
     *
     * Policies operate within a fixed orchestration and placement algorithm.
     * Invocations are considered sequentially; each selected plan changes the
     * working resources available to later invocations. Earlier choices are not
     * revisited to optimize the complete batch. Policy replacement therefore does
     * not change the precedence of the scheduling phases or placement passes.
     *
     * @par Scheduling round
     * Each schedule() call constructs a fresh ServerlessSchedulingState and:
     *
     * 1. Records the failure policy's decisions and removes those invocations
     *    from the local worklist.
     * 2. Requests the number of additional workers selected by the activation
     *    policy. Requested workers do not become usable within this round.
     * 3. Orders the surviving worklist in place using the ordering policy.
     * 4. Considers each invocation, selects a dispatch or image-preparation plan
     *    when possible, and applies the selected plan's accounting changes to
     *    the working state. An invocation for which no plan is selected remains
     *    pending; it does not prevent consideration of later invocations.
     * 5. Evaluates prewarming against the post-placement working state and
     *    appends the resulting container-creation requests.
     * 6. Collects the installed policies' requested wakeup dates.
     *
     * Failure selection, worker activation, ordering, prewarming, and wakeup
     * collection are outside the per-invocation placement loop. They are evaluated
     * even when the worklist is empty. Node selection, reusable-container
     * selection, eviction, and plan selection are evaluated as placement needs
     * them. Invocation ordering is not strict head-of-line blocking.
     *
     * @par Placement precedence
     * For an invocation, the node-selection policy supplies an eligibility set,
     * not a ranking. The scheduler retains selected, already-activated nodes with
     * remaining node-wide invocation capacity, preserving their traversal order.
     * It then tries these passes in order:
     *
     * 1. Ask the reusable-container policy for an initialized, compatible
     *    container with remaining invocation capacity. The container need not
     *    be idle: an occupied container may accept more work up to its limit.
     * 2. Construct cold-dispatch alternatives on nodes where every required
     *    image layer is resident on disk and in RAM. Reclamation may be needed
     *    for private RAM, private disk space, or a container position.
     * 3. Select the first node where some required RAM residency is still
     *    missing, but every missing layer is already being loaded.
     * 4. Construct alternatives that initiate the remaining RAM loads on nodes
     *    where every required layer is already on disk.
     * 5. Select the first node where some required disk residency is still
     *    missing, but every missing layer is already being copied.
     * 6. Construct alternatives that initiate the remaining disk copies.
     *
     * The first pass that selects a node determines the outcome for this
     * invocation. The plan-selection policy compares alternatives within the
     * cold-dispatch, RAM-load, and disk-copy passes; it does not compare plans
     * across readiness levels or override the reusable-container decision.
     *
     * @par Working state and capacity accounting
     * The worklist is a copy of invocation references, not Invocation objects.
     * Filtering and ordering it do not change the service's pending collection.
     * Runtime containers and invocations remain shared objects; planning must
     * not mutate them to simulate application of a returned decision.
     *
     * The working state distinguishes three count constraints: remaining
     * invocations per node, remaining container positions per node, and remaining
     * invocations per existing initialized container. These counts are separate
     * from RAM/disk availability and physical CPU capacity.
     *
     * A selected dispatch consumes one unit of node invocation capacity. Reuse
     * also consumes one unit of the existing container's invocation capacity.
     * Cold dispatch instead consumes a container position and its private RAM
     * and disk allocations. A selected idle-container termination returns its
     * private resources and container position and removes the container from
     * the working collection. Selected transfers reserve their destination
     * space and mark their layers as in progress, not resident.
     *
     * Alternative plans are evaluated before committing their changes to the
     * working state. Only the chosen alternative's changes constrain subsequent
     * placement. This bookkeeping is local to the round: it is not a persistent
     * service-side reservation or invocation-to-node binding.
     *
     * In particular, selecting a node to wait for image preparation does not
     * consume invocation capacity or reserve a future container position. It
     * protects the invocation's required layers on that node for the remainder
     * of this round but emits no operation. Starting a transfer likewise does
     * not reserve all resources needed for a later container. Future rounds
     * reassess feasibility from the observed system state.
     *
     * @par Reclamation
     * Reclamation considers idle containers first, then RAM layers, then disk
     * layers. The eviction policy selects victims from eligible candidates.
     * Required resources are compared with currently available resources to
     * obtain deficits; needing one new container does not necessarily require
     * terminating an existing container. Only container termination can free a
     * container position; layer eviction addresses byte deficits.
     *
     * Layers needed by the invocation, earlier chosen plans, ongoing transfers,
     * or containers retained by the tentative plan are excluded as appropriate.
     * Evicting a disk layer also requires removal of its RAM copy when present.
     * Reclamation is feasible only if it satisfies the required byte deficits
     * and frees enough container positions.
     *
     * @par Prewarming
     * Prewarming is considered after current requests and their image-preparation
     * plans. The policy must select a jointly feasible batch using the remaining
     * container positions, RAM, and disk, with required layers already resident
     * on activated workers. This phase does not plan additional evictions or
     * transfers to make its proposals feasible.
     *
     * The scheduler appends the proposals and accounts for their container
     * positions; it does not perform another RAM/disk planning pass over them.
     * Each proposal creates one environment, so repeated function/node pairs
     * request distinct containers. Prewarming has no assigned client invocation
     * and does not consume invocation-concurrency capacity, but retains its
     * container resources. Its lower precedence applies to this scheduling
     * round, not to requests that arrive after prewarming has started.
     *
     * @note The working state is not a complete projected runtime state. The
     * invocation worklist still contains requests selected for dispatch, and
     * newly planned containers do not yet exist as selectable Container objects.
     * Policies must not infer the undispatched backlog from worklist size alone
     * or treat the container collections as a complete post-batch inventory.
     *
     * @par Wakeups and execution
     * After policy evaluation, the scheduler takes the minimum reported wakeup
     * date. DBL_MAX means no request; if all policies return it, the output's
     * optional wakeup date remains unset. Other dates must be finite absolute
     * simulated dates strictly later than the current time. Maintaining valid,
     * nonstale deadlines is part of each policy's contract; this aggregation
     * does not itself validate every contribution.
     *
     * The result describes operations for ServerlessComputeService to apply.
     * schedule() neither executes them nor waits for completion. The service
     * applies decision categories in its own order and performs its runtime
     * checks. The batch is not an atomic transaction. Dependencies on operation
     * completion are handled through later scheduling rounds, not by assuming
     * that a requested copy, load, initialization, or activation is complete.
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
        [[nodiscard]] ServerlessReusableContainerSelectionPolicy* getReusableContainerSelectionPolicy() const;

         void setPolicy(std::shared_ptr<ServerlessNodeActivationPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessInvocationOrderingPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessNodeSelectionPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessEvictionPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessPlanSelectionPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessContainerPrewarmPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessInvocationFailingPolicy> policy);
         void setPolicy(std::shared_ptr<ServerlessReusableContainerSelectionPolicy> policy);

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
            unsigned long num_needed_containers,
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
        /** @brief The reusable container selection policy **/
        std::shared_ptr<ServerlessReusableContainerSelectionPolicy> _reusable_container_selection_policy;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
} // namespace wrench

#endif //WRENCH_GREEDYSERVERLESSCHEDULER_H
