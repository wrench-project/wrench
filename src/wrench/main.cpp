
#include <iostream>

#include "wrench.h"

int main(int argc, char **argv) {

  wrench::Simulation simulation;

  simulation.init(&argc, argv);

  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <xml platform file> <dax file>" << std::endl;
    exit(1);
  }

  char *platform_file = argv[1];
  char *dax_file = argv[2];


  std::cerr << "Creating a bogus workflow..." << std::endl;

  wrench::Workflow workflow;

//	std::cerr << "Creating a few tasks..." << std::endl;
//
//	wrench::WorkflowTask *t1 = workflow.addTask("T1", 1.0, 1);
//	wrench::WorkflowTask *t2 = workflow.addTask("T2", 10.0, 1);
//	wrench::WorkflowTask *t3 = workflow.addTask("T3", 10.0, 1);
//	wrench::WorkflowTask *t4 = workflow.addTask("T4", 10.0, 1);
//	wrench::WorkflowTask *t5 = workflow.addTask("T5", 1.0, 1);
//
//	std::cerr << "Creating a few  files..." << std::endl;
//
//	wrench::WorkflowFile *f2 = workflow.addFile("file2", 1000.0);
//	wrench::WorkflowFile *f3 = workflow.addFile("file3", 2000.0);
//	wrench::WorkflowFile *f4 = workflow.addFile("file4", 2000.0);
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
//
//  workflow.exportToEPS("workflow.eps");

  workflow.loadFromDAX(dax_file);
  std::cerr << "The workflow has " << workflow.getNumberOfTasks() << " tasks " << std::endl;
  std::cerr.flush();
//	std::cerr << "Number of children of root task: " << workflow.getReadyTasks()[0]->getNumberOfChildren() << std::endl;

  std::cerr << "Instantiating SimGrid platform..." << std::endl;
  simulation.instantiatePlatform(platform_file);

  try {

    std::cerr << "Instantiating a  MultiCore Job executor on c-1.me..." << std::endl;
    simulation.add(
            std::unique_ptr<wrench::MulticoreJobExecutor>(new wrench::MulticoreJobExecutor("c-1.me", true, false)));

//    std::cerr << "Instantiating a  MultiCore Job executor on c-2.me..." << std::endl;
//		simulation.add(std::unique_ptr<wrench::MulticoreJobExecutor>(new wrench::MulticoreJobExecutor("c-2.me", true, false, {{wrench::MulticoreJobExecutor::Property::STOP_DAEMON_MESSAGE_PAYLOAD, "666"}})));

//    std::cerr << "Instantiating a  MultiCore Job executor on c-3.me..." << std::endl;
//		simulation.add(std::unique_ptr<wrench::MulticoreJobExecutor>(new wrench::MulticoreJobExecutor("c-3.me", false, true, {{wrench::MulticoreJobExecutor::Property::STOP_DAEMON_MESSAGE_PAYLOAD, "666"}})));

//    std::cerr << "Instantiating a  MultiCore Job executor on c-4.me..." << std::endl;
//		simulation.add(std::unique_ptr<wrench::MulticoreJobExecutor>(new wrench::MulticoreJobExecutor("c-4.me", true, true, {{wrench::MulticoreJobExecutor::Property::STOP_DAEMON_MESSAGE_PAYLOAD, "666"}})));

  } catch (std::invalid_argument e) {

    std::cerr << "Error: " << e.what() << std::endl;
    std::exit(1);

  }


  std::cerr << "Instantiating a WMS on c-0.me..." << std::endl;

  std::unique_ptr<wrench::Scheduler> scheduler(new wrench::RandomScheduler());
//  std::unique_ptr<wrench::Scheduler> scheduler(new wrench::MinMinScheduler());
//  std::unique_ptr<wrench::Scheduler> scheduler(new wrench::MaxMinScheduler());
  std::unique_ptr<wrench::StaticOptimization> opt(new wrench::SimplePipelineClustering());

  std::unique_ptr<wrench::WMS> wms(new wrench::SimpleWMS(&simulation, &workflow, std::move(scheduler), "c-0.me"));
  wms.get()->add_static_optimization(std::move(opt));

  simulation.setWMS(std::move(wms));


  std::cerr << "Launching the Simulation..." << std::endl;
  simulation.launch();
  std::cerr << "Simulation done!" << std::endl;

//  std::cerr << simulation.timeStamps[wrench::SimulationTimestamp::Type::TASK_COMPLETION].size() << std::endl;

  return 0;
}
