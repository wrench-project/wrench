//
// Created by Henri Casanova on 2/22/17.
//

#include "WMS.h"

namespace WRENCH {

		WMS::WMS(Platform *p, Workflow *w) {
			this->platform = p;
			this->workflow = w;
		}

		WMS::~WMS() {

		}

};