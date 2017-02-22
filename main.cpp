
#include <iostream>

#include "wrench.h"
#include "computation/sequential_task_executor/SequentialTaskExecutor.h"

int main(int argc, char **argv) {

	WRENCH::Simulation simulation;

	simulation.init(&argc, argv);

	std::cerr << "Creating a bogus workflow..." << std::endl;

	WRENCH::Workflow workflow;

	std::cerr << "Creating two tasks..." << std::endl;

	std::shared_ptr<WRENCH::WorkflowTask> t1 = workflow.addTask("T1", 60.0, 1);
	std::shared_ptr<WRENCH::WorkflowTask> t2 = workflow.addTask("T2", 10.0, 1);
	std::shared_ptr<WRENCH::WorkflowTask> t3 = workflow.addTask("T3", 10.0, 1);
	std::shared_ptr<WRENCH::WorkflowTask> t4 = workflow.addTask("T4", 30.0, 1);

	std::cerr << "Task states: " << t1->getState() << t2->getState() << t3->getState() << t4->getState() << std::endl;
	std::cerr << "Adding control dependency edges..." << std::endl;
	workflow.addControlDependency(t1, t2);
	workflow.addControlDependency(t1, t2);  // redundant, and should be ignored
	workflow.addControlDependency(t3, t4);

	std::cerr << "Task states: " << t1->getState() << t2->getState() << t3->getState() << t4->getState() << std::endl;

	std::cerr << "Creating a couple of files..." << std::endl;

	std::shared_ptr<WRENCH::WorkflowFile> f1 = workflow.addFile("file1", 1000.0);
	std::shared_ptr<WRENCH::WorkflowFile> f2 = workflow.addFile("file2", 2000.0);

	std::cerr << "Adding data dependencies..." << std::endl;

	workflow.addDataDependency(t2, t4, f1);
	workflow.addDataDependency(t3, t4, f2);

	std::cerr << "Task states: " << t1->getState() << t2->getState() << t3->getState() << t4->getState() << std::endl;

	std::cerr << "Counting children..." << std::endl;
	std::cerr << t1->id << " has " << t1->getNumberOfChildren() << " children" << std::endl;
	std::cerr << t2->id << " has " << t2->getNumberOfChildren() << " children" << std::endl;
	std::cerr << t3->id << " has " << t3->getNumberOfChildren() << " children" << std::endl;
	std::cerr << t4->id << " has " << t4->getNumberOfChildren() << " children" << std::endl;

	std::cerr << "Counting parents..." << std::endl;
	std::cerr << t1->id << " has " << t1->getNumberOfParents() << " parents" << std::endl;
	std::cerr << t2->id << " has " << t2->getNumberOfParents() << " parents" << std::endl;
	std::cerr << t3->id << " has " << t3->getNumberOfParents() << " parents" << std::endl;
	std::cerr << t4->id << " has " << t4->getNumberOfParents() << " parents" << std::endl;

//	workflow.exportToEPS("workflow.eps");

	std::cerr << "Instantiating SimGrid platform..." << std::endl;
	WRENCH::Platform platform("./two_hosts.xml");

	std::cerr << "Instantiating a Sequential Task Executor on Tremblay..." << std::endl;
	WRENCH::SequentialTaskExecutor no_op_1 = WRENCH::SequentialTaskExecutor("Tremblay");
	no_op_1.start();

	std::cerr << "Instantiating a Sequential Task Executor on Tremblay..." << std::endl;
	WRENCH::SequentialTaskExecutor no_op_2 = WRENCH::SequentialTaskExecutor("Tremblay");
	no_op_2.start();


	std::cerr << "Instantiating a Sequential Task Executor on Jupiter..." << std::endl;
	WRENCH::SequentialTaskExecutor no_op_3 = WRENCH::SequentialTaskExecutor("Jupiter");
	no_op_3.start();



	std::cerr << "Launching the Simulation..." << std::endl;
	simulation.launch();

	std::cerr << "Simulation done!" << std::endl;



	return 0;
}


