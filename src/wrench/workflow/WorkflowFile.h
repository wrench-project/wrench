/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::WorkflowFile represents a data file used in a WRENCH::Workflow.
 */

#ifndef WRENCH_WORKFLOWFILE_H
#define WRENCH_WORKFLOWFILE_H

#include <string>
#include <map>

namespace wrench {

		class WorkflowTask;
		class Workflow;

		class WorkflowFile {

				friend class Workflow;
				friend class WorkflowTask;

		public:
				std::string id;
				double size; // in bytes

		protected:
				void setOutputOf(WorkflowTask *task);
				WorkflowTask *getOutputOf();
				void setInputOf(WorkflowTask *task);
				std::map<std::string, WorkflowTask *> getInputOf();


		private:
				Workflow *workflow; // Containing workflow
				WorkflowFile(const std::string, double);
				WorkflowTask *output_of;
				std::map<std::string, WorkflowTask *> input_of;

		};

};

#endif //WRENCH_WORKFLOWFILE_H
