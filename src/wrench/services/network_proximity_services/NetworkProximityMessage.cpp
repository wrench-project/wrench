//
// Created by suraj on 8/6/17.
//

#include "NetworkProximityMessage.h"

namespace wrench {
    /**
     * @brief Constructor
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    NetworkProximityMessage::NetworkProximityMessage(std::string name, double payload) :
            ServiceMessage("NetworkProximity::" + name, payload) {
    }


    /**
     * @brief NetworkProximityLookupRequestMessage class
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param file: the file to look up
     * @param payload: the message size in bytes
     */
    NetworkProximityLookupRequestMessage::NetworkProximityLookupRequestMessage(std::string answer_mailbox, std::pair<std::string,std::string> hosts, double payload) :
            NetworkProximityMessage("PROXIMITY_LOOKUP_REQUEST", payload) {

        if ((answer_mailbox == "") || (std::get<0>(hosts)=="") || (std::get<1>(hosts)=="")) {
            throw std::invalid_argument("NetworkProximityLookupRequestMessage::NetworkProximityLookupRequestMessage(): Invalid argument");
        }
        this->answer_mailbox = answer_mailbox;
        this->hosts = hosts;
    }


    /**
     * @brief NetworkProximityLookupAnswerMessage class
     * @param file: the file that was looked up
     * @param locations: the set of storage services where the file is located
     * @param payload: the message size in bytes
     */
    NetworkProximityLookupAnswerMessage::NetworkProximityLookupAnswerMessage(std::pair<std::string,std::string> hosts, double proximityvalue, double payload) :
            NetworkProximityMessage("PROXIMITY_LOOKUP_ANSWER", payload) {
        if ((std::get<0>(hosts)=="") || (std::get<1>(hosts)=="")) {
            throw std::invalid_argument("NetworkProximityLookupAnswerMessage::NetworkProximityLookupAnswerMessage(): Invalid argument");
        }
        this->hosts = hosts;
        this->proximityValue = proximityvalue;
    }

    /**
     * @brief NetworkProximityLookupAnswerMessage class
     * @param file: the file that was looked up
     * @param locations: the set of storage services where the file is located
     * @param payload: the message size in bytes
     */
    NetworkProximityComputeAnswerMessage::NetworkProximityComputeAnswerMessage(std::pair<std::string,std::string> hosts,double proximityvalue,double payload) :
            NetworkProximityMessage("PROXIMITY_COMPUTE_ANSWER", payload) {
        if ((std::get<0>(hosts)=="") || (std::get<1>(hosts)=="")) {
            throw std::invalid_argument("NetworkProximityComputeAnswerMessage::NetworkProximityComputeAnswerMessage(): Invalid argument");
        }
        this->hosts = hosts;
        this->proximityValue = proximityvalue;
    }


    /**
     * @brief NextContactDaemonRequestMessage class
     * @param string: the mailbox to return the request to
     * @param payload: the message size in bytes
     */
    NextContactDaemonRequestMessage::NextContactDaemonRequestMessage(std::string answer_mailbox, double payload) :
            NetworkProximityMessage("NEXT_CONTACT_DAEMON_REQUEST", payload) {
        this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief NextContactDaemonAnswerMessage class
     * @param string: the next mailbox to contact
     * @param string: the next host to contact
     * @param payload: the message size in bytes
     */
    NextContactDaemonAnswerMessage::NextContactDaemonAnswerMessage(std::string next_host_to_send,std::string next_mailbox_to_send,double payload) :
            NetworkProximityMessage("NEXT_CONTACT_DAEMON_ANSWER", payload) {
        this->next_host_to_send = next_host_to_send;
        this->next_mailbox_to_send = next_mailbox_to_send;
    }


    /**
     * @brief NetworkProximityTransferMessage class
     * @param string: the message to transfer to measure proximity
     * @param payload: the message size in bytes
     */
    NetworkProximityTransferMessage::NetworkProximityTransferMessage(std::string message_to_transfer,double payload) :
            NetworkProximityMessage("NETWORK_PROXIMITY_TRANSFER", payload) {
        this->message_to_transfer = message_to_transfer;
    }
}