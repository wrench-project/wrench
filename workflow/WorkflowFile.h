/**
 *  @file    WorkflowFile.h
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::WorkflowFile class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::WorkflowFile class represents a data file used in a WRENCH::Workflow.
 *
 */

#ifndef WRENCH_WORKFLOWFILE_H
#define WRENCH_WORKFLOWFILE_H

#include <map>
#include <lemon/list_graph.h>


namespace WRENCH {


		class WorkflowFile {

		public:
				std::string id;
				double size; // in bytes

				WorkflowFile(const std::string &string, double s) {
					id = string;
					size = s;
				};
		};

};


#endif //WRENCH_WORKFLOWFILE_H
