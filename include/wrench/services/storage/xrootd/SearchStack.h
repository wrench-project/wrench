/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef WRENCH_SEARCHSTACK_H
#define WRENCH_SEARCHSTACK_H

#include <list>
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include "wrench/data_file/DataFile.h"

namespace wrench {
    namespace XRootD{
        class Node;

        class SearchStack{
        public:
            SearchStack(Node* terminal,std::shared_ptr<DataFile> file);

            std::list<Node*> stack;
            std::list<Node*>::iterator current;
            Node* terminalNode;
            Node* headNode=nullptr;
            std::shared_ptr<DataFile> file;
            std::shared_ptr<FileLocation> fileLocation;

            Node* moveDown();
            Node* moveUp();
            Node* peak();
            bool atStart();
            bool atEnd();
            void push(Node* node);
            bool inTree(Node* potentialParent);
        };

    }
}
#endif //WRENCH_SEARCHSTACK_H
