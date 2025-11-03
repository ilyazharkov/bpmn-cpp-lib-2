#include "bpmn/parser.h"
#include "bpmn/model.h"
#include <libxml/xpath.h>
#include <stdexcept>
#include <libxml/xpathInternals.h>

namespace bpmn {

    BpmnParser::BpmnParser() : doc_(nullptr) {}

    BpmnParser::~BpmnParser() {
        if (doc_) xmlFreeDoc(doc_);
    }

    std::unique_ptr<Process> BpmnParser::parse(const std::string& file_path) {
        doc_ = xmlReadFile(file_path.c_str(), nullptr, 0);
        if (!doc_) {
            throw std::runtime_error("Failed to parse BPMN file: " + file_path);
        }

        return parseDocument();
    }

    std::unique_ptr<Process> BpmnParser::parseFromString(const std::string& xml_content) {
        doc_ = xmlReadMemory(xml_content.c_str(), xml_content.length(),
            "noname.xml", nullptr, 0);
        if (!doc_) {
            throw std::runtime_error("Failed to parse BPMN XML content");
        }

        return parseDocument();
    }

    std::unique_ptr<Process> BpmnParser::parseDocument() {
        // Set up XPath context
        xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc_);
        if (!xpathCtx) {
            throw std::runtime_error("Failed to create XPath context");
        }

        // Register BPMN namespace
        if (xmlXPathRegisterNs(xpathCtx, BAD_CAST "bpmn",
            BAD_CAST "http://www.omg.org/spec/BPMN/20100524/MODEL") != 0) {
            xmlXPathFreeContext(xpathCtx);
            throw std::runtime_error("Failed to register BPMN namespace");
        }

        try {
            // Find all process elements
            xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(
                BAD_CAST "//bpmn:process", xpathCtx);

            if (!xpathObj || !xpathObj->nodesetval || xpathObj->nodesetval->nodeNr == 0) {
                xmlXPathFreeObject(xpathObj);
                xmlXPathFreeContext(xpathCtx);
                throw std::runtime_error("No process definition found in BPMN file");
            }

            // Parse the first process found
            xmlNodePtr processNode = xpathObj->nodesetval->nodeTab[0];

            // Get process ID and name
            std::string processId = getAttribute(processNode, "id");
            std::string processName = getAttribute(processNode, "name");

            auto process = std::make_unique<Process>(processId, processName);
            parseProcess(processNode, *process);

            xmlXPathFreeObject(xpathObj);

            // Parse all sequence flows from the entire document
            xpathObj = xmlXPathEvalExpression(BAD_CAST "//bpmn:sequenceFlow", xpathCtx);
            if (xpathObj && xpathObj->nodesetval) {
                for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++) {
                    parseSequenceFlow(xpathObj->nodesetval->nodeTab[i], *process);
                }
            }
            xmlXPathFreeObject(xpathObj);

            xmlXPathFreeContext(xpathCtx);

            return process;
        }
        catch (...) {
            xmlXPathFreeContext(xpathCtx);
            throw;
        }
    }

    void BpmnParser::parseProcess(xmlNodePtr node, Process& process) {
        // Parse all elements in the process
        for (xmlNodePtr cur = node->children; cur; cur = cur->next) {
            if (cur->type != XML_ELEMENT_NODE) continue;

            const std::string nodeName = getNodeName(cur);

            if (nodeName == "startEvent") {
                parseStartEvent(cur, process);
            }
            else if (nodeName == "userTask") {
                parseUserTask(cur, process);
            }
            else if (nodeName == "serviceTask") {
                parseServiceTask(cur, process);
            }
            else if (nodeName == "endEvent") {
                parseEndEvent(cur, process);
            }
            else if (nodeName == "parallelGateway") {
                parseParallelGateway(cur, process);
            }
            else if (nodeName == "exclusiveGateway") {
                parseExclusiveGateway(cur, process);
            }
            // Note: sequence flows are parsed separately from the entire document
        }
    }

    void BpmnParser::parseStartEvent(xmlNodePtr node, Process& process) {
        std::string id = getAttribute(node, "id");
        std::string name = getAttribute(node, "name");

        if (!id.empty()) {
            auto startEvent = std::make_unique<StartEvent>(id, name);
            process.addElement(std::unique_ptr<FlowElement>(startEvent.release()));
        }
    }

    void BpmnParser::parseUserTask(xmlNodePtr node, Process& process) {
        std::string id = getAttribute(node, "id");
        std::string name = getAttribute(node, "name");

        if (!id.empty()) {
            auto userTask = std::make_unique<UserTask>(id, name);
            process.addElement(std::unique_ptr<FlowElement>(userTask.release()));
        }
    }

    void BpmnParser::parseServiceTask(xmlNodePtr node, Process& process) {
        std::string id = getAttribute(node, "id");
        std::string name = getAttribute(node, "name");

        if (!id.empty()) {
            auto serviceTask = std::make_unique<services::ServiceTask>(id, name);
            process.addElement(std::unique_ptr<FlowElement>(serviceTask.release()));
        }
    }

    void BpmnParser::parseEndEvent(xmlNodePtr node, Process& process) {
        std::string id = getAttribute(node, "id");
        std::string name = getAttribute(node, "name");

        if (!id.empty()) {
            auto endEvent = std::make_unique<EndEvent>(id, name);
            process.addElement(std::unique_ptr<FlowElement>(endEvent.release()));
        }
    }

    void BpmnParser::parseParallelGateway(xmlNodePtr node, Process& process) {
        std::string id = getAttribute(node, "id");
        std::string name = getAttribute(node, "name");

        if (!id.empty()) {
            auto parallelGateway = std::make_unique<ParallelGateway>(id, name);
            process.addElement(std::unique_ptr<FlowElement>(parallelGateway.release()));
        }
    }

    void BpmnParser::parseExclusiveGateway(xmlNodePtr node, Process& process) {
        std::string id = getAttribute(node, "id");
        std::string name = getAttribute(node, "name");

        if (!id.empty()) {
            auto exclusiveGateway = std::make_unique<ExclusiveGateway>(id, name);
            process.addElement(std::unique_ptr<FlowElement>(exclusiveGateway.release()));
        }
    }

    void BpmnParser::parseSequenceFlow(xmlNodePtr node, Process& process) {
        std::string id = getAttribute(node, "id");
        std::string name = getAttribute(node, "name");
        std::string sourceRef = getAttribute(node, "sourceRef");
        std::string targetRef = getAttribute(node, "targetRef");

        if (!id.empty() && !sourceRef.empty() && !targetRef.empty()) {
            try {
                process.addSequenceFlow(id, name, sourceRef, targetRef);
            }
            catch (const std::runtime_error& e) {
                // Log warning but don't stop parsing
                // You might want to add logging here
            }
        }
    }

    std::string BpmnParser::getAttribute(xmlNodePtr node, const std::string& attribute_name) {
        xmlChar* value = xmlGetProp(node, BAD_CAST attribute_name.c_str());
        if (value) {
            std::string result(reinterpret_cast<char*>(value));
            xmlFree(value);
            return result;
        }
        return "";
    }

    std::string BpmnParser::getNodeName(xmlNodePtr node) {
        if (node->name) {
            return std::string(reinterpret_cast<const char*>(node->name));
        }
        return "";
    }

} // namespace bpmn