//
// Created by Henri Casanova on 2/18/17.
//

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
