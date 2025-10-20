// Persistent Node Manager Header
#ifndef PERSISTENT_NODE_MANAGER_H
#define PERSISTENT_NODE_MANAGER_H

#include <Arduino.h>

class PersistentNodeManager {
public:
    PersistentNodeManager();
    bool begin();
    String getNodeId();
    bool saveConfig(const String& config);
    String loadConfig();

private:
    String nodeId;
    bool isInitialized;

    bool loadNodeId();
    void generateNodeId();
    void saveNodeId();
};

#endif // PERSISTENT_NODE_MANAGER_H