#include "wrench/simulation/SimulationTimestampTypes.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"

namespace wrench {

    /*
     * SimulationTimestampType
     */
    SimulationTimestampType::SimulationTimestampType() {
        this->date = S4U_Simulation::getClock();
    }

    double SimulationTimestampType::getDate() {
        return this->date;
    }

    SimulationTimestampType* SimulationTimestampType::getEndpoint() {
        return this->endpoint;
    }

    /*
     * SimulationTimestampTask
     */
    SimulationTimestampTask::SimulationTimestampTask(WorkflowTask *task) : task(task){

    }

    WorkflowTask* SimulationTimestampTask::getTask() {
        return this->task;
    }

    std::map<std::string, SimulationTimestampTask *> SimulationTimestampTask::pending_task_timestamps;

    void SimulationTimestampTask::setEndpoints() {
        auto pending_tasks_itr = pending_task_timestamps.find(this->task->getID());
        if (pending_tasks_itr != pending_task_timestamps.end()) {
            (*pending_tasks_itr).second->endpoint = this;
            this->endpoint = (*pending_tasks_itr).second->endpoint;
            pending_task_timestamps.erase(pending_tasks_itr);
        }
    }

    /*
     * SimulationTimestampTaskXXXX
     */
    SimulationTimestampTaskStart::SimulationTimestampTaskStart(WorkflowTask *task) : SimulationTimestampTask(task) {
        pending_task_timestamps.insert(std::make_pair(task->getID(), this));
    }

    SimulationTimestampTaskFailure::SimulationTimestampTaskFailure(WorkflowTask *task) : SimulationTimestampTask(task){
            if (task != nullptr) {
                setEndpoints();
            }
    }

    SimulationTimestampTaskCompletion::SimulationTimestampTaskCompletion(WorkflowTask *task) : SimulationTimestampTask(task) {
        if (task != nullptr) {
            setEndpoints();
        }
    }
}

