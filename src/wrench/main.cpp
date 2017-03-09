
#include <iostream>

#include "wrench.h"

int main(int argc, char **argv) {

	WRENCH::Simulation simulation;

	simulation.init(&argc, argv);

	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << "<xml platform file> <dax file>" << std::endl;
		exit(1);
	}

	char *platform_file = argv[1];
	char *dax_file = argv[2];


	std::cerr << "Creating a bogus workflow..." << std::endl;

	WRENCH::Workflow workflow;

//	std::cerr << "Creating a few tasks..." << std::endl;
//
//	WRENCH::WorkflowTask *t1 = workflow.addTask("T1", 1.0, 1);
//	WRENCH::WorkflowTask *t2 = workflow.addTask("T2", 10.0, 1);
//	WRENCH::WorkflowTask *t3 = workflow.addTask("T3", 10.0, 1);
//	WRENCH::WorkflowTask *t4 = workflow.addTask("T4", 10.0, 1);
//	WRENCH::WorkflowTask *t5 = workflow.addTask("T5", 1.0, 1);
//
//	std::cerr << "Creating a few  files..." << std::endl;
//
//	WRENCH::WorkflowFile *f2 = workflow.addFile("file2", 1000.0);
//	WRENCH::WorkflowFile *f3 = workflow.addFile("file3", 2000.0);
//	WRENCH::WorkflowFile *f4 = workflow.addFile("file4", 2000.0);
//
//	std::cerr << "Adding data dependencies..." << std::endl;
//
//	t1->addOutputFile(f2);
//	t1->addOutputFile(f3);
//	t1->addOutputFile(f4);
//
//	t2->addInputFile(f2);
//	t3->addInputFile(f3);
//	t4->addInputFile(f4);
//
//	std::cerr << "Adding control dependency edges..." << std::endl;
//
//	workflow.addControlDependency(t2, t5);
//	workflow.addControlDependency(t3, t5);
//	workflow.addControlDependency(t4, t5);


	//workflow.exportToEPS("workflow.eps");

	workflow.loadFromDAX(dax_file);
	std::cerr << "The workflow has " << workflow.getNumberOfTasks() << " tasks " << std::endl;
//	std::cerr << "Number of childern of root task: " << workflow.getReadyTasks()[0]->getNumberOfChildren() << std::endl;

//	return 0;

	std::cerr << "Instantiating SimGrid platform..." << std::endl;
	simulation.createPlatform(platform_file);

//	std::cerr << "Instantiating a Sequential Task Executor on Tremblay..." << std::endl;
//	simulation.createSequentialTaskExecutor("Tremblay");
//
//	std::cerr << "Instantiating another Sequential Task Executor on Tremblay..." << std::endl;
//	simulation.createSequentialTaskExecutor("Tremblay");

	std::cerr << "Instantiating a Multicore Task Executor on Jupiter..." << std::endl;
	simulation.createMulticoreTaskExecutor("Jupiter");


	std::cerr << "Instantiating a WMS on Tremblay..." << std::endl;
	simulation.createSimpleWMS(&workflow, "Tremblay");

	std::cerr << "Launching the Simulation..." << std::endl;
	simulation.launch();
	std::cerr << "Simulation done!" << std::endl;

	return 0;
}
