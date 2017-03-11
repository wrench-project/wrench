/**
 * @brief WRENCH::WMS is a mostly abstract implementation of a WMS
 */

#include "WMS.h"

namespace WRENCH {

		/**
		 * @brief Constructor
		 *
		 * @param w is a pointer to a workflow to execute
		 */
		WMS::WMS(Workflow *w) {
			this->workflow = w;
		}

};