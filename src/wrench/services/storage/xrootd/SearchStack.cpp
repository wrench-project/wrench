/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "wrench/services/storage/xrootd/SearchStack.h"
#include "wrench/services/storage/xrootd/XRootD.h"
#include "wrench/services/storage/xrootd/Node.h"

namespace wrench {
    namespace XRootD{

        SearchStack::SearchStack(Node* terminal,std::shared_ptr<DataFile> file):terminalNode(terminal),file(file),fileLocation(terminalNode->hasFile(file)){
            current=stack.begin();
        }

        Node* SearchStack::moveUp(){
            current--;
            return *current;
        }
        Node* SearchStack::moveDown(){
           current++;
           return *current;
        }
        Node* SearchStack::peak(){
            return *current;

        }
        bool SearchStack::atEnd(){
            return current==stack.end();
        }
        bool SearchStack::atStart(){
            return current==stack.begin();

        }

        void SearchStack::push(Node* node){
            stack.push_front(node);
        }
        bool SearchStack::inTree(Node* potentialParent){
            XRootD* metavisor=terminalNode->metavisor;
            stack=std::list<Node*>();
            push(terminalNode);
            Node* current=terminalNode;
            do{
                push(current);
                if(current==potentialParent){
                    headNode=potentialParent;
                    this->current=stack.begin();
                    return true;
                }
                current=current->getParent();
                if(stack.size()>metavisor->size()){//crude cycle detection, but should prevent infinite unexplainable loop on file read
                    throw std::runtime_error("Cycle detected in XRootD file server.  This version of wrench does not support cycles within XRootD.");
                }
            }while( current!=nullptr);

            return  false;

        }

    }
}
