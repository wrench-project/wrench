/**
 *  @file    WMS.cpp
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::WMS class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::WMS is a mostly abstract implementation of a WMS
 *
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