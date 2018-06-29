#include "wrench/simulation/SimulationTimestampTypes.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"

namespace wrench {

    /**
     * @brief Constructor
     */
    SimulationTimestampType::SimulationTimestampType() {
        this->date = S4U_Simulation::getClock();
    }

    /**
     * @brief Retrieve the date recorded for this timestamp
     * @return the date of this timestamp
     */
    double SimulationTimestampType::getDate() {
        return this->date;
    }

    /**
     * @brief Retrieve the corresponding start timestamp if this is an end(failure or completion) timestamp or vice versa.
     * @return a pointer to the corresponding timestamp object
     */
    SimulationTimestampType *SimulationTimestampType::getEndpoint() {
        return this->endpoint;
    }

    /**
     * @brief Constructor
     * @param task: a pointer to the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTask::SimulationTimestampTask(WorkflowTask *task) : task(task) {

    }

    /**
     * @brief Retrieves the WorkflowTask associated with this timestamp
     * @return a pointer to the WorkflowTask associated with this timestamp
     */
    WorkflowTask *SimulationTimestampTask::getTask() {
        return this->task;
    }

    /**
     * @brief A static map of SimulationTimestampTaskStart objects that have yet to matched with SimulationTimestampTaskFailure or SimulationTimestampTaskCompletion timestamps
     */
    std::map<std::string, SimulationTimestampTask *> SimulationTimestampTask::pending_task_timestamps;

    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampTaskFailure or SimulationTimestampTaskStart) with a SimulationTimestampTaskStart object
     */
    void SimulationTimestampTask::setEndpoints() {
        // find the SimulationTimestampTaskStart object containing the same task
        auto pending_tasks_itr = pending_task_timestamps.find(this->task->getID());
        if (pending_tasks_itr != pending_task_timestamps.end()) {
            // set the SimulationTimestampTaskStart's endpoint to me
            (*pending_tasks_itr).second->endpoint = this;

            // set my endpoint to the SimulationTimestampTaskStart
            this->endpoint = (*pending_tasks_itr).second->endpoint;

            // the SimulationTimestampTaskStart is no longer waiting to be matched with an end timestamp, remove it from the map
            pending_task_timestamps.erase(pending_tasks_itr);
        }
    }

    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskStart::SimulationTimestampTaskStart(WorkflowTask *task) : SimulationTimestampTask(task) {
        /*
         * Upon creation, this object adds a pointer of itself to the 'pending_task_timestamps' map so that it's endpoint can
         * be set when a SimulationTimestampTaskFailure or SimulationTimestampTaskCompletion is created
         */
        pending_task_timestamps.insert(std::make_pair(task->getID(), this));
    }


    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskFailure::SimulationTimestampTaskFailure(WorkflowTask *task) : SimulationTimestampTask(task) {
        // match this timestamp with a SimulationTimestampTaskStart
        if (task != nullptr) {
            setEndpoints();
        }
    }

    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskCompletion::SimulationTimestampTaskCompletion(WorkflowTask *task) : SimulationTimestampTask(
            task) {
        // match this timestamp with a SimulationTimestampTaskStart
        if (task != nullptr) {
            setEndpoints();
        }
    }
}

